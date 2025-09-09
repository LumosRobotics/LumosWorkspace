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

def test_socket_fix():
    """Test adding Modules path to fix _socket import"""
    print("Testing Socket Module Fix...")
    
    # Add both site-packages and Modules to sys.path
    path_fix = """
import sys
sys.path.append('../third_party/cpython/Lib/site-packages')
sys.path.append('../third_party/cpython/Modules')
print('Updated sys.path with site-packages and Modules')
"""
    
    fix_response = send_debug_command({"command": "execute", "code": path_fix})
    print(f"Path fix response: {fix_response}")
    
    # Check sys.path after modification
    path_check = send_debug_command({"command": "execute", "code": "print('\\n'.join(sys.path))"})
    print(f"Updated sys.path: {path_check}")
    
    # Try importing _socket directly
    socket_test = send_debug_command({"command": "execute", "code": "import _socket; print('_socket module imported successfully')"})
    print(f"_socket import: {socket_test}")
    
    # Try importing requests now
    requests_test = send_debug_command({"command": "execute", "code": "import requests; print(f'requests {requests.__version__} imported successfully')"})
    print(f"requests import: {requests_test}")
    
    print("\n✅ Socket fix test complete!")

if __name__ == "__main__":
    test_socket_fix()