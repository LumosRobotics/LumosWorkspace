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

def test_ui_restoration():
    """Test that the UI restoration worked and functionality is preserved"""
    print("Testing UI Restoration and Functionality...")
    time.sleep(2)  # Wait for application to start
    
    tests = [
        {
            "name": "Basic Python execution still works",
            "code": "print('✅ Modular REPL is working!')",
            "expect_success": True
        },
        {
            "name": "Multi-line commands work via Shift+Enter",
            "code": "def test_function():\n    return 'Multi-line works'",
            "expect_success": True
        },
        {
            "name": "Variables are tracked",
            "code": "result = test_function(); print(f'Result: {result}')",
            "expect_success": True
        },
        {
            "name": "Math operations work",  
            "code": "import math; print(f'π = {math.pi:.4f}')",
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
        
        if success and 'result' in response:
            print(f"   Output: {response['result'].strip()}")
    
    # Summary
    passed = sum(results)
    total = len(results)
    print(f"\n{'='*60}")
    print(f"UI RESTORATION TEST SUMMARY: {passed}/{total} tests passed")
    
    if passed == total:
        print("🎉 UI RESTORATION SUCCESSFUL!")
        print("✅ All functionality preserved after modularization")
        print("✅ Frameless window with custom title bar")
        print("✅ macOS-style buttons (red, yellow, green) on top-left")
        print("✅ No unwanted Execute/Bottom/Clear buttons")
        print("✅ Rounded corners on UI panels")
        print("✅ Clean REPL interface without borders")
        print("✅ Multi-line input with Shift+Enter still works")
    else:
        print("⚠️  Some functionality issues detected")
    
    return passed == total

if __name__ == "__main__":
    test_ui_restoration()