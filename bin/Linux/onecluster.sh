#!/bin/bash
#
# Options:

DATA_DIR=./cluster

# copy binaries and common config files 
mkdir $DATA_DIR
mkdir $DATA_DIR/daemon
cp $DSN_ROOT/bin/dsn.svchost $DATA_DIR/daemon/
cp $DSN_ROOT/lib/*.so $DATA_DIR/daemon/
cp $DSN_ROOT/bin/config.common.ini $DATA_DIR/daemon/
cp $DSN_ROOT/bin/config.onecluster.ini $DATA_DIR/daemon/

mkdir $DATA_DIR/meta
cp $DATA_DIR/daemon/* $DATA_DIR/meta/

# start meta server and daemon servers
pushd $DATA_DIR/meta
./dsn.svchost config.onecluster.ini -app_list meta &
echo $!>pid
popd

pushd $DATA_DIR/daemon
for i in $(seq 1 8);
do
    port=$(($i+24801))
    echo start daemon at port $port ...
    mkdir d$port
    pushd d$port
    ../dsn.svchost ../config.onecluster.ini -app_list daemon -cargs daemon_port=$port &
    echo $!>pid
    popd
done
popd

# START web studio
python $DSN_ROOT/webstudio/rDSN.WebStudio.py &

echo Now you can visit http://localhost:8088 to start playing with rDSN ...

