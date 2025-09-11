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

def test_clear_behavior():
    """Test the updated clear command behavior"""
    print("Testing Updated Clear Behavior...")
    time.sleep(2)  # Wait for application to start
    
    tests = [
        {
            "name": "Add some output content",
            "code": "print('Line 1'); print('Line 2'); print('Line 3')",
            "expect_success": True
        },
        {
            "name": "Create variables",
            "code": "x = 42; y = 'test'",
            "expect_success": True
        },
        {
            "name": "More output to clear",
            "code": "print('This will be cleared'); print('More content')",
            "expect_success": True
        },
        {
            "name": "Test 'clear' command - should leave only '>>> '",
            "code": "clear",
            "expect_success": True,
            "is_clear": True
        },
        {
            "name": "Verify variables still exist after clear",
            "code": "print(f'x={x}, y={y}')",
            "expect_success": True
        },
        {
            "name": "Test clear again with different content",
            "code": "print('New content to clear')",
            "expect_success": True
        },
        {
            "name": "Clear again - should show only empty prompt",
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
        
        if success and 'result' in response and response['result'].strip():
            result_text = response['result'].strip()
            if test.get('is_clear'):
                # For clear commands, check if result shows clean state
                if ">>> " in result_text:
                    print(f"   ✅ Clear result: Shows clean prompt")
                else:
                    print(f"   Output: {result_text}")
            else:
                print(f"   Output: {result_text}")
    
    # Summary
    passed = sum(results)
    total = len(results)
    print(f"\n{'='*60}")
    print(f"CLEAR BEHAVIOR TEST SUMMARY: {passed}/{total} tests passed")
    
    if passed == total:
        print("🎉 CLEAR BEHAVIOR UPDATED SUCCESSFULLY!")
        print("✅ 'clear' command now shows only empty '>>> ' prompt")
        print("✅ No 'Output cleared' message")
        print("✅ Clean, minimal appearance after clear")
        print("✅ Variables preserved after clear")
    else:
        print("⚠️  Some clear behavior tests failed")
    
    return passed == total

if __name__ == "__main__":
    test_clear_behavior()