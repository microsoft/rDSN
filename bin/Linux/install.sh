#!/usr/bin/env bash
# !!! This script should be run in dsn project root directory (../../).
#
# Options:
#    INSTALL_DIR    <dir>

if [ -z "$INSTALL_DIR" ]
then
    echo "ERROR: no INSTALL_DIR specified"
    exit -1
fi

ROOT=`pwd`
BUILD_DIR="${DSN_BUILD_DIR:-$ROOT/builder}"
case "$BUILD_DIR" in
    /*) ;;
    *) BUILD_DIR="$ROOT/$BUILD_DIR" ;;
esac

if [ ! -d "$BUILD_DIR/output" ]
then
    echo "ERROR: not build yet"
    exit -1
fi

mkdir -p $INSTALL_DIR
if [ $? -ne 0 ]
then
    echo "ERROR: mkdir $INSTALL_DIR failed"
    exit -1
fi
INSTALL_DIR=`cd $INSTALL_DIR; pwd`
echo "INSTALL_DIR=$INSTALL_DIR"

echo "Copying files..."

cp -r -v "$BUILD_DIR"/output/* $INSTALL_DIR

if [ -f "$BUILD_DIR/bin/dsn.svchost/dsn.svchost" ]
then
    cp "$BUILD_DIR/bin/dsn.svchost/dsn.svchost" $INSTALL_DIR/bin
    echo "Install succeed"    
    if [ -z "$DSN_ROOT" -o "$DSN_ROOT" != "$INSTALL_DIR" ]
    then
        if ! grep -q '^export DSN_ROOT=' ~/.bashrc
        then
            echo "export DSN_ROOT=$INSTALL_DIR" >>~/.bashrc
        else
            sed -i "s@^export DSN_ROOT=.*@export DSN_ROOT=$INSTALL_DIR@" ~/.bashrc
        fi
        if ! grep -q '^export PATH=.*DSN_ROOT' ~/.bashrc
        then
            echo 'export PATH=$DSN_ROOT/bin:$PATH' >>~/.bashrc
        fi
        if ! grep -q '^export LD_LIBRARY_PATH=.*DSN_ROOT' ~/.bashrc
        then
            echo 'export LD_LIBRARY_PATH=$DSN_ROOT/lib:$LD_LIBRARY_PATH' >>~/.bashrc
        fi
        echo "====== ENVIRONMENT VARIABLE \$DSN_ROOT SET OR CHANGED, please run 'source ~/.bashrc' ======"
    fi
else
    echo "Install succeed"
fi
