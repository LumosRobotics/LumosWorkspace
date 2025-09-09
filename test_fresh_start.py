#!/usr/bin/env python3

import socket
import json
import time

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

def test_fresh_start():
    """Test with fresh REPL start"""
    print("Testing Fresh REPL Start...")
    time.sleep(2)  # Wait for REPL to start
    
    # Check initial sys.path
    print("1. Checking initial sys.path...")
    path_response = send_debug_command({"command": "execute", "code": "import sys; print('Initial paths:', len(sys.path))"})
    print(f"Initial path count: {path_response}")
    
    # Add Modules path
    print("2. Adding Modules path...")
    modules_response = send_debug_command({"command": "execute", "code": "import sys; sys.path.append('../third_party/cpython/Modules')"})
    print(f"Modules path add: {modules_response}")
    
    # Try importing _socket
    print("3. Testing _socket import...")
    socket_response = send_debug_command({"command": "execute", "code": "import _socket; print('_socket imported!')"})
    print(f"_socket import: {socket_response}")
    
    # Add site-packages path
    print("4. Adding site-packages path...")
    site_response = send_debug_command({"command": "execute", "code": "import sys; sys.path.append('../third_party/cpython/Lib/site-packages')"})
    print(f"Site-packages add: {site_response}")
    
    # Try importing requests
    print("5. Testing requests import...")
    requests_response = send_debug_command({"command": "execute", "code": "import requests; print(f'Requests {requests.__version__} works!')"})
    print(f"Requests import: {requests_response}")
    
    print("\n✅ Fresh start test complete!")

if __name__ == "__main__":
    test_fresh_start()