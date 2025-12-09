import requests
import sys
import time

def test_replication():
    """
    Test that data written to one node is replicated to all other nodes.
    
    Test Flow:
    1. Write data to node on port 8080
    2. Verify data appears on nodes 8081 and 8082
    3. Test with various data types (text, binary, large values)
    """
    
    # Configuration
    NODES = [
        {"name": "Node 1", "port": 8080, "url": "http://localhost:8080"},
        {"name": "Node 2", "port": 8081, "url": "http://localhost:8081"},
        {"name": "Node 3", "port": 8082, "url": "http://localhost:8082"},
    ]
    
    # Test data with various types
    TEST_DATA = {
        "repl_test_1": ("Simple text value", "utf-8"),
        "repl_test_2": ("A longer value with special chars: !@#$%^&*()", "utf-8"),
        "repl_test_3": ("Unicode test: 你好世界 🚀 café", "utf-8"),
        "repl_test_4": ("x" * 1000, "utf-8"),  # 1KB of data
        "repl_binary": ("\x00\x01\x02\xFF\xFE", "latin-1"),  # Binary data
    }
    
    print("=" * 70)
    print("🔄 DYNAMO REPLICATION TEST")
    print("=" * 70)
    print()
    
    # First, verify all nodes are running
    print("## 🔌 Step 1: Verifying All Nodes Are Running")
    print("-" * 70)
    
    running_nodes = []
    for node in NODES:
        try:
            # Try a simple health check (assuming /get with non-existent key returns 404 or similar)
            response = requests.get(f"{node['url']}/get", headers={"Key": "_health_check"}, timeout=2)
            print(f"✅ {node['name']} (port {node['port']}) is RUNNING")
            running_nodes.append(node)
        except requests.exceptions.RequestException as e:
            print(f"❌ {node['name']} (port {node['port']}) is DOWN: {e}")
    
    if len(running_nodes) < len(NODES):
        print(f"\n⚠️  WARNING: Only {len(running_nodes)}/{len(NODES)} nodes are running.")
        print("Replication test will continue with available nodes...\n")
    
    if len(running_nodes) < 2:
        print("❌ CRITICAL: Need at least 2 nodes running to test replication.")
        sys.exit(1)
    
    print()
    
    # Test each key-value pair
    print("## 📝 Step 2: Writing Data to Node 1 and Verifying Replication")
    print("-" * 70)
    
    total_tests = len(TEST_DATA)
    successful_replications = 0
    
    for key, (value, encoding) in TEST_DATA.items():
        print(f"\n🔑 Testing key: '{key}'")
        print(f"   Value preview: {repr(value[:50])}...")
        
        # Convert to bytes using appropriate encoding
        value_bytes = value.encode(encoding)
        
        # Step 1: Write to first node
        write_node = running_nodes[0]
        try:
            put_response = requests.post(
                f"{write_node['url']}/put",
                headers={"Key": key, 
                         "Content-Type": "application/octet-stream"
                         },
                data=value_bytes,
                timeout=5
            )
            
            if not put_response.ok:
                print(f"   ❌ WRITE FAILED to {write_node['name']}. Status: {put_response.status_code}")
                continue
            
            print(f"   ✅ Successfully wrote to {write_node['name']}")
            
        except requests.exceptions.RequestException as e:
            print(f"   ❌ WRITE ERROR to {write_node['name']}: {e}")
            continue
        
        # Small delay to allow replication to propagate
        time.sleep(0.1)
        
        # Step 2: Verify on all other nodes
        replication_success = True
        for node in running_nodes[1:]:  # Skip the write node
            try:
                get_response = requests.get(
                    f"{node['url']}/get",
                    headers={"Key": key},
                    timeout=5
                )
                
                if not get_response.ok:
                    print(f"   ❌ REPLICATION FAILED: Key not found on {node['name']}")
                    replication_success = False
                    continue
                
                received_bytes = get_response.content
                
                # Verify data matches
                if received_bytes == value_bytes:
                    print(f"   ✅ Replicated correctly to {node['name']}")
                else:
                    print(f"   ❌ DATA MISMATCH on {node['name']}")
                    print(f"      Expected: {value_bytes[:30]}...")
                    print(f"      Got:      {received_bytes[:30]}...")
                    replication_success = False
                    
            except requests.exceptions.RequestException as e:
                print(f"   ❌ READ ERROR from {node['name']}: {e}")
                replication_success = False
        
        if replication_success and len(running_nodes) > 1:
            successful_replications += 1
            print(f"   🎉 FULL REPLICATION SUCCESS for '{key}'")
    
    print()
    print("=" * 70)
    print("## 📊 REPLICATION TEST SUMMARY")
    print("=" * 70)
    print(f"Nodes tested: {len(running_nodes)}/{len(NODES)}")
    print(f"Keys tested: {total_tests}")
    print(f"Successful replications: {successful_replications}/{total_tests}")
    print()
    
    if successful_replications == total_tests:
        print("🎉 ✅ ALL REPLICATION TESTS PASSED!")
        print("Data is successfully replicating across all nodes.")
        return 0
    else:
        print("🛑 ❌ SOME REPLICATION TESTS FAILED")
        print(f"Failed: {total_tests - successful_replications}/{total_tests}")
        return 1


def test_write_quorum():
    """
    Test write quorum behavior - writes should succeed even if some nodes are down.
    This assumes W=2, N=3 configuration.
    """
    print()
    print("=" * 70)
    print("🔍 BONUS: WRITE QUORUM TEST (Optional)")
    print("=" * 70)
    print()
    print("This test checks if writes succeed with W=2 when one node is down.")
    print("To run this test:")
    print("  1. Keep nodes 8080 and 8081 running")
    print("  2. Stop node 8082")
    print("  3. Write should still succeed (reaching quorum of 2)")
    print()
    print("Manual test command:")
    print('  curl -X PUT http://localhost:8080/put -H "Key: quorum_test" -d "test_value"')
    print()


def test_consistency_check():
    """
    Additional test: Verify all nodes have consistent view of the same keys.
    """
    print()
    print("=" * 70)
    print("🔍 BONUS: CONSISTENCY CHECK")
    print("=" * 70)
    print()
    
    NODES = [
        {"name": "Node 1", "port": 8080, "url": "http://localhost:8080"},
        {"name": "Node 2", "port": 8081, "url": "http://localhost:8081"},
        {"name": "Node 3", "port": 8082, "url": "http://localhost:8082"},
    ]
    
    # Test keys from previous test
    TEST_KEYS = [
        "repl_test_1",
        "repl_test_2",
        "repl_test_3",
    ]
    
    print("Checking if all nodes return the same data for each key...")
    print("-" * 70)
    
    for key in TEST_KEYS:
        print(f"\n🔑 Checking key: '{key}'")
        
        values = {}
        for node in NODES:
            try:
                response = requests.get(
                    f"{node['url']}/get",
                    headers={"Key": key},
                    timeout=2
                )
                if response.ok:
                    values[node['name']] = response.content
                    print(f"   {node['name']}: Found ({len(response.content)} bytes)")
                else:
                    print(f"   {node['name']}: Not found")
            except:
                print(f"   {node['name']}: Unreachable")
        
        # Check consistency
        if len(values) > 1:
            unique_values = len(set(values.values()))
            if unique_values == 1:
                print(f"   ✅ All nodes have CONSISTENT data")
            else:
                print(f"   ❌ INCONSISTENCY DETECTED: {unique_values} different values")


if __name__ == "__main__":
    try:
        # Main replication test
        exit_code = test_replication()
        
        # Optional bonus tests
        test_write_quorum()
        test_consistency_check()
        
        sys.exit(exit_code)
        
    except KeyboardInterrupt:
        print("\n\n⚠️  Test interrupted by user.")
        sys.exit(1)
    except Exception as e:
        print(f"\n\n❌ CRITICAL ERROR: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)