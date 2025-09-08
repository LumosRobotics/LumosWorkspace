#!/usr/bin/env python3
"""
Test script for the repl_gui debug API.

Usage:
1. Start repl_gui application with debug port enabled
2. Run this script: python3 test_debug_api.py
"""

import socket
import json
import time
import sys

def send_debug_command(command_dict, timeout=5):
    """Send a debug command and return the response."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(timeout)
    
    try:
        sock.connect(('127.0.0.1', 8081))
        
        # Send command
        command_json = json.dumps(command_dict)
        sock.send(command_json.encode())
        
        # Receive response
        response = sock.recv(4096).decode()
        return json.loads(response)
    except ConnectionRefusedError:
        print("ERROR: Could not connect to debug port. Is repl_gui running with debug enabled?")
        return None
    except socket.timeout:
        print(f"ERROR: Command timed out after {timeout} seconds")
        return None
    except Exception as e:
        print(f"ERROR: {e}")
        return None
    finally:
        sock.close()

def test_ping():
    """Test basic connectivity."""
    print("Testing ping...")
    response = send_debug_command({"command": "ping"})
    if response and response.get("status") == "success":
        print("✓ Ping successful")
        return True
    else:
        print("✗ Ping failed")
        return False

def test_execute():
    """Test code execution."""
    print("\nTesting code execution...")
    
    # Test simple assignment
    response = send_debug_command({
        "command": "execute", 
        "code": "test_var = 42"
    })
    if response and response.get("status") == "success":
        print("✓ Simple assignment executed")
    else:
        print("✗ Simple assignment failed")
        return False
    
    # Test print output
    response = send_debug_command({
        "command": "execute", 
        "code": "print('Hello from debug API!')"
    })
    if response and response.get("status") == "success":
        print("✓ Print statement executed")
        print(f"  Output preview: {response.get('result', '')[:50]}...")
    else:
        print("✗ Print statement failed")
        return False
    
    return True

def test_variables():
    """Test variable retrieval."""
    print("\nTesting variable retrieval...")
    
    # Set some variables
    send_debug_command({
        "command": "execute", 
        "code": "x = 123\ny = 'test string'\nz = [1, 2, 3]"
    })
    
    # Get variables
    response = send_debug_command({"command": "get_variables"})
    if response and response.get("status") == "success":
        variables = response.get("variables", [])
        print(f"✓ Retrieved {len(variables)} variables")
        for var in variables[:5]:  # Show first 5
            print(f"  {var}")
        if len(variables) > 5:
            print(f"  ... and {len(variables) - 5} more")
        return True
    else:
        print("✗ Variable retrieval failed")
        return False

def test_input_text():
    """Test input text manipulation."""
    print("\nTesting input text manipulation...")
    
    # Set input text
    test_text = "import math\nmath.pi"
    response = send_debug_command({
        "command": "set_input", 
        "text": test_text
    })
    if response and response.get("status") == "success":
        print("✓ Input text set")
    else:
        print("✗ Failed to set input text")
        return False
    
    # Get input text
    response = send_debug_command({"command": "get_input"})
    if response and response.get("status") == "success":
        retrieved_text = response.get("input", "")
        if retrieved_text == test_text:
            print("✓ Input text retrieved correctly")
        else:
            print(f"✗ Input text mismatch. Expected: '{test_text}', Got: '{retrieved_text}'")
            return False
    else:
        print("✗ Failed to get input text")
        return False
    
    return True

def test_output_management():
    """Test output area management."""
    print("\nTesting output management...")
    
    # Get current output
    response = send_debug_command({"command": "get_output"})
    if response and response.get("status") == "success":
        print("✓ Output retrieved")
        print(f"  Output length: {len(response.get('output', ''))} characters")
    else:
        print("✗ Failed to get output")
        return False
    
    # Clear output
    response = send_debug_command({"command": "clear_output"})
    if response and response.get("status") == "success":
        print("✓ Output cleared")
    else:
        print("✗ Failed to clear output")
        return False
    
    return True

def test_error_handling():
    """Test error handling."""
    print("\nTesting error handling...")
    
    # Invalid command
    response = send_debug_command({"command": "invalid_command"})
    if response and response.get("status") == "error":
        print("✓ Invalid command properly rejected")
    else:
        print("✗ Invalid command not properly handled")
        return False
    
    # Missing parameter
    response = send_debug_command({"command": "execute"})  # Missing code
    if response and response.get("status") == "error":
        print("✓ Missing parameter properly rejected")
    else:
        print("✗ Missing parameter not properly handled")
        return False
    
    return True

def run_comprehensive_test():
    """Run a comprehensive test with realistic usage."""
    print("\nRunning comprehensive test...")
    
    # Clear output first
    send_debug_command({"command": "clear_output"})
    
    # Execute a series of Python commands
    commands = [
        "import random",
        "numbers = [random.randint(1, 100) for _ in range(10)]",
        "average = sum(numbers) / len(numbers)",
        "print(f'Numbers: {numbers}')",
        "print(f'Average: {average:.2f}')",
    ]
    
    for cmd in commands:
        response = send_debug_command({"command": "execute", "code": cmd})
        if not response or response.get("status") != "success":
            print(f"✗ Failed to execute: {cmd}")
            return False
        time.sleep(0.1)  # Small delay between commands
    
    # Check that variables were created
    response = send_debug_command({"command": "get_variables"})
    if response and response.get("status") == "success":
        variables = response.get("variables", [])
        var_names = [var.split(":")[0] for var in variables]
        if "numbers" in var_names and "average" in var_names:
            print("✓ Comprehensive test completed successfully")
            return True
    
    print("✗ Comprehensive test failed - expected variables not found")
    return False

def main():
    """Run all tests."""
    print("=== repl_gui Debug API Test Suite ===\n")
    
    # Check connection first
    if not test_ping():
        print("\nTest suite aborted - cannot connect to debug port.")
        print("Make sure repl_gui is running with ENABLE_DEBUG_PORT=ON")
        sys.exit(1)
    
    # Run all tests
    tests = [
        test_execute,
        test_variables,
        test_input_text,
        test_output_management,
        test_error_handling,
        run_comprehensive_test,
    ]
    
    passed = 0
    total = len(tests)
    
    for test in tests:
        if test():
            passed += 1
        else:
            print("Test failed!")
    
    print(f"\n=== Test Results ===")
    print(f"Passed: {passed}/{total}")
    
    if passed == total:
        print("🎉 All tests passed!")
        sys.exit(0)
    else:
        print("❌ Some tests failed!")
        sys.exit(1)

if __name__ == "__main__":
    main()