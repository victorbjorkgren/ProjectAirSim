#!/usr/bin/env python3
"""
Environment check script to diagnose OpenCV display issues.
Run this script to see what environment variables and settings
might be causing OpenCV display problems.
"""

import os
import sys
import cv2


def check_environment():
    """Check various environment variables and settings."""
    print("=== Environment Check for OpenCV Display Issues ===\n")
    
    # Check Python version
    print(f"Python version: {sys.version}")
    print(f"OpenCV version: {cv2.__version__}")
    
    # Check display-related environment variables
    print("\n=== Display Environment Variables ===")
    display_vars = [
        'DISPLAY',
        'XDG_SESSION_TYPE',
        'SSH_CONNECTION',
        'SSH_CLIENT',
        'TERM',
        'SSH_TTY',
        'PWD',
        'HOME'
    ]
    
    for var in display_vars:
        value = os.environ.get(var)
        if value:
            print(f"{var}: {value}")
        else:
            print(f"{var}: Not set")
    
    # Check if we're in a headless environment
    print("\n=== Headless Environment Detection ===")
    
    is_headless = False
    reasons = []
    
    if os.environ.get('DISPLAY') is None:
        is_headless = True
        reasons.append("DISPLAY environment variable not set")
    
    if os.environ.get('XDG_SESSION_TYPE') == 'tty':
        is_headless = True
        reasons.append("XDG_SESSION_TYPE is 'tty'")
    
    if os.environ.get('SSH_CONNECTION') is not None:
        is_headless = True
        reasons.append("SSH connection detected")
    
    if is_headless:
        print("❌ HEADLESS ENVIRONMENT DETECTED")
        print("Reasons:")
        for reason in reasons:
            print(f"  - {reason}")
        print("\nRecommendation: Use ImageDisplay(headless=True) or let it auto-detect")
    else:
        print("✅ GUI environment detected")
    
    # Test OpenCV window creation
    print("\n=== OpenCV Window Creation Test ===")
    try:
        cv2.namedWindow("test_window", cv2.WINDOW_AUTOSIZE)
        cv2.destroyWindow("test_window")
        print("✅ OpenCV window creation successful")
    except Exception as e:
        print(f"❌ OpenCV window creation failed: {e}")
        print("This confirms you need to use headless mode")
    
    # Check if we're in a virtual environment
    print("\n=== Virtual Environment Check ===")
    if hasattr(sys, 'real_prefix') or (hasattr(sys, 'base_prefix') and sys.base_prefix != sys.prefix):
        print("✅ Running in virtual environment")
        print(f"  Base prefix: {getattr(sys, 'base_prefix', 'N/A')}")
        print(f"  Current prefix: {sys.prefix}")
    else:
        print("❌ Not running in virtual environment")
    
    # Check current working directory
    print(f"\n=== Current Working Directory ===")
    print(f"PWD: {os.getcwd()}")
    
    print("\n=== End of Environment Check ===")


if __name__ == "__main__":
    check_environment()

