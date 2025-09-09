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

def test_package_import():
    """Test importing the installed package"""
    print("Testing Package Import in REPL...")
    
    # Test importing requests
    exec_response = send_debug_command({"command": "execute", "code": "import requests"})
    print(f"Import requests response: {exec_response}")
    
    # Test using requests
    test_response = send_debug_command({"command": "execute", "code": "print(f'requests version: {requests.__version__}')"})
    print(f"Version check response: {test_response}")
    
    # Test a simple request (just checking if it's callable)
    usage_response = send_debug_command({"command": "execute", "code": "print(f'requests.get function available: {callable(requests.get)}')"})
    print(f"Usage check response: {usage_response}")
    
    print("\n✅ Package import test complete!")

if __name__ == "__main__":
    test_package_import()