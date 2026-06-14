#!/bin/bash
#
# Options:
#    INSTALL_DIR    <dir>
#    PORT           <port>

if [ -z "$INSTALL_DIR" ]
then
    echo "ERROR: no INSTALL_DIR specified"
    exit -1
fi

if [ -z "$PORT" ]
then
    echo "ERROR: no PORT specified"
    exit -1
fi

function download_file()
{
    local url="$1"
    local output="$2"

    if command -v wget >/dev/null 2>&1
    then
        wget --no-check-certificate -nv -O "$output" "$url"
    elif command -v curl >/dev/null 2>&1
    then
        curl -fL --retry 3 --insecure -o "$output" "$url"
    else
        echo "ERROR: neither wget nor curl is installed"
        return 1
    fi
}

function update_config()
{
    local expression="$1"
    local file="$2"
    local tmp_file="${file}.tmp"

    sed "$expression" "$file" > "$tmp_file" && mv "$tmp_file" "$file"
}

function show_zookeeper_log()
{
    local log_file="$INSTALL_DIR/zookeeper.out"

    if [ -f "$log_file" ]; then
        echo "Zookeeper output:"
        tail -n 40 "$log_file"
    fi
}

if [ -z "$GIT_SOURCE" -o "$GIT_SOURCE" == "github" ]
then
    download_url="https://raw.githubusercontent.com/linmajia/packages/master/common/zookeeper-3.4.6.tar.gz"
else
    echo "ERROR: invalid git source '$GIT_SOURCE', should be github"
    exit -1
fi

mkdir -p "$INSTALL_DIR"
if [ $? -ne 0 ]
then
    echo "ERROR: mkdir $INSTALL_DIR failed"
    exit -1
fi

cd "$INSTALL_DIR"

archive_file="zookeeper-3.4.6.tar.gz"
zookeeper_dir="zookeeper-3.4.6"

if [ -f "$archive_file" ] && ! tar tf "$archive_file" >/dev/null 2>&1; then
    echo "Removing invalid zookeeper archive..."
    rm -f "$archive_file"
fi

if [ ! -f "$archive_file" ]; then
    echo "Downloading zookeeper..."
    download_file "$download_url" "$archive_file.tmp"
    if [ $? -ne 0 ]; then
        echo "ERROR: download zookeeper failed"
        rm -f "$archive_file.tmp"
        exit -1
    fi
    if ! tar tf "$archive_file.tmp" >/dev/null 2>&1; then
        echo "ERROR: downloaded zookeeper archive is invalid"
        rm -f "$archive_file.tmp"
        exit -1
    fi
    mv "$archive_file.tmp" "$archive_file"
fi

if [ ! -d "$zookeeper_dir" ]; then
    echo "Decompressing zookeeper..."
    tar xfz "$archive_file"
    if [ $? -ne 0 ]; then
        echo "ERROR: decompress zookeeper failed"
        rm -rf "$zookeeper_dir"
        exit -1
    fi
fi

ZOOKEEPER_HOME=`pwd`/$zookeeper_dir
ZOOKEEPER_PORT=$PORT

if ! java -version >/dev/null 2>&1; then
    echo "ERROR: Java runtime is required to start zookeeper"
    exit -1
fi

cp "$ZOOKEEPER_HOME/conf/zoo_sample.cfg" "$ZOOKEEPER_HOME/conf/zoo.cfg"
update_config "s@dataDir=/tmp/zookeeper@dataDir=$ZOOKEEPER_HOME/data@" "$ZOOKEEPER_HOME/conf/zoo.cfg"
update_config "s@clientPort=2181@clientPort=$ZOOKEEPER_PORT@" "$ZOOKEEPER_HOME/conf/zoo.cfg"

mkdir -p "$ZOOKEEPER_HOME/data"
"$ZOOKEEPER_HOME/bin/zkServer.sh" start

sleep 2
if printf ruok | nc -w 5 localhost "$ZOOKEEPER_PORT" | grep -q imok; then
    echo "Zookeeper started at port $ZOOKEEPER_PORT"
    exit 0
else
    echo "ERROR: start zookeeper failed"
    show_zookeeper_log
    exit -1
fi
 
