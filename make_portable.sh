#!/bin/bash

APP_NAME="video_splitter.exe"
BUILD_DIR="builddir"
PORTABLE_DIR="video_splitter_portable"

echo "Building portable app for $APP_NAME..."

# Clean and create portable directory
rm -rf "$PORTABLE_DIR"
mkdir -p "$PORTABLE_DIR"

# Build the project
echo "Building project..."
meson compile -C "$BUILD_DIR"

# Copy executable
cp "$BUILD_DIR/src/$APP_NAME" "$PORTABLE_DIR/"

# Copy DLL dependencies
echo "Copying DLL dependencies..."
ldd "$BUILD_DIR/src/$APP_NAME" | grep mingw64 | awk '{print $3}' | while read dll; do
    if [ -f "$dll" ]; then
        cp "$dll" "$PORTABLE_DIR/"
        echo "Copied: $(basename $dll)"
    fi
done

# Copy GTK data
echo "Copying GTK3 data files..."
mkdir -p "$PORTABLE_DIR/share"/{gtk-3.0,glib-2.0/schemas,icons,themes}

cp -r /mingw64/share/gtk-3.0/* "$PORTABLE_DIR/share/gtk-3.0/" 2>/dev/null || true
cp -r /mingw64/share/glib-2.0/schemas/* "$PORTABLE_DIR/share/glib-2.0/schemas/" 2>/dev/null || true
cp -r /mingw64/share/icons/Adwaita "$PORTABLE_DIR/share/icons/" 2>/dev/null || true
cp -r /mingw64/share/icons/hicolor "$PORTABLE_DIR/share/icons/" 2>/dev/null || true

# Compile schemas
cd "$PORTABLE_DIR/share/glib-2.0/schemas"
glib-compile-schemas . 2>/dev/null || true
cd - > /dev/null

# Create settings
cat > "$PORTABLE_DIR/settings.ini" << 'EOF'
[Settings]
gtk-theme-name=Windows10
gtk-icon-theme-name=Adwaita
gtk-font-name=Segoe UI 10
EOF

# Create launcher
cat > "$PORTABLE_DIR/run_app.bat" << EOF
@echo off
set GTK_DATA_PREFIX=%~dp0
set GSETTINGS_SCHEMA_DIR=%~dp0\\share\\glib-2.0\\schemas
"%~dp0\\$APP_NAME.exe" %*
EOF

echo "Portable app created in $PORTABLE_DIR/"
echo "You can now copy this folder to any Windows machine!"
