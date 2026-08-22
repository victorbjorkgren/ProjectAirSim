# Headless Mode for ImageDisplay

This document explains how to use the headless mode feature in ProjectAirSim's `ImageDisplay` class to avoid OpenCV display errors when running in environments without display access.

## Problem

When running ProjectAirSim scripts in certain environments, you may encounter this error:

```
cv2.error: Unknown C++ exception from OpenCV code
```

This typically occurs when:
- Running on a headless server
- Connected via SSH without X11 forwarding
- Running in a container without display access
- Running in certain CI/CD environments

## Solution

The `ImageDisplay` class now supports headless mode, which disables GUI windows and prevents OpenCV display errors.

## Usage

### Option 1: Auto-detect Headless Environment (Recommended)

```python
from projectairsim.image_utils import ImageDisplay

# The class will automatically detect if you're in a headless environment
image_display = ImageDisplay(headless=False)  # Will auto-detect
```

### Option 2: Force Headless Mode

```python
from projectairsim.image_utils import ImageDisplay

# Force headless mode
image_display = ImageDisplay(headless=True)
```

### Option 3: Check and Change Mode

```python
from projectairsim.image_utils import ImageDisplay

image_display = ImageDisplay()

# Check current mode
if image_display.is_headless():
    print("Running in headless mode")
else:
    print("Running with GUI windows")

# Change mode if needed
image_display.set_headless(True)  # Force headless
image_display.set_headless(False) # Enable GUI
```

## How It Works

When headless mode is enabled:

1. **No GUI windows are created** - `cv2.namedWindow()` calls are skipped
2. **Images are still processed** - Camera data is received and processed
3. **No display errors** - OpenCV operations that require display are safely skipped
4. **Performance maintained** - The display loop continues running without interruption

## Environment Detection

The class automatically detects headless environments by checking:

- `DISPLAY` environment variable (missing = headless)
- `XDG_SESSION_TYPE` (value 'tty' = headless)
- `SSH_CONNECTION` (present = likely headless)

## Example Scripts

### Basic Headless Usage

```python
from projectairsim.image_utils import ImageDisplay

# Initialize with headless mode
image_display = ImageDisplay(headless=True)

# Add camera feeds (will work without GUI)
image_display.add_image("RGB", subwin_idx=0)
image_display.add_image("Depth", subwin_idx=1)

# Start display (runs in headless mode)
image_display.start()

# Your drone control code here...

# Stop display
image_display.stop()
```

### Environment Check Script

Run `check_environment.py` to diagnose your environment:

```bash
python check_environment.py
```

This will show:
- Environment variables
- Headless detection results
- OpenCV window creation test
- Recommendations

## Migration from Existing Code

If you have existing code using `ImageDisplay`, you can easily add headless support:

**Before:**
```python
image_display = ImageDisplay()
```

**After:**
```python
# Option 1: Auto-detect (recommended)
image_display = ImageDisplay(headless=False)

# Option 2: Force headless
image_display = ImageDisplay(headless=True)
```

## Troubleshooting

### Still Getting Errors?

1. **Check your environment:**
   ```bash
   python check_environment.py
   ```

2. **Force headless mode:**
   ```python
   image_display = ImageDisplay(headless=True)
   ```

3. **Check for other OpenCV calls:**
   - Look for `cv2.imshow()`, `cv2.waitKey()` in your code
   - These are now protected in the `ImageDisplay` class

### Performance Issues?

- Headless mode actually improves performance by skipping GUI operations
- Camera data is still processed and available
- No impact on drone control or simulation

## Advanced Usage

### Conditional Headless Mode

```python
import os
from projectairsim.image_utils import ImageDisplay

# Check if running in CI/CD environment
is_ci = os.environ.get('CI') == 'true'

# Use headless mode in CI, auto-detect otherwise
image_display = ImageDisplay(headless=is_ci)
```

### Runtime Mode Switching

```python
image_display = ImageDisplay()

# Start with GUI
image_display.start()

# Switch to headless if needed
if some_condition:
    image_display.set_headless(True)
    print("Switched to headless mode")

# Switch back to GUI
image_display.set_headless(False)
```

## Support

If you continue to experience issues:

1. Run the environment check script
2. Check the ProjectAirSim logs for detailed error messages
3. Ensure you're using the latest version of ProjectAirSim
4. Consider filing an issue with your environment details

## Related Files

- `client/python/projectairsim/src/projectairsim/image_utils.py` - Main implementation
- `client/python/example_user_scripts/headless_test.py` - Example usage
- `client/python/example_user_scripts/check_environment.py` - Environment diagnostics

