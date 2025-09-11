# macOS Application Bundle

LumosWorkspace now supports building as a proper macOS application bundle that can be installed in the Applications folder.

## Features

✅ **Native macOS App Bundle** - Proper `.app` structure  
✅ **Application Icon** - Custom icon in Finder and Dock  
✅ **Info.plist Metadata** - Version info, file associations  
✅ **Drag-and-Drop Installation** - Copy to Applications folder  
✅ **Spotlight Integration** - Searchable in Spotlight  
✅ **File Type Associations** - Handles `.py` and `.pickle` files  

## Building the App Bundle

```bash
# Standard build creates the app bundle automatically on macOS
cd build
make -j6

# The app bundle is created at:
# build/src/applications/repl_gui/repl_gui_modular.app
```

## Installation

### Method 1: Direct Copy
```bash
# Copy to Applications folder
cp -R build/src/applications/repl_gui/repl_gui_modular.app /Applications/LumosWorkspace.app
```

### Method 2: Create DMG for Distribution
```bash
# Create a distributable DMG file
./scripts/create_dmg.sh

# This creates: LumosWorkspace-1.0.0.dmg
# Users can download and drag to Applications
```

## App Bundle Structure

```
LumosWorkspace.app/
├── Contents/
│   ├── Info.plist          # Application metadata
│   ├── MacOS/
│   │   └── repl_gui_modular # Main executable  
│   └── Resources/
│       └── AppIcon.icns    # Application icon
```

## Application Properties

- **Bundle Name**: LumosWorkspace
- **Bundle ID**: com.lumosworkspace.app  
- **Version**: 1.0.0
- **Category**: Developer Tools
- **Minimum macOS**: 10.15 (Catalina)

## File Type Support

The app registers to handle:
- **Python files** (`.py`) - Alternative handler
- **Pickle files** (`.pickle`) - Alternative handler

## Customization

### Custom Icon
1. Replace `platform/macos/AppIcon.icns` with your icon
2. Or modify `platform/macos/create_icon.sh` to generate from artwork
3. Rebuild the project

### App Metadata
1. Edit `platform/macos/Info.plist.in`
2. Update CMake variables in `src/applications/repl_gui/CMakeLists.txt`
3. Rebuild the project

### Code Signing (Optional)
Uncomment and configure in `CMakeLists.txt`:
```cmake
set_target_properties(repl_gui_modular PROPERTIES
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Developer ID Application: Your Name"
    XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "YOUR_TEAM_ID"
)
```

## Distribution

1. **DMG File**: Use `scripts/create_dmg.sh` to create distributable disk image
2. **Direct Download**: Provide the `.app` bundle in a ZIP file  
3. **Mac App Store**: Requires additional configuration and signing

## Launch Methods

After installation, LumosWorkspace can be launched via:
- **Applications folder** - Double-click LumosWorkspace.app
- **Launchpad** - Search for "LumosWorkspace"  
- **Spotlight** - Cmd+Space, type "LumosWorkspace"
- **Finder** - Right-click `.py` files → Open With → LumosWorkspace

The macOS app bundle provides a professional, native experience for Mac users while maintaining all the functionality of the command-line version.