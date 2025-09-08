# Debug TCP Port API

The repl_gui application includes an optional debug TCP server for automated testing. This feature is enabled by default during development but can be disabled for production builds.

## Building with Debug Port

### Enable Debug Port (Default)
```bash
cmake -DENABLE_DEBUG_PORT=ON ..
make repl_gui
```

### Disable Debug Port for Production
```bash
cmake -DENABLE_DEBUG_PORT=OFF ..
make repl_gui
```

## Debug Server Details

- **Port**: 8081 (localhost only)
- **Protocol**: JSON over TCP
- **Security**: Bound to 127.0.0.1 only

## API Commands

### 1. Execute Python Code
```json
{"command": "execute", "code": "x = 42\nprint(x)"}
```

**Response:**
```json
{"status": "success", "result": ">>> x = 42\nprint(x)\n42\n>>> "}
```

### 2. Get Current Output
```json
{"command": "get_output"}
```

**Response:**
```json
{"status": "success", "output": "Python REPL with TCP Integration\nType Python commands below. TCP server listening on port 8080.\n>>> "}
```

### 3. Get Variables List
```json
{"command": "get_variables"}
```

**Response:**
```json
{"status": "success", "variables": ["x: 42", "y: [1, 2, 3]", "greeting: hello world"]}
```

### 4. Clear Output Area
```json
{"command": "clear_output"}
```

**Response:**
```json
{"status": "success", "message": "Output cleared"}
```

### 5. Get Input Text
```json
{"command": "get_input"}
```

**Response:**
```json
{"status": "success", "input": "current input text"}
```

### 6. Set Input Text
```json
{"command": "set_input", "text": "new_variable = 123"}
```

**Response:**
```json
{"status": "success", "message": "Input text set"}
```

### 7. Ping Test
```json
{"command": "ping"}
```

**Response:**
```json
{"status": "success", "message": "pong"}
```

## Example Usage

### Python Test Script
```python
import socket
import json

def send_debug_command(command_dict):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        sock.connect(('127.0.0.1', 8081))
        
        # Send command
        command_json = json.dumps(command_dict)
        sock.send(command_json.encode())
        
        # Receive response
        response = sock.recv(4096).decode()
        return json.loads(response)
    finally:
        sock.close()

# Test ping
response = send_debug_command({"command": "ping"})
print("Ping:", response)

# Execute code
response = send_debug_command({
    "command": "execute", 
    "code": "import math\nresult = math.pi * 2\nprint(f'2π = {result}')"
})
print("Execute:", response)

# Get variables
response = send_debug_command({"command": "get_variables"})
print("Variables:", response)
```

### Bash Test Script
```bash
#!/bin/bash

function debug_command() {
    echo "$1" | nc 127.0.0.1 8081
}

# Ping test
debug_command '{"command": "ping"}'

# Execute Python code
debug_command '{"command": "execute", "code": "x = 42\nprint(x)"}'

# Get output
debug_command '{"command": "get_output"}'

# Get variables
debug_command '{"command": "get_variables"}'
```

### Node.js Test Script
```javascript
const net = require('net');

function sendDebugCommand(command) {
    return new Promise((resolve, reject) => {
        const client = new net.Socket();
        
        client.connect(8081, '127.0.0.1', () => {
            client.write(JSON.stringify(command));
        });
        
        client.on('data', (data) => {
            try {
                const response = JSON.parse(data.toString());
                resolve(response);
            } catch (e) {
                reject(e);
            }
            client.destroy();
        });
        
        client.on('error', reject);
    });
}

async function runTests() {
    // Ping test
    console.log('Ping:', await sendDebugCommand({command: "ping"}));
    
    // Execute code
    console.log('Execute:', await sendDebugCommand({
        command: "execute",
        code: "greeting = 'Hello from automation!'\nprint(greeting)"
    }));
    
    // Get variables
    console.log('Variables:', await sendDebugCommand({command: "get_variables"}));
}

runTests().catch(console.error);
```

## Error Responses

All commands return error responses in this format:
```json
{"status": "error", "message": "Error description"}
```

Common errors:
- `"Unknown command: xyz"` - Invalid command
- `"Missing 'code' parameter"` - Execute command without code
- `"REPL widget not available"` - Internal error

## Thread Safety

All debug commands are automatically executed on the main Qt thread using `QMetaObject::invokeMethod` with `Qt::BlockingQueuedConnection`, ensuring thread-safe access to GUI components.

## Security Considerations

- Debug server only binds to localhost (127.0.0.1)
- No authentication mechanism (suitable for local testing only)
- Should be disabled in production builds
- Can execute arbitrary Python code - only use in trusted environments

## Use Cases

1. **Automated GUI Testing**: Inject test data and verify outputs
2. **Integration Tests**: Test Python REPL functionality programmatically  
3. **Debugging**: Inspect application state during development
4. **Continuous Integration**: Automated testing of GUI application features
5. **Performance Testing**: Measure response times and behavior under load