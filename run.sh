#!/bin/bash
local_path=`readlink -f $0`
local_dir=`dirname $local_path`
echo "run $0 in $local_dir"
$local_dir/bin/dsn.run.sh $1 $2 $3 $4 $5 $6 $7 $8 $9

