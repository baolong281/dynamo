#!/usr/bin/env python3
"""
Chaos Testing Script for Dynamo Cluster

Launches a cluster and introduces chaos:
- Random node outages via SIGINT/SIGQUIT
- Node respawning after random delay
- Load simulation with PUT/GET requests
- Failure aggregation per node on termination
"""

import subprocess
import signal
import random
import time
import threading
import sys
import os
import requests
import base64
import string
from dataclasses import dataclass, field
from typing import Dict, Optional, List, Set
from datetime import datetime

# Configuration
BASE_PORT = 8080
NUM_NODES = 10
CHAOS_INTERVAL_MIN = 5.0  # seconds
CHAOS_INTERVAL_MAX = 15.0
RESPAWN_DELAY_MIN = 3.0
RESPAWN_DELAY_MAX = 8.0
LOAD_INTERVAL_MIN = 0.5
LOAD_INTERVAL_MAX = 2.0
BUILD_DIR = "build"
EXECUTABLE = "./build/app"

# ANSI colors
class Colors:
    RED = '\033[91m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    MAGENTA = '\033[95m'
    CYAN = '\033[96m'
    BOLD = '\033[1m'
    RESET = '\033[0m'


@dataclass
class NodeStats:
    successes: int = 0
    failures: int = 0
    redirect_failures: int = 0


@dataclass
class NodeInfo:
    port: int
    process: Optional[subprocess.Popen] = None
    alive: bool = False
    is_bootstrap: bool = False
    stats: NodeStats = field(default_factory=NodeStats)


class ChaosCluster:
    def __init__(self):
        self.nodes: Dict[int, NodeInfo] = {}
        self.lock = threading.Lock()
        self.running = False
        self.chaos_thread: Optional[threading.Thread] = None
        self.load_thread: Optional[threading.Thread] = None
        self.known_keys: List[str] = []

    def log(self, message: str, color: str = ""):
        timestamp = datetime.now().strftime("%H:%M:%S")
        print(f"{color}[{timestamp}] {message}{Colors.RESET}")

    def log_node(self, port: int, message: str):
        # Only log if node is supposed to be alive (to avoid zombie logs)
        with self.lock:
            if port in self.nodes and self.nodes[port].alive:
                print(f"{Colors.CYAN}[Node {port}]{Colors.RESET} {message}", end="")

    def build(self):
        """Build the project"""
        self.log("Building project...", Colors.YELLOW)
        os.makedirs(BUILD_DIR, exist_ok=True)
        
        # CMake
        result = subprocess.run(
            ["cmake", ".."],
            cwd=BUILD_DIR,
            capture_output=True,
            text=True
        )
        if result.returncode != 0:
            self.log(f"CMake failed: {result.stderr}", Colors.RED)
            return False
        
        # Make
        result = subprocess.run(
            ["make", "-j"],
            cwd=BUILD_DIR,
            capture_output=True,
            text=True
        )
        if result.returncode != 0:
            self.log(f"Make failed: {result.stderr}", Colors.RED)
            return False
        
        self.log("Build complete!", Colors.GREEN)
        return True

    def start_node(self, port: int, is_bootstrap: bool = False):
        """Start a single node"""
        cmd = [EXECUTABLE, "--port", str(port), "--address", "localhost"]
        if not is_bootstrap:
            cmd.extend(["--bootstrap-servers", f"localhost:{BASE_PORT}"])
        
        process = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1
        )
        
        # Stream output in background
        def stream_output(p, prt):
            try:
                for line in p.stdout:
                    self.log_node(prt, line)
            except:
                pass
        
        threading.Thread(target=stream_output, args=(process, port), daemon=True).start()
        
        with self.lock:
            if port in self.nodes:
                self.nodes[port].process = process
                self.nodes[port].alive = True
            else:
                self.nodes[port] = NodeInfo(port=port, process=process, alive=True, is_bootstrap=is_bootstrap)
        
        return process

    def start_cluster(self):
        """Start the full cluster"""
        self.log("=" * 60, Colors.BOLD)
        self.log("Starting bootstrap node on port 8080...", Colors.GREEN)
        self.log("=" * 60, Colors.BOLD)
        
        self.start_node(BASE_PORT, is_bootstrap=True)
        time.sleep(2)  # Wait for bootstrap to initialize
        
        for i in range(1, NUM_NODES):
            port = BASE_PORT + i
            self.log(f"Starting node on port {port}...", Colors.GREEN)
            self.start_node(port)
            time.sleep(0.3)
        
        self.log("=" * 60, Colors.BOLD)
        self.log(f"Cluster running with {NUM_NODES} nodes!", Colors.GREEN)
        self.log("=" * 60, Colors.BOLD)

    def get_active_nodes(self) -> List[int]:
        """Get list of active node ports"""
        with self.lock:
            return [port for port, info in self.nodes.items() if info.alive]

    def kill_node(self, port: int, sig: signal.Signals):
        """Kill a node with specified signal"""
        with self.lock:
            if port not in self.nodes or not self.nodes[port].alive:
                return False
            
            node = self.nodes[port]
            sig_name = "SIGINT" if sig == signal.SIGINT else "SIGQUIT"
            
            print()
            self.log("=" * 60, Colors.RED)
            self.log(f"⚡⚡⚡ NODE KILLED: Port {port} ({sig_name}) ⚡⚡⚡", Colors.RED + Colors.BOLD)
            self.log("=" * 60, Colors.RED)
            print()
            
            try:
                node.process.send_signal(sig)
                node.alive = False
                node.process = None # Clear process after killing
            except:
                pass
            
            # Start respawn timer
            threading.Thread(target=self.respawn_timer, args=(port,), daemon=True).start()
            return True

    def respawn_timer(self, port: int):
        """Wait and then restart the node"""
        delay = random.uniform(RESPAWN_DELAY_MIN, RESPAWN_DELAY_MAX)
        time.sleep(delay)
        
        if not self.running:
            return

        self.log("=" * 60, Colors.GREEN)
        self.log(f"♻️  RESPAWNING NODE: Port {port}", Colors.GREEN + Colors.BOLD)
        self.log("=" * 60, Colors.GREEN)
        self.start_node(port, is_bootstrap=(port == BASE_PORT))

    def chaos_loop(self):
        """Background thread: randomly kill nodes"""
        self.log("Chaos daemon started!", Colors.MAGENTA)
        
        while self.running:
            interval = random.uniform(CHAOS_INTERVAL_MIN, CHAOS_INTERVAL_MAX)
            time.sleep(interval)
            
            if not self.running:
                break
            
            active = self.get_active_nodes()
            # Protect bootstrap (8080) for stability, or allow killing it too? 
            # User said "similar to launch_cluster" which has a dedicated bootstrap. 
            # Let's keep bootstrap alive to maintain the ring structure easily, 
            # but allow killing others.
            killable = [p for p in active if p != BASE_PORT]
            
            if not killable:
                continue
            
            target = random.choice(killable)
            sig = random.choice([signal.SIGINT, signal.SIGQUIT])
            self.kill_node(target, sig)

    def string_to_base64(self, s: str) -> str:
        """Encode string to base64 (matching webui pattern)"""
        return base64.b64encode(s.encode('utf-8')).decode('utf-8')

    def base64_to_string(self, b64: str) -> str:
        """Decode base64 to string"""
        return base64.b64decode(b64).decode('utf-8')

    def random_key(self) -> str:
        """Generate random key"""
        return ''.join(random.choices(string.ascii_lowercase, k=8))

    def random_value(self) -> str:
        """Generate random value"""
        return ''.join(random.choices(string.ascii_letters + string.digits, k=20))

    def handle_request(self, method: str, port: int, url: str, json_body: dict, label: str) -> bool:
        """Execute request with quiet redirect handling"""
        try:
            # We handle redirects manually to check if target node is down
            resp = requests.request(method, url, json=json_body, timeout=2, allow_redirects=False)
            
            if resp.status_code == 307:
                target_url = resp.headers.get('Location')
                if target_url:
                    # Extract port from target URL
                    try:
                        target_port = int(target_url.split(':')[-1].split('/')[0])
                        with self.lock:
                            is_target_down = target_port in self.nodes and not self.nodes[target_port].alive
                        
                        if is_target_down:
                            # Squelch error as target node is known to be down
                            # self.log(f"{label} -> :{port} (Redirect to DOWN node :{target_port}) [SQUELCHED]", Colors.BLUE)
                            with self.lock:
                                self.nodes[port].stats.redirect_failures += 1
                            return False
                        
                        # Try the redirect
                        resp = requests.request(method, target_url, json=json_body, timeout=2, allow_redirects=True)
                    except:
                        pass # Fall through to generic error handling

            if resp.status_code == 200:
                with self.lock:
                    self.nodes[port].stats.successes += 1
                return True
            else:
                with self.lock:
                    self.nodes[port].stats.failures += 1
                self.log(f"{label} -> :{port} failed: {resp.status_code}", Colors.YELLOW)
                return False
                
        except (requests.ConnectionError, requests.Timeout):
            # Check if node is dead before logging error
            with self.lock:
                is_dead = port in self.nodes and not self.nodes[port].alive
                if not is_dead:
                    self.nodes[port].stats.failures += 1
                    self.log(f"{label} -> :{port} ERROR: Node unreachable but should be alive", Colors.RED)
            return False
        except Exception as e:
            with self.lock:
                self.nodes[port].stats.failures += 1
            self.log(f"{label} -> :{port} ERROR: {type(e).__name__}", Colors.RED)
            return False

    def do_put(self, port: int, key: str, value: str) -> bool:
        """Execute PUT request"""
        url = f"http://localhost:{port}/put"
        body = {
            "key": key,
            "data": self.string_to_base64(value),
            "context": ""
        }
        if self.handle_request("POST", port, url, body, f"PUT {key}={value[:5]}..."):
            self.log(f"PUT {key}={value[:10]}... -> :{port} ✓", Colors.GREEN)
            return True
        return False

    def do_get(self, port: int, key: str) -> bool:
        """Execute GET request"""
        url = f"http://localhost:{port}/get"
        body = {"key": key}
        if self.handle_request("POST", port, url, body, f"GET {key}"):
            self.log(f"GET {key} -> :{port} ✓", Colors.GREEN)
            return True
        return False

    def load_loop(self):
        """Background thread: simulate load"""
        self.log("Load simulator started!", Colors.BLUE)
        
        while self.running:
            interval = random.uniform(LOAD_INTERVAL_MIN, LOAD_INTERVAL_MAX)
            time.sleep(interval)
            
            active = self.get_active_nodes()
            if not active or not self.running:
                continue
            
            target = random.choice(active)
            
            # 60% PUT, 40% GET
            if random.random() < 0.6 or not self.known_keys:
                key = self.random_key()
                value = self.random_value()
                if self.do_put(target, key, value):
                    with self.lock:
                        self.known_keys.append(key)
                        if len(self.known_keys) > 100:
                            self.known_keys = self.known_keys[-50:]
            else:
                key = random.choice(self.known_keys)
                self.do_get(target, key)

    def print_summary(self):
        """Print failure aggregation per node"""
        print("\n" + "=" * 60)
        print(f"{Colors.BOLD}CHAOS TEST SUMMARY{Colors.RESET}")
        print("=" * 60)
        print(f"{'Node':<10} | {'Success':<10} | {'Failure':<10} | {'Redirect Down':<15}")
        print("-" * 60)
        
        with self.lock:
            total_success = 0
            total_failure = 0
            sorted_ports = sorted(self.nodes.keys())
            for port in sorted_ports:
                stats = self.nodes[port].stats
                total_success += stats.successes
                total_failure += stats.failures
                
                status_color = Colors.GREEN if stats.failures == 0 else Colors.YELLOW
                if stats.failures > stats.successes and stats.successes > 0:
                    status_color = Colors.RED
                
                print(f"{status_color}Port {port:<5}{Colors.RESET} | "
                      f"{stats.successes:<10} | "
                      f"{stats.failures:<10} | "
                      f"{stats.redirect_failures:<15}")
        
        print("-" * 60)
        print(f"{Colors.BOLD}TOTAL:{Colors.RESET}      | {total_success:<10} | {total_failure:<10}")
        print("=" * 60 + "\n")

    def start(self):
        """Start everything"""
        if not os.path.exists(EXECUTABLE):
            if not self.build():
                return False
        
        self.running = True
        self.start_cluster()
        
        time.sleep(2)
        
        self.chaos_thread = threading.Thread(target=self.chaos_loop, daemon=True)
        self.chaos_thread.start()
        
        self.load_thread = threading.Thread(target=self.load_loop, daemon=True)
        self.load_thread.start()
        
        self.log("=" * 60, Colors.BOLD)
        self.log("Chaos testing active! Press Ctrl+C to stop.", Colors.MAGENTA)
        self.log("=" * 60, Colors.BOLD)
        
        return True

    def stop(self):
        """Stop everything"""
        self.running = False
        self.log("Shutting down...", Colors.YELLOW)
        
        with self.lock:
            for port, node in self.nodes.items():
                if node.process:
                    try:
                        node.process.terminate()
                        node.process.wait(timeout=1)
                    except:
                        try:
                            node.process.kill()
                        except:
                            pass
        
        self.print_summary()
        self.log("All nodes stopped.", Colors.GREEN)


def main():
    cluster = ChaosCluster()
    
    def signal_handler(sig, frame):
        cluster.stop()
        sys.exit(0)
    
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    if not cluster.start():
        sys.exit(1)
    
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        pass # Summary is printed in stop() called by signal handler


if __name__ == "__main__":
    main()
