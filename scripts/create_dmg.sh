#!/bin/bash

# Script to create a macOS DMG for distribution
# This creates a disk image that users can download and drag to Applications

set -e

PROJECT_NAME="LumosWorkspace"
VERSION="1.0.0"
BUILD_DIR="build"
APP_NAME="LumosWorkspace"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}Creating macOS DMG for ${PROJECT_NAME} v${VERSION}${NC}"

# Check if app bundle exists
APP_BUNDLE="${BUILD_DIR}/src/applications/repl_gui/repl_gui_modular.app"
if [ ! -d "$APP_BUNDLE" ]; then
    echo -e "${RED}Error: App bundle not found at $APP_BUNDLE${NC}"
    echo "Please build the project first with: cd build && make -j6"
    exit 1
fi

# Create temporary DMG directory
TEMP_DMG_DIR=$(mktemp -d)
DMG_DIR="${TEMP_DMG_DIR}/${PROJECT_NAME}"
mkdir -p "$DMG_DIR"

echo -e "${BLUE}Copying application bundle...${NC}"
cp -R "$APP_BUNDLE" "${DMG_DIR}/${APP_NAME}.app"

# Create Applications symlink for easy installation
echo -e "${BLUE}Creating Applications symlink...${NC}"
ln -s /Applications "${DMG_DIR}/Applications"

# Create a simple README for users
cat > "${DMG_DIR}/README.txt" << EOF
${PROJECT_NAME} v${VERSION}

Installation:
1. Drag ${APP_NAME}.app to the Applications folder
2. Launch from Launchpad or Applications folder

Features:
- Interactive Python REPL with embedded interpreter
- Variable save/load with pickle files
- TCP debug API for external tools
- Cross-platform Qt6 interface

For more information, visit: https://github.com/your-repo/${PROJECT_NAME}

Copyright © 2025 LumosWorkspace. All rights reserved.
EOF

# Set DMG properties
DMG_NAME="${PROJECT_NAME}-${VERSION}"
DMG_FILE="${DMG_NAME}.dmg"

echo -e "${BLUE}Creating disk image...${NC}"

# Remove any existing DMG
rm -f "$DMG_FILE"

# Create DMG
if command -v hdiutil >/dev/null 2>&1; then
    # Create temporary DMG
    TEMP_DMG="${DMG_NAME}-temp.dmg"
    
    # Calculate size needed (app size + some padding)
    SIZE=$(du -sm "${DMG_DIR}" | cut -f1)
    SIZE=$((SIZE + 50)) # Add 50MB padding
    
    # Create the DMG
    hdiutil create -srcfolder "$DMG_DIR" -volname "$PROJECT_NAME" -fs HFS+ \
        -fsargs "-c c=64,a=16,e=16" -format UDZO -size ${SIZE}m "$DMG_FILE"
    
    echo -e "${GREEN}✅ DMG created successfully: $DMG_FILE${NC}"
    echo -e "${BLUE}Size: $(du -h "$DMG_FILE" | cut -f1)${NC}"
else
    echo -e "${RED}Warning: hdiutil not found. Creating simple zip archive instead.${NC}"
    
    # Fallback: create ZIP file
    ZIP_FILE="${DMG_NAME}.zip"
    rm -f "$ZIP_FILE"
    
    cd "${TEMP_DMG_DIR}"
    zip -r "$(pwd)/${ZIP_FILE}" "$PROJECT_NAME"
    mv "$ZIP_FILE" "$OLDPWD/"
    cd "$OLDPWD"
    
    echo -e "${GREEN}✅ ZIP archive created: $ZIP_FILE${NC}"
fi

# Cleanup
rm -rf "$TEMP_DMG_DIR"

echo -e "${GREEN}Distribution package ready!${NC}"
echo
echo -e "${BLUE}To test the DMG:${NC}"
echo "1. Double-click the DMG file"
echo "2. Drag ${APP_NAME}.app to Applications"
echo "3. Launch from Applications folder"
echo
echo -e "${BLUE}To distribute:${NC}"
echo "Upload the DMG file for users to download"