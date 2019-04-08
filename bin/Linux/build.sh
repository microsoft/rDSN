#!/bin/bash
# !!! This script should be run in dsn project root directory (../../).
#
# Shell Options:
#    CLEAR          YES|NO
#    JOB_NUM        <num>
#    BUILD_TYPE     debug|release
#    GIT_SOURCE     github|xiaomi
#    RUN_VERBOSE    YES|NO
#    WARNING_ALL    YES|NO
#    ENABLE_GCOV    YES|NO
#    BUILD_PLUGINS  YES|NO
#    BOOST_DIR      <dir>|""
#
# CMake options:
#    -DCMAKE_C_COMPILER=gcc
#    -DCMAKE_CXX_COMPILER=g++
#    [-DCMAKE_BUILD_TYPE=Debug]
#    [-DDSN_GIT_SOURCE=github|xiaomi]
#    [-DWARNING_ALL=TRUE]
#    [-DENABLE_GCOV=TRUE]
#    [-DBoost_NO_BOOST_CMAKE=ON -DBOOST_ROOT=$BOOST_DIR -DBoost_NO_SYSTEM_PATHS=ON]

ROOT=`pwd`
REPORT_DIR=$ROOT/test_reports
BUILD_DIR="$ROOT/builder"
GCOV_DIR="$ROOT/gcov_report"
GCOV_TMP="$ROOT/.gcov_tmp"
GCOV_PATTERN=`find $ROOT/include $ROOT/src -name '*.h' -o -name '*.cpp'`
TIME=`date --rfc-3339=seconds`
CMAKE_OPTIONS="$CMAKE_OPTIONS -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++"
MAKE_OPTIONS="$MAKE_OPTIONS -j$JOB_NUM"

scripts_path=`readlink -f $0`
CBIN_DIR=`dirname $scripts_path`

TOP_DIR=$CBIN_DIR/../..

if [ "$CLEAR" == "YES" ]
then
    echo "CLEAR=YES"
else
    echo "CLEAR=NO"
fi

if [ "$BUILD_TYPE" == "debug" ]
then
    echo "BUILD_TYPE=debug"
    CMAKE_OPTIONS="$CMAKE_OPTIONS -DCMAKE_BUILD_TYPE=Debug"
else
    echo "BUILD_TYPE=release"
fi

echo "SERIALIZE_TYPE=$SERIALIZE_TYPE"
if [ -n "$SERIALIZE_TYPE" ]
then
    CMAKE_OPTIONS="$CMAKE_OPTIONS -DDSN_SERIALIZATION_TYPE=$SERIALIZE_TYPE"
fi

echo "GIT_SOURCE=$GIT_SOURCE"
if [ -n "$GIT_SOURCE" ]
then
    CMAKE_OPTIONS="$CMAKE_OPTIONS -DDSN_GIT_SOURCE=$GIT_SOURCE"
fi

if [ "$RUN_VERBOSE" == "YES" ]
then
    echo "RUN_VERBOSE=YES"
    MAKE_OPTIONS="$MAKE_OPTIONS VERBOSE=1"
else
    echo "RUN_VERBOSE=NO"
fi

if [ "$WARNING_ALL" == "YES" ]
then
    echo "WARNING_ALL=YES"
    CMAKE_OPTIONS="$CMAKE_OPTIONS -DWARNING_ALL=TRUE"
else
    echo "WARNING_ALL=NO"
fi

if [ "$ENABLE_GCOV" == "YES" ]
then
    echo "ENABLE_GCOV=YES"
    CMAKE_OPTIONS="$CMAKE_OPTIONS -DENABLE_GCOV=TRUE"
else
    echo "ENABLE_GCOV=NO"
fi

if [ "$BUILD_PLUGINS" == "YES" ]
then
    echo "BUILD_PLUGINS=YES"
    CMAKE_OPTIONS="$CMAKE_OPTIONS -DBUILD_PLUGINS=TRUE"
else
    echo "BUILD_PLUGINS=NO"
    CMAKE_OPTIONS="$CMAKE_OPTIONS -DBUILD_PLUGINS=FALSE"
fi


# You can specify customized boost by defining BOOST_DIR.
# Install boost like this:
#   wget http://downloads.sourceforge.net/project/boost/boost/1.54.0/boost_1_54_0.zip?r=&ts=1442891144&use_mirror=jaist
#   unzip -q boost_1_54_0.zip
#   cd boost_1_54_0
#   ./bootstrap.sh --with-libraries=system,filesystem --with-toolset=gcc
#   ./b2 toolset=gcc cxxflags="-std=c++11 -fPIC" -j8 -d0
#   ./b2 install --prefix=$DSN_ROOT -d0
# And set BOOST_DIR as:
#   export BOOST_DIR=/path/to/boost_1_54_0/output
if [ -n "$BOOST_DIR" ]
then
    echo "Use customized boost: $BOOST_DIR"
    CMAKE_OPTIONS="$CMAKE_OPTIONS -DBoost_NO_BOOST_CMAKE=ON -DBOOST_ROOT=$BOOST_DIR -DBoost_NO_SYSTEM_PATHS=ON"
else
    echo "Use system boost"
fi

echo "CMAKE_OPTIONS=$CMAKE_OPTIONS"
echo "MAKE_OPTIONS=$MAKE_OPTIONS"

if [ ! -f "$TOP_DIR/bin/Linux/thrift" ]
then
    echo "Downloading thrift..."
    if [ "$GIT_SOURCE" == "xiaomi" ]
    then
        wget http://git.n.xiaomi.com/pegasus/packages/raw/master/rdsn/thrift
    else
        wget --no-check-certificate -nv https://github.com/imzhenyu/thrift/raw/master/pre-built/ubuntu14.04/thrift
    fi
    chmod u+x thrift
    mv thrift $TOP_DIR/bin/Linux
fi

echo "############################ BUILD #################################################"
if [ "$BUILD_PLUGINS" == "YES" ]; then
    pushd $TOP_DIR/src/plugins_ext
    git submodule init
    git submodule update
    popd
fi

if [ -f $BUILD_DIR/CMAKE_OPTIONS ]
then
    LAST_OPTIONS=`cat $BUILD_DIR/CMAKE_OPTIONS`
    if [ "$CMAKE_OPTIONS" != "$LAST_OPTIONS" ]
    then
        echo "WARNING: CMAKE_OPTIONS has changed from last build, clear environment first"
        CLEAR=YES
    fi
fi

if [ "$CLEAR" == "YES" -a -d "$BUILD_DIR" ]
then
    echo "Clear builder..."
    rm -rf $BUILD_DIR
fi


if [ ! -d "$BUILD_DIR" ]
then
    echo "Running cmake..."
    mkdir -p $BUILD_DIR
    cd $BUILD_DIR
    echo "$CMAKE_OPTIONS" >CMAKE_OPTIONS
    cmake $ROOT -DCMAKE_INSTALL_PREFIX=$BUILD_DIR/output $CMAKE_OPTIONS
    if [ $? -ne 0 ]
    then
        echo "ERROR: cmake failed"
        exit -1
    fi
    cd ..
fi

cd $BUILD_DIR
echo "Building..."
make install $MAKE_OPTIONS
if [ $? -ne 0 ]
then
    echo "ERROR: build failed"
    exit -1
else
    echo "Build succeed"
fi
cd ..

exit 0


