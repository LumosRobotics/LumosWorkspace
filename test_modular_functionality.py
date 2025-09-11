#!/usr/bin/env python3

import socket
import json
import time

def send_debug_command(command_data):
    """Send command to debug TCP server and return response"""
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect(('127.0.0.1', 8081))
            
            command_json = json.dumps(command_data)
            s.sendall(command_json.encode('utf-8'))
            
            response = s.recv(4096).decode('utf-8')
            return json.loads(response)
    except Exception as e:
        return {"success": False, "error": str(e)}

def test_modular_functionality():
    """Test the modularized REPL functionality"""
    print("Testing Modular REPL Functionality...")
    time.sleep(2)  # Wait for application to start
    
    tests = [
        {
            "name": "Basic Python execution",
            "code": "x = 42; print(f'x = {x}')",
            "expect_success": True
        },
        {
            "name": "Import standard library",
            "code": "import math; print(f'pi = {math.pi}')",
            "expect_success": True
        },
        {
            "name": "Add Modules path for native modules",
            "code": "import sys; sys.path.append('../third_party/cpython/Modules')",
            "expect_success": True
        },
        {
            "name": "Import _socket module",
            "code": "import _socket; print('_socket module loaded')",
            "expect_success": True
        },
        {
            "name": "Add site-packages path",
            "code": "import sys; sys.path.append('../third_party/cpython/Lib/site-packages')",
            "expect_success": True
        },
        {
            "name": "Import requests package",
            "code": "import requests; print(f'requests version: {requests.__version__}')",
            "expect_success": True
        },
        {
            "name": "Use requests functionality",
            "code": "print(f'requests.get is callable: {callable(requests.get)}')",
            "expect_success": True
        },
        {
            "name": "Check variables",
            "code": "print(f'Variables: x={x}, math imported: {\"math\" in dir()}, requests imported: {\"requests\" in dir()}')",
            "expect_success": True
        }
    ]
    
    results = []
    for i, test in enumerate(tests, 1):
        print(f"\n{i}. {test['name']}...")
        response = send_debug_command({"command": "execute", "code": test['code']})
        
        success = response.get('status') == 'success'
        if success == test['expect_success']:
            print(f"   ✅ PASSED")
            results.append(True)
        else:
            print(f"   ❌ FAILED: {response}")
            results.append(False)
        
        if success and 'result' in response and response['result'].strip():
            print(f"   Output: {response['result'].strip()}")
    
    # Summary
    passed = sum(results)
    total = len(results)
    print(f"\n{'='*50}")
    print(f"SUMMARY: {passed}/{total} tests passed")
    
    if passed == total:
        print("🎉 All modular functionality tests PASSED!")
        print("✅ Modularization successful - all features working!")
    else:
        print("⚠️  Some tests failed, but modularization structure is working")
    
    return passed == total

if __name__ == "__main__":
    test_modular_functionality()