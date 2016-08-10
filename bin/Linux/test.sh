#!/bin/bash
# !!! This script should be run in dsn project root directory (../../).
#
# Shell Options:
#    RUN_VERBOSE    YES|NO
#    ENABLE_GCOV    YES|NO
#    TEST_MODULE    "<module1> <module2> ..."

ROOT=`pwd`
REPORT_DIR=$ROOT/test_reports
BUILD_DIR="$ROOT/builder"
GCOV_DIR="$ROOT/gcov_report"
GCOV_TMP="$ROOT/.gcov_tmp"
GCOV_PATTERN=`find $ROOT/include $ROOT/src -name '*.h' -o -name '*.cpp'`
TIME=`date --rfc-3339=seconds`
CMAKE_OPTIONS="$CMAKE_OPTIONS -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++"
MAKE_OPTIONS="$MAKE_OPTIONS -j$JOB_NUM"

CBIN_DIR=$(dirname "$0")
TOP_DIR=$CBIN_DIR/../..

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

if [ ! -f "$TOP_DIR/bin/Linux/thrift" ]
then
    echo "Downloading thrift..."
    if [ "$GIT_SOURCE" == "xiaomi" ]
    then
        wget http://git.n.xiaomi.com/pegasus/packages/raw/master/rdsn/thrift
    else
        wget --no-check-certificate https://github.com/imzhenyu/thrift/raw/master/pre-built/ubuntu14.04/thrift
    fi
    chmod u+x thrift
    mv thrift $TOP_DIR/bin/Linux
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

### TODO: add test module filtering 

##### unit tests #######
for dir in $BUILD_DIR/test/*/
do
    echo $dir
    if [ -f "$dir/gtests" ]
    then
        pushd $dir
        cat $dir/gtests | while read -r line || [ -n "$line" ]; do
            echo "============ run unit tests in $dir with $line ============"
            rm -fr ./data
            $BUILD_DIR/bin/dsn.svchost/dsn.svchost $dir/$line

            if [ $? -ne 0 ]; then
                echo "run unit tests in $dir with $line failed"
                echo "---- ls ----"
                ls -l
                if find . -name log.1.txt; then
                    echo "---- tail -n 100 log.1.txt ----"
                    tail -n 100 `find . -name log.1.txt`
                fi
                if [ -f core ]; then
                    echo "---- gdb $BUILD_DIR/bin/dsn.svchost/dsn.svchost core ----"
                    gdb $BUILD_DIR/bin/dsn.svchost/dsn.svchost core -ex "thread apply all bt" -ex "set pagination 0" -batch
                fi
                popd
                exit -1
            fi
        done
        popd
    fi    
done


##### test.sh #######
for dir in $BUILD_DIR/bin/*/
do
    echo $dir
    if [ -f "$dir/test.sh" ]
    then
        pushd $dir
        echo "============ run test.sh in $dir ============"
        REPORT_DIR=$REPORT_DIR ./test.sh

        if [ $? -ne 0 ]; then
            echo "run test.sh in $dir failed"
            popd
            exit -1
        fi
        popd
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

