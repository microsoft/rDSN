#!/bin/bash

function download_file()
{
    local url="$1"
    local output="$2"

    if [ -z "$url" -o -z "$output" ]
    then
        echo "ERROR: download_file requires URL and output path"
        return 1
    fi

    if ! command -v curl >/dev/null 2>&1
    then
        echo "ERROR: curl is required to download $url"
        return 1
    fi

    curl -fL --retry 3 --insecure -o "$output" "$url"
}

if [ "${BASH_SOURCE[0]}" == "$0" ]
then
    download_file "$@"
    exit $?
fi
