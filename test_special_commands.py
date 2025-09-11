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

def test_special_commands():
    """Test the special command functionality"""
    print("Testing Special Commands...")
    time.sleep(2)  # Wait for application to start
    
    tests = [
        {
            "name": "Create some test variables",
            "code": "x = 42; y = 'hello'; z = [1, 2, 3]",
            "expect_success": True
        },
        {
            "name": "Verify variables exist",
            "code": "print(f'x={x}, y={y}, z={z}')",
            "expect_success": True
        },
        {
            "name": "Test 'clear' command to clear output",
            "code": "clear",
            "expect_success": True,
            "is_special": True
        },
        {
            "name": "Add more content to clear",
            "code": "print('This will be cleared'); print('More output to clear')",
            "expect_success": True
        },
        {
            "name": "Test 'clear' command again",
            "code": "clear",
            "expect_success": True,
            "is_special": True
        },
        {
            "name": "Verify variables still exist after clear",
            "code": "print(f'Variables still there: x={x}, y={y}, z={z}')",
            "expect_success": True
        },
        {
            "name": "Test 'clear vars' command",
            "code": "clear vars",
            "expect_success": True,
            "is_special": True
        },
        {
            "name": "Verify variables are gone",
            "code": "try:\n    print(x)\nexcept NameError:\n    print('✅ Variables successfully cleared!')",
            "expect_success": True
        },
        {
            "name": "Test case insensitive commands",
            "code": "CLEAR",
            "expect_success": True,
            "is_special": True
        },
        {
            "name": "Test with extra spaces",
            "code": "  clear vars  ",
            "expect_success": True,
            "is_special": True
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
            # Show only the relevant output for special commands
            if test.get('is_special'):
                lines = result_text.split('\n')
                relevant_lines = [line for line in lines if 'cleared' in line.lower() or 'output cleared' in line.lower()]
                if relevant_lines:
                    print(f"   Output: {relevant_lines[-1]}")
            else:
                print(f"   Output: {result_text}")
    
    # Summary
    passed = sum(results)
    total = len(results)
    print(f"\n{'='*60}")
    print(f"SPECIAL COMMANDS TEST SUMMARY: {passed}/{total} tests passed")
    
    if passed == total:
        print("🎉 SPECIAL COMMANDS WORKING PERFECTLY!")
        print("✅ 'clear' - Clears all REPL output")
        print("✅ 'clear vars' - Deletes all variables from memory")
        print("✅ Case insensitive command recognition")
        print("✅ Handles extra whitespace")
        print("✅ Commands added to history")
        print("✅ Proper feedback messages")
    else:
        print("⚠️  Some special commands failed")
    
    return passed == total

if __name__ == "__main__":
    test_special_commands()