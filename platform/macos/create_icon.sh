#!/bin/bash

# Script to create a macOS application icon (.icns) from a source image
# This creates a basic icon using system tools

echo "Creating LumosWorkspace application icon..."

# Create a temporary directory for icon generation
TEMP_DIR="$(mktemp -d)"
ICON_SET="${TEMP_DIR}/AppIcon.iconset"
mkdir -p "$ICON_SET"

# Create a simple colored square as base icon (you can replace this with actual artwork)
# This uses system tools to create a basic icon
cat > "${TEMP_DIR}/create_base.py" << 'EOF'
import sys
from PIL import Image, ImageDraw, ImageFont
import os

def create_icon(size):
    # Create a gradient background
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    # Create gradient background
    for i in range(size):
        color = (30 + i//4, 80 + i//3, 150 + i//2, 255)
        draw.rectangle([i, 0, i+1, size], fill=color)
    
    # Add rounded corners
    mask = Image.new('L', (size, size), 0)
    mask_draw = ImageDraw.Draw(mask)
    corner_radius = size // 8
    mask_draw.rounded_rectangle([0, 0, size, size], corner_radius, fill=255)
    
    # Apply mask
    img.putalpha(mask)
    
    # Add "LW" text
    try:
        font_size = size // 3
        font = ImageFont.truetype("/System/Library/Fonts/Helvetica.ttc", font_size)
        text = "LW"
        bbox = draw.textbbox((0, 0), text, font=font)
        text_width = bbox[2] - bbox[0]
        text_height = bbox[3] - bbox[1]
        x = (size - text_width) // 2
        y = (size - text_height) // 2 - font_size // 8
        draw.text((x, y), text, fill=(255, 255, 255, 255), font=font)
    except:
        # Fallback if font not available
        draw.text((size//3, size//3), "LW", fill=(255, 255, 255, 255))
    
    return img

# Icon sizes required for macOS
sizes = [16, 32, 64, 128, 256, 512, 1024]

for size in sizes:
    img = create_icon(size)
    if size == 32 or size == 64:
        img.save(f"AppIcon.iconset/icon_{size}x{size}.png")
    img.save(f"AppIcon.iconset/icon_{size}x{size}@2x.png" if size <= 512 else f"AppIcon.iconset/icon_{size//2}x{size//2}@2x.png")

print("Base icon images created")
EOF

# Check if PIL is available, if not use a simpler approach
if python3 -c "import PIL" 2>/dev/null; then
    echo "Using PIL to create icon..."
    cd "$TEMP_DIR"
    python3 create_base.py
else
    echo "PIL not available, creating simple icon with system tools..."
    # Create a simple colored rectangle using system tools
    for size in 16 32 64 128 256 512 1024; do
        # Create simple colored square using sips (macOS built-in tool)
        /usr/bin/sips -s format png -s pixelsW $size -s pixelsH $size /System/Library/CoreServices/CoreTypes.bundle/Contents/Resources/ApplicationsFolderIcon.icns --out "${ICON_SET}/temp_${size}.png" 2>/dev/null || true
        
        # If that fails, create with built-in tools
        if [ ! -f "${ICON_SET}/temp_${size}.png" ]; then
            # Create a simple blue square
            cat > "${ICON_SET}/temp_${size}.svg" << EOF
<svg width="$size" height="$size" xmlns="http://www.w3.org/2000/svg">
  <defs>
    <linearGradient id="grad1" x1="0%" y1="0%" x2="100%" y2="100%">
      <stop offset="0%" style="stop-color:#2E5BFF;stop-opacity:1" />
      <stop offset="100%" style="stop-color:#4A90E2;stop-opacity:1" />
    </linearGradient>
  </defs>
  <rect width="$size" height="$size" rx="$((size/8))" fill="url(#grad1)"/>
  <text x="50%" y="50%" font-family="Arial" font-size="$((size/3))" fill="white" text-anchor="middle" dy=".3em">LW</text>
</svg>
EOF
            # Convert SVG to PNG if possible
            if command -v rsvg-convert >/dev/null 2>&1; then
                rsvg-convert -w $size -h $size "${ICON_SET}/temp_${size}.svg" -o "${ICON_SET}/icon_${size}x${size}.png"
            elif command -v convert >/dev/null 2>&1; then
                convert -size ${size}x${size} -background "#4A90E2" -fill white -gravity center -font Arial -pointsize $((size/4)) label:"LW" "${ICON_SET}/icon_${size}x${size}.png"
            else
                # Last resort: copy system icon and rename
                cp /System/Library/CoreServices/CoreTypes.bundle/Contents/Resources/ApplicationsFolderIcon.icns "${ICON_SET}/icon_${size}x${size}.png" 2>/dev/null || touch "${ICON_SET}/icon_${size}x${size}.png"
            fi
        else
            mv "${ICON_SET}/temp_${size}.png" "${ICON_SET}/icon_${size}x${size}.png"
        fi
    done
fi

# Create the .icns file using iconutil (macOS built-in tool)
if [ -d "$ICON_SET" ] && [ "$(ls -A $ICON_SET)" ]; then
    echo "Creating .icns file..."
    iconutil -c icns "$ICON_SET" -o "$(dirname "$0")/AppIcon.icns"
    echo "Icon created at $(dirname "$0")/AppIcon.icns"
else
    echo "Warning: Could not create icon images. Creating placeholder..."
    # Create a minimal placeholder .icns
    touch "$(dirname "$0")/AppIcon.icns"
fi

# Cleanup
rm -rf "$TEMP_DIR"

echo "Icon creation complete!"