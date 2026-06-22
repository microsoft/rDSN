#!/usr/bin/env bash
# !!! This script should be run in dsn project root directory (../../).
#
# Shell Options:
#    RUN_VERBOSE    YES|NO
#    ENABLE_GCOV    YES|NO
#    TEST_MODULE    "<module1> <module2> ..."

ROOT=`pwd`
BUILD_DIR="${DSN_BUILD_DIR:-$ROOT/builder}"
case "$BUILD_DIR" in
    /*) ;;
    *) BUILD_DIR="$ROOT/$BUILD_DIR" ;;
esac
REPORT_DIR=$BUILD_DIR/test_reports
TEST_TMP_DIR="$BUILD_DIR/test_tmp"
export DSN_TEST_TMP_DIR="$TEST_TMP_DIR"
GCOV_DIR="$ROOT/gcov_report"
GCOV_TMP="$ROOT/.gcov_tmp"
GCOV_PATTERN=`find $ROOT/include $ROOT/src -name '*.h' -o -name '*.cpp'`
TIME=`date '+%Y-%m-%d %H:%M:%S%z'`
CMAKE_C_COMPILER="${CC:-cc}"
CMAKE_CXX_COMPILER="${CXX:-c++}"
CMAKE_OPTIONS="$CMAKE_OPTIONS -DCMAKE_C_COMPILER=$CMAKE_C_COMPILER -DCMAKE_CXX_COMPILER=$CMAKE_CXX_COMPILER"
MAKE_OPTIONS="$MAKE_OPTIONS -j$JOB_NUM"

CBIN_DIR=$(dirname "$0")
TOP_DIR=$CBIN_DIR/../..
source "$CBIN_DIR/download.sh"

if [ "$RUN_VERBOSE" == "YES" ]
then
    echo "RUN_VERBOSE=YES"
else
    echo "RUN_VERBOSE=NO"
fi

if [ "$ENABLE_GCOV" == "YES" ]
then
    echo "ENABLE_GCOV=YES"
else
    echo "ENABLE_GCOV=NO"
fi

# rDSN now uses the thrift compiler built from ext/thrift and installed under
# ${DSN_BUILD_DIR}/output/bin/${CMAKE_SYSTEM_NAME}. Keep the old prebuilt-binary download
# path disabled below for easy rollback if needed.
if false
then
    if [ ! -f "$TOP_DIR/bin/Linux/thrift" ]
    then
        echo "Downloading thrift..."
        download_file https://github.com/linmajia/thrift/raw/master/pre-built/ubuntu14.04/thrift thrift
        if [ $? -ne 0 ]
        then
            echo "ERROR: download thrift failed"
            rm -f thrift
            exit -1
        fi
        chmod u+x thrift
        mv thrift "$TOP_DIR/bin/Linux"
    fi
fi

echo "########################### TEST START ############################################"

if [ "$ENABLE_GCOV" == "YES" ]
then
    echo "Initializing gcov..."
    cd $ROOT
    rm -rf $GCOV_TMP &>/dev/null
    mkdir -p $GCOV_TMP
    lcov -q -d $BUILD_DIR -z
    lcov -q -d $BUILD_DIR -b $ROOT --no-external --initial -c -o $GCOV_TMP/initial.info
    if [ $? -ne 0 ]
    then
        echo "ERROR: lcov init failed, maybe need to run again with --clear option"
        exit -1
    fi
    lcov -q -e $GCOV_TMP/initial.info $GCOV_PATTERN -o $GCOV_TMP/initial.extract.info
    if [ $? -ne 0 ]
    then
        echo "ERROR: lcov init extract failed"
        exit -1
    fi
fi

if [ ! -d "$REPORT_DIR" ]
then
    mkdir -p $REPORT_DIR
fi
rm -rf "$TEST_TMP_DIR"
mkdir -p "$TEST_TMP_DIR"

### TODO: add test module filtering 

##### unit tests #######
if [ -f "$BUILD_DIR/bin/dsn.svchost/dsn.svchost" ]
then
    SVC_HOST=$BUILD_DIR/bin/dsn.svchost/dsn.svchost
else
    SVC_HOST=$DSN_ROOT/bin/dsn.svchost
fi 

for dir in $BUILD_DIR/test/*/
do
    echo $dir
    if [ -f "$dir/gtests" ]
    then
        pushd "$dir" >/dev/null
        while read -r line || [ -n "$line" ]; do
            case "$line" in
                ""|\#*) continue ;;
            esac
            echo "============ run unit tests in $dir with $line ============"
            rm -fr ./data core
            $SVC_HOST $dir/$line

            if [ $? -ne 0 ]; then
                echo "run unit tests in $dir with $line failed"
                echo "---- ls ----"
                ls -l
                if find . -name log.1.txt; then
                    echo "---- tail -n 100 log.1.txt ----"
                    tail -n 100 `find . -name log.1.txt`
                fi
                if [ -f core ]; then
                    echo "---- gdb $SVC_HOST core ----"
                    gdb $SVC_HOST core -ex "thread apply all bt" -ex "set pagination 0" -batch
                fi
                popd >/dev/null
                exit -1
            fi
        done < "$dir/gtests"
        popd >/dev/null
    fi    
done


##### test.sh #######
for dir in $BUILD_DIR/bin/*/
do
    echo $dir
    if [ -f "$dir/test.sh" ]
    then
        pushd "$dir" >/dev/null
        echo "============ run test.sh in $dir ============"
        rm -fr ./data core
        REPORT_DIR=$REPORT_DIR ./test.sh

        if [ $? -ne 0 ]; then
            echo "run test.sh in $dir failed"
            popd >/dev/null
            exit -1
        fi
        popd >/dev/null
    fi    
done

if [ "$ENABLE_GCOV" == "YES" ]
then
    echo "Generating gcov report..."
    cd $ROOT
    lcov -q -d $BUILD_DIR -b $ROOT --no-external -c -o $GCOV_TMP/gcov.info
    if [ $? -ne 0 ]
    then
        echo "ERROR: lcov generate failed"
        exit -1
    fi
    lcov -q -e $GCOV_TMP/gcov.info $GCOV_PATTERN -o $GCOV_TMP/gcov.extract.info
    if [ $? -ne 0 ]
    then
        echo "ERROR: lcov extract failed"
        exit -1
    fi
    genhtml $GCOV_TMP/*.extract.info --show-details --legend --title "GCOV report at $TIME" -o $GCOV_TMP/report
    if [ $? -ne 0 ]
    then
        echo "ERROR: gcov genhtml failed"
        exit -1
    fi
    rm -rf $GCOV_DIR &>/dev/null
    mv $GCOV_TMP/report $GCOV_DIR
    rm -rf $GCOV_TMP
    echo "View gcov report: firefox $GCOV_DIR/index.html"
fi

echo "Test succeed"
