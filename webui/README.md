# Dynamo WebUI - API Integration Summary

## Overview
The WebUI has been completely rewritten to work with the JSON-based API that uses base64 encoding for data and context fields.

## API Format

### GET Request
```json
{
  "key": "mykey"
}
```

### GET Response
```json
[
  {
    "data": "<base64-encoded-value>",
    "context": "<base64-encoded-vector-clock>"
  },
  ...
]
```

### PUT Request
```json
{
  "key": "mykey",
  "data": "<base64-encoded-value>",
  "context": "<base64-encoded-vector-clock>"
}
```

## Key Changes

### 1. **Context Management** 🎯
- **Automatic Storage**: When a GET operation is performed, all contexts (vector clocks) are automatically stored in memory
- **Automatic Reuse**: When performing a PUT operation on a previously GET'd key, the context is automatically included
- **User Feedback**: Notifications inform users whether a context is being used or if a new value is being created

### 2. **Base64 Encoding/Decoding** 🔐
- **PUT Operations**: User input is automatically base64-encoded before sending to the server
- **GET Operations**: Base64-encoded data is automatically decoded for display
- **Context Preservation**: Contexts remain base64-encoded as they're passed directly to subsequent PUT requests

### 3. **Enhanced Response Display** 📊
The response viewer now shows:
- **Raw JSON**: The complete JSON response from the server
- **Decoded Values**: Human-readable decoded data for each version
- **Context Information**: Base64-encoded contexts (ready for reuse in PUT)
- **Version Count**: Number of concurrent versions returned

### 4. **Workflow Example**

#### Read-Modify-Write Pattern:
1. **GET** a key → Context is stored automatically
2. **Modify** the value in the PUT form
3. **PUT** with the same key → Context is automatically included → Server merges with existing versions

#### Create New Value:
1. **PUT** a key without prior GET → Empty context is sent → Server creates new version

## Technical Implementation

### Base64 Utilities
```javascript
function stringToBase64(str) {
    return btoa(unescape(encodeURIComponent(str)));
}

function base64ToString(base64) {
    return decodeURIComponent(escape(atob(base64)));
}
```

### Context Storage
```javascript
let keyContexts = new Map(); // Map<key, context[]>
```

- Contexts are stored per key
- Multiple contexts are preserved (for handling concurrent versions)
- First context is used by default for PUT operations
- Contexts are cleared after successful PUT

### API Communication
- **Content-Type**: `application/json`
- **Method**: GET and POST (both send JSON body)
- **CORS**: Enabled with `mode: 'cors'`

## User Interface Hints

The UI now includes helpful hints:
- **PUT form**: "Context will be used from previous GET if available"
- **GET form**: "Context will be stored for subsequent PUT operations"
- **Notifications**: Real-time feedback about context usage

## Development Notes

### Running the WebUI
```bash
cd webui
./serve.sh
```

### Testing Workflow
1. Add a node (e.g., `localhost:8080`)
2. Perform a GET on a key to retrieve and store contexts
3. Modify the value
4. Perform a PUT - the stored context will be automatically used
5. The response viewer will show if the operation succeeded

## Benefits

✅ **Automatic Context Handling**: No manual context management required  
✅ **Version Tracking**: Proper support for Dynamo's version reconciliation  
✅ **User-Friendly**: Clear feedback about context usage  
✅ **Standards Compliant**: Uses JSON and base64 encoding as per API spec  
✅ **Developer Experience**: Enhanced response viewer with decoded data  

## API Compatibility

This WebUI is compatible with the server implementation in:
- `include/server/server.h` (lines 144-231)
- Handles GET responses as JSON arrays with base64 fields
- Sends PUT requests with base64-encoded data and context
- Properly preserves VectorClock contexts for version reconciliation
