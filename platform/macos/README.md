# macOS Platform Files

This directory contains macOS-specific files for creating a proper application bundle.

## Files

### `Info.plist.in`
Template for the application's Info.plist file. This contains:
- Application metadata (name, version, identifier)
- Icon file reference
- Document type associations (Python files, pickle files)
- System requirements (macOS 10.15+)
- Privacy usage descriptions

### `AppIcon.icns`
Application icon in Apple's ICNS format containing multiple resolutions:
- 16x16, 32x32, 64x64, 128x128, 256x256, 512x512, 1024x1024
- Both standard and @2x (Retina) versions

### `create_icon.sh`
Script to generate the application icon from source materials. 
Currently uses a system icon as placeholder - replace with custom artwork.

## Application Bundle Structure

When built, creates:
```
LumosWorkspace.app/
├── Contents/
│   ├── Info.plist          # Generated from Info.plist.in
│   ├── MacOS/
│   │   └── LumosWorkspace  # Main executable
│   ├── Resources/
│   │   ├── AppIcon.icns    # Application icon
│   │   └── ...             # Other Qt resources
│   └── Frameworks/         # Qt frameworks (if not system)
```

## Usage

After building, the `.app` bundle can be:
- Copied to `/Applications/` folder
- Distributed via DMG
- Code signed and notarized for distribution

## Customization

To use a custom icon:
1. Create artwork at 1024x1024 resolution
2. Use `create_icon.sh` or manual iconutil process
3. Replace `AppIcon.icns`

To modify app metadata:
1. Edit `Info.plist.in` template
2. Update CMake variables if needed
3. Rebuild application