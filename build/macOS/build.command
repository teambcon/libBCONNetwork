#!/bin/bash
# DESCRIPTION: Builds and installs and libBCONNetwork.

# Move to the current working directory.
cd $(dirname "$0")

# Attempt to locate Qt.
QT="$(which qmake)"
if [ ! -x "${QT}" ] ; then
    # Not in the PATH, try the default configured path.
    QT=~/Qt/5.12.2/clang_64/bin

    # Check if the above Qt path exists before proceeding.
    if [ ! -d "${QT}" ]; then
        echo 'Failed to find Qt. Please ensure it is accessible in the PATH and try again.'
        exit 1
    else
        PATH=${QT}:${PATH}
    fi
fi

# Prepare for the build.
echo "Cleaning previous build artifacts..."
rm -rf ../../lib/macOS &> /dev/null
rm -f ../../.qmake.stash &> /dev/null

# Build and install the library.
pushd . &> /dev/null
cd ../..
if ! qmake libBCONNetwork.pro; then
    echo "Failed to create Makefile for libBCONNetwork!"
    exit 1
fi
if ! make clean install; then
    echo "Build failed for libBCONNetwork!"
    exit 1
fi

echo "Cleaning up..."
make clean
rm -f .qmake.stash &> /dev/null
rm -f libBCONNetwork.a &> /dev/null
rm -f Makefile &> /dev/null
popd &> /dev/null
