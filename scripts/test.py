import requests
import sys

# Configuration for the key that will be overwritten
OVERWRITE_KEY = "session_token_xyz"
OVERWRITE_NEW_MESSAGE = "This is the NEW, overwritten session data."

def run_multi_key_test_with_overwrite():
    # Configuration
    base_url = "http://localhost:8080"
    
    # 1. Define Multiple Key-Value Payloads
    # Dictionary where key is the KV-server key and the value is the raw byte payload
    TEST_DATA = {
        "user_profile_1": "Hello world from User A!",
        OVERWRITE_KEY: "This is the ORIGINAL session token.", # This key will be overwritten later
        "config_setting_3": "short",
        "binary_data_test": "\x00\xFF\x0A\x0D" 
    }

    # Convert string payloads to raw bytes for testing
    PAYLOAD_BYTES = {
        key: value.encode('utf-8') for key, value in TEST_DATA.items()
    }
    # Special handling for binary data test (using latin-1 for byte fidelity)
    PAYLOAD_BYTES["binary_data_test"] = TEST_DATA["binary_data_test"].encode('latin-1') 
    
    # Define the NEW payload bytes for the overwrite test
    NEW_PAYLOAD_BYTES = OVERWRITE_NEW_MESSAGE.encode('utf-8')


    # --- Step 1: Run Initial Batch PUT Requests ---
    print("## 🚀 Step 1: Running Initial Batch PUT Requests (including the key to be overwritten)")
    print("----------------------------------------------------------------------------------")
    
    successful_puts = 0
    for key, payload in PAYLOAD_BYTES.items():
        url = f"{base_url}/put"
        put_headers = {
            "Key": key,
            "Content-Type": "application/octet-stream"
        }
        
        try:
            put_response = requests.post(url, data=payload, headers=put_headers, timeout=5)
            
            if put_response.ok:
                print(f"✅ PUT Success for key '{key}'.")
                successful_puts += 1
            else:
                print(f"❌ PUT FAILED for key '{key}'. Status: {put_response.status_code}")
        
        except requests.exceptions.RequestException as e:
            print(f"❌ PUT CRITICAL Failure for key '{key}': {e}")
            sys.exit(1) # Exit if connection fails

    
    # --- Step 2: Overwrite and Verification Test ---
    print(f"\n## 🔁 Step 2: Overwrite Key '{OVERWRITE_KEY}' and Verify")
    print("----------------------------------------------------------")
    
    overwrite_success = False
    
    # 2a. Overwrite PUT request
    url_put = f"{base_url}/put"
    put_headers = {
        "Key": OVERWRITE_KEY,
        "Content-Type": "application/octet-stream"
    }
    
    try:
        overwrite_response = requests.post(url_put, data=NEW_PAYLOAD_BYTES, headers=put_headers, timeout=5)
        print(f"Overwrite PUT Status: {overwrite_response.status_code}")

        if not overwrite_response.ok:
            print("❌ Overwrite PUT failed. Cannot proceed with verification.")
        else:
            # 2b. Verification GET request
            url_get = f"{base_url}/get"
            get_headers = {"Key": OVERWRITE_KEY}
            
            verify_response = requests.get(url_get, headers=get_headers, timeout=5)
            received_bytes = verify_response.content
            
            print(f"Verification GET Status: {verify_response.status_code}")
            
            if received_bytes == NEW_PAYLOAD_BYTES:
                print("✅ **OVERWRITE SUCCESS:** Retrieved data matches the NEW payload.")
                print(f"   New Value: '{received_bytes.decode('utf-8')}'")
                overwrite_success = True
            else:
                print("❌ **OVERWRITE FAILURE:** Retrieved data does NOT match the NEW payload.")
                print(f"   Expected (New) Bytes: {NEW_PAYLOAD_BYTES}")
                print(f"   Got:                  {received_bytes}")
                
    except requests.exceptions.RequestException as e:
        print(f"❌ OVERWRITE CRITICAL Failure for key '{OVERWRITE_KEY}': {e}")
        sys.exit(1)


    # --- Step 3: Run Remaining GET and Verification Requests ---
    print("\n## 🔍 Step 3: Running Batch GETs (Checking if other keys were affected)")
    print("-----------------------------------------------------------------------")
    
    successful_gets = 0
    total_tests = len(PAYLOAD_BYTES)
    
    for key, expected_payload in PAYLOAD_BYTES.items():
        # NOTE: If the key was overwritten, we use the new expected payload for the check
        if key == OVERWRITE_KEY:
            expected_payload = NEW_PAYLOAD_BYTES

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
                print(f"✅ GET Success for key '{key}'. Verified!")
                successful_gets += 1
            else:
                print(f"❌ GET FAILURE for key '{key}': Data mismatch!")

        except requests.exceptions.RequestException as e:
            print(f"❌ GET CRITICAL Failure for key '{key}': {e}")

    # --- Final Summary ---
    print("\n## ✨ Test Summary")
    print("--------------------")
    
    if overwrite_success and successful_gets == total_tests:
        print(f"🎉 **TEST PASSED:** All {total_tests} keys were verified, and the overwrite test was successful.")
    else:
        print("🛑 **TEST FAILED:** One or more checks (PUT, GET, or OVERWRITE) failed.")
        
# Entry point
if __name__ == "__main__":
    try:
        run_multi_key_test_with_overwrite()
    except requests.exceptions.ConnectionError:
        print(f"\n❌ CRITICAL: Could not connect to {base_url}. Is the KV server running on localhost:8080?")