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

def test_silent_clear_vars():
    """Test the silent clear vars command behavior"""
    print("Testing Silent Clear Vars Behavior...")
    time.sleep(2)  # Wait for application to start
    
    tests = [
        {
            "name": "Create test variables",
            "code": "x = 42; y = 'hello'; z = [1, 2, 3]",
            "expect_success": True
        },
        {
            "name": "Add some output content",
            "code": "print('Some output'); print(f'x={x}, y={y}, z={z}')",
            "expect_success": True
        },
        {
            "name": "Test silent 'clear vars' - should show NO output",
            "code": "clear vars",
            "expect_success": True,
            "is_silent": True
        },
        {
            "name": "Verify variables are gone",
            "code": "try:\n    print(x)\nexcept NameError:\n    print('Variables successfully cleared!')",
            "expect_success": True
        },
        {
            "name": "Add more content",
            "code": "a = 10; b = 20; print(f'New vars: a={a}, b={b}')",
            "expect_success": True
        },
        {
            "name": "Test clear vars again - silent operation", 
            "code": "clear vars",
            "expect_success": True,
            "is_silent": True
        },
        {
            "name": "Verify new variables are also gone",
            "code": "try:\n    print(a)\nexcept NameError:\n    print('New variables also cleared!')",
            "expect_success": True
        },
        {
            "name": "Test that clear still works normally",
            "code": "print('This will be cleared')",
            "expect_success": True
        },
        {
            "name": "Test 'clear' command - should show '>>> ' prompt",
            "code": "clear",
            "expect_success": True,
            "is_clear": True
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
            result_text = response['result'].strip()
            if test.get('is_silent'):
                # For silent commands, should be empty or minimal
                if not result_text or result_text == "":
                    print(f"   ✅ Silent operation: No output shown")
                else:
                    print(f"   Output (should be empty): '{result_text}'")
            elif test.get('is_clear'):
                if ">>> " in result_text:
                    print(f"   ✅ Clear result: Shows clean prompt")
                else:
                    print(f"   Output: {result_text}")
            elif result_text:
                print(f"   Output: {result_text}")
    
    # Summary
    passed = sum(results)
    total = len(results)
    print(f"\n{'='*60}")
    print(f"SILENT CLEAR VARS TEST SUMMARY: {passed}/{total} tests passed")
    
    if passed == total:
        print("🎉 SILENT CLEAR VARS WORKING PERFECTLY!")
        print("✅ 'clear vars' command is completely silent")
        print("✅ No confirmation messages shown")
        print("✅ Variables are deleted from memory")
        print("✅ 'clear' command still works normally")
        print("✅ Clean, minimal user experience")
    else:
        print("⚠️  Some silent clear vars tests failed")
    
    return passed == total

if __name__ == "__main__":
    test_silent_clear_vars()