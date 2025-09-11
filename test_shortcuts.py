#!/usr/bin/env python3

import socket
import json
import time

def send_debug_command(command_data):
    """Send command to debug TCP server and return response"""
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(5)  # 5 second timeout
            s.connect(('127.0.0.1', 8081))
            
            command_json = json.dumps(command_data)
            s.sendall(command_json.encode('utf-8'))
            
            response = s.recv(4096).decode('utf-8')
            return json.loads(response)
    except Exception as e:
        return {"success": False, "error": str(e)}

def test_app_running():
    """Test if the application is running and responsive"""
    print("Testing Application Shortcuts...")
    time.sleep(2)  # Wait for application to start
    
    # Test if app is responsive
    response = send_debug_command({"command": "execute", "code": "print('App is running and responsive!')"})
    
    if response.get('status') == 'success':
        print("✅ Application is running and responsive")
        print("📋 Manual tests to perform:")
        print("   1. Press Ctrl+C when REPL input is NOT focused -> Should close app")
        print("   2. Press Ctrl+Q anywhere -> Should close app")  
        print("   3. Click the red button (top-left) -> Should close app")
        print("   4. Press Ctrl+C while typing in REPL input -> Should NOT close app")
        print("\n🎯 Try these shortcuts manually in the GUI application")
        return True
    else:
        print("❌ Application not responsive:", response)
        return False

if __name__ == "__main__":
    success = test_app_running()
    if success:
        print("\n✅ Ready for manual shortcut testing!")
        print("The application is running in the background.")
        print("Test the shortcuts and then manually close the app.")
    else:
        print("\n❌ Application testing failed")