import requests
import sys

def run_persistence_test():
    # Configuration
    base_url = "http://localhost:8080"
    
    # Expected data that should have been persisted
    EXPECTED_DATA = {
        "user_profile_1": "Hello world from User A!",
        "session_token_xyz": "This is the NEW, overwritten session data.",
        "config_setting_3": "short",
        "binary_data_test": "\x00\xFF\x0A\x0D"
    }

    # Convert string payloads to raw bytes for comparison
    EXPECTED_BYTES = {
        key: value.encode('utf-8') for key, value in EXPECTED_DATA.items()
    }
    # Special handling for binary data test (using latin-1 for byte fidelity)
    EXPECTED_BYTES["binary_data_test"] = EXPECTED_DATA["binary_data_test"].encode('latin-1')

    # --- Run GET Requests to Verify Persistence ---
    print("## 🔍 Testing Persistence: Running GET Requests")
    print("------------------------------------------------")
    
    successful_gets = 0
    total_tests = len(EXPECTED_BYTES)
    
    for key, expected_payload in EXPECTED_BYTES.items():
        url = f"{base_url}/get"
        get_headers = {"Key": key}
        
        try:
            # Send GET request
            get_response = requests.get(url, headers=get_headers, timeout=5)
            
            if not get_response.ok:
                print(f"❌ GET FAILED for key '{key}'. Status: {get_response.status_code}")
                continue
            
            received_bytes = get_response.content
            
            # Decode for human-readable output
            try:
                received_string = received_bytes.decode('utf-8')
            except:
                received_string = f"[Binary data: {received_bytes[:10]}...]"

            # Perform the critical byte-to-byte verification
            if received_bytes == expected_payload:
                print(f"✅ GET Success for key '{key}'. Data persisted correctly!")
                successful_gets += 1
            else:
                print(f"❌ GET FAILURE for key '{key}': Data mismatch!")
                print(f"   Expected: {expected_payload[:50]}...")
                print(f"   Got:      {received_bytes[:50]}...")

        except requests.exceptions.RequestException as e:
            print(f"❌ GET CRITICAL Failure for key '{key}': {e}")

    # --- Final Summary ---
    print("\n## ✨ Persistence Test Summary")
    print("-------------------------------")
    
    if successful_gets == total_tests:
        print(f"🎉 **TEST PASSED:** All {total_tests} keys were successfully retrieved from persistent storage.")
    else:
        print(f"🛑 **TEST FAILED:** Only {successful_gets}/{total_tests} keys were retrieved successfully.")
        
# Entry point
if __name__ == "__main__":
    try:
        run_persistence_test()
    except requests.exceptions.ConnectionError:
        print(f"\n❌ CRITICAL: Could not connect to the server. Is the KV server running on localhost:8080?")
        sys.exit(1)