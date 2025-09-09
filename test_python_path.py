#!/usr/bin/env python3

import socket
import json

def send_debug_command(command_data):
    """Send command to debug TCP server and return response"""
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect(('127.0.0.1', 8081))
            
            # Send command as JSON
            command_json = json.dumps(command_data)
            s.sendall(command_json.encode('utf-8'))
            
            # Receive response
            response = s.recv(4096).decode('utf-8')
            return json.loads(response)
    except Exception as e:
        return {"success": False, "error": str(e)}

def check_python_path():
    """Check Python path and site-packages availability"""
    print("Checking Python Path...")
    
    # Check sys.path
    path_response = send_debug_command({"command": "execute", "code": "import sys; print('\\n'.join(sys.path))"})
    print(f"sys.path response: {path_response}")
    
    # Check if we can add site-packages
    add_path_response = send_debug_command({"command": "execute", "code": "import sys; sys.path.append('../third_party/cpython/Lib/site-packages')"})
    print(f"Add path response: {add_path_response}")
    
    # Try importing requests again
    import_response = send_debug_command({"command": "execute", "code": "import requests; print(f'Successfully imported requests {requests.__version__}')"})
    print(f"Import after path fix: {import_response}")

if __name__ == "__main__":
    check_python_path()