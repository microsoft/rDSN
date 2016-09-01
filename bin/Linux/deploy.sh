#!/bin/bash
os=linux
scripts_path=`readlink -f $0`
scripts_dir=`dirname $scripts_path`

function usage() {
    echo "Option for subcommand 'deploy|start|stop|clean|scds(stop-clean-deploy-start)"
    echo " -s|--source-dir <dir> local source directory for deployment containing start.sh, stop.sh, machines.txt and other resources, or a directory containing an apps.txt each line spcifying a sub-directory above (inside this directory"
    echo " -t|--target <dir>    remote target directory for deployment"
}

CMD=$1
shift

while [ $# -gt 0 ];do
#TODO: this may cause infinit loop when parameters are invalid
    key=$1
    case $key in
        -h|--help)
            usage
            exit 0
            ;;
        -s|--source-dir)
            s_dir=$2
            shift 2
            ;;
        -t|--target-dir)
            t_dir=$2
            shift 2
            ;;
        *)
            echo "ERROR: unknown option $key"
            echo
            usage
            exit -1
            ;;
    esac

done

if [ -z $s_dir ] || [ -z $t_dir ];then
    usage
    exit -1
fi

if [ ! -d $s_dir ];then
    echo "$s_dir no such directory"
    exit -1
fi


#$1 machine $2 app $3 sdir $4 tdir
function deploy_files(){

    echo "deploy $2 at $3 to $4 at $1"
    ssh $1 "mkdir -p '${4}'"
    scp $3/* "${1}:'${4}'"
}


#$1 machine $2 app $3 sdir $4 tdir
function clean_files(){
    echo "cleaning $2 at $1"
    ssh $1 'rm -fr '${4}''
}

#$1 machine $2 app $3 sdir $4 tdir
function start_server(){
    echo "starting $2 at $1"

    ssh $1 'cd '${4}';nohup sh -c "(( ./start.sh >foo.out 2>foo.err </dev/null)&)"'
}

#$1 machine $2 app $3 sdir $4 tdir
function stop_server(){
    echo "stopping $2 at $1"
    ssh $1 'cd '${4}';nohup sh -c "(( ./stop.sh >foo.out 2>foo.err </dev/null)&)"'
}

#$1 cmd $2 app $3 sdir $4 tdir
function run_one()
{    
    machines=$(cat $3/machines.txt)
    for mac in $machines;do
        $1 $mac $2 $3 $4
    done
}

#$1 cmd
function run_()
{
    if [ -f ${s_dir}/apps.txt ]; then
        applist=$(cat ${s_dir}/apps.txt)
        for app in $applist;do
            run_one $1 $app ${s_dir}/$app ${t_dir}/$app
        done        
    else
        run_one $1 $(basename "$s_dir") $s_dir $t_dir
    fi
}

case $CMD in
    start)
        run_ "start_server"
        ;;
    stop)
        run_ "stop_server"
        ;;
    deploy)
        run_ "deploy_files"
        ;;
    clean)
        run_ "stop_server"
        run_ "clean_files"
        ;;
    scds)
        run_ "stop_server"
        run_ "clean_files"
        run_ "deploy_files"
        run_ "start_server"
        ;;
    *)
        echo "Bug shouldn't come here"
        echo
        ;;
esac

