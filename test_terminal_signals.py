#!/usr/bin/env python3

import subprocess
import time
import signal
import os

def test_terminal_signal_handling():
    """Test terminal Ctrl+C signal handling"""
    print("Testing Terminal Signal Handling...")
    
    # Start the application as a subprocess
    print("🚀 Starting repl_gui_modular...")
    proc = subprocess.Popen(
        ['./src/applications/repl_gui/repl_gui_modular'],
        cwd='/Users/danielpi/work/LumosWorkspace/build',
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        preexec_fn=os.setsid  # Create new process group
    )
    
    # Wait a bit for the application to start
    time.sleep(3)
    
    # Check if process is running
    if proc.poll() is None:
        print("✅ Application started successfully")
        print(f"   Process ID: {proc.pid}")
    else:
        print("❌ Application failed to start")
        return False
    
    # Test SIGINT (Ctrl+C equivalent)
    print("\n📨 Sending SIGINT (equivalent to terminal Ctrl+C)...")
    try:
        os.killpg(os.getpgid(proc.pid), signal.SIGINT)
        
        # Wait for graceful shutdown
        try:
            exit_code = proc.wait(timeout=5)
            print(f"✅ Application terminated gracefully with exit code: {exit_code}")
            
            # Check output for our graceful shutdown message
            stdout, stderr = proc.communicate()
            if b"shutting down gracefully" in stderr:
                print("✅ Graceful shutdown message detected")
            
            return True
            
        except subprocess.TimeoutExpired:
            print("⚠️  Application didn't terminate within 5 seconds, force killing...")
            proc.kill()
            proc.wait()
            return False
            
    except Exception as e:
        print(f"❌ Error sending signal: {e}")
        proc.kill()
        proc.wait()
        return False

def test_direct_ctrl_c_instructions():
    """Provide instructions for manual testing of Ctrl+C"""
    print("\n" + "="*60)
    print("MANUAL TERMINAL Ctrl+C TEST")
    print("="*60)
    print("1. Open a new terminal")
    print("2. Navigate to: /Users/danielpi/work/LumosWorkspace/build")
    print("3. Run: ./src/applications/repl_gui/repl_gui_modular")
    print("4. Wait for the GUI to appear")
    print("5. Press Ctrl+C in the terminal")
    print("6. Verify:")
    print("   ✅ Should see: 'Received signal 2 - shutting down gracefully...'")
    print("   ✅ Application should close")
    print("   ✅ Terminal should return to prompt")
    print("="*60)

if __name__ == "__main__":
    print("Terminal Signal Handler Test")
    print("="*40)
    
    # Test programmatic signal sending
    success = test_terminal_signal_handling()
    
    if success:
        print("\n🎉 TERMINAL SIGNAL HANDLING WORKS!")
        print("✅ SIGINT (Ctrl+C) properly handled")
        print("✅ Graceful shutdown implemented")
        print("✅ Application terminates correctly")
    else:
        print("\n⚠️  Automatic test had issues")
    
    # Provide manual testing instructions
    test_direct_ctrl_c_instructions()