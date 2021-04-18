#!/bin/bash

NDNSIM_PATH="../ndnSIM"

EXAMPLES_PATH="/ns-3/src/ndnSIM/examples"
NFD_FW_PATH="/ns-3/src/ndnSIM/NFD/daemon/fw"

FILE_NAME="dweb_simulation"

rsync -r "../ndnSIM_modifications/" ${NDNSIM_PATH}"/ns-3/src/ndnSIM"
rsync -r "../sim_files/" ${NDNSIM_PATH}${EXAMPLES_PATH}
rsync -r "../topologies/" ${NDNSIM_PATH}${EXAMPLES_PATH}"/topologies"
mkdir ${NDNSIM_PATH}${EXAMPLES_PATH}/$FILE_NAME
rsync -r "../sim_files/custom-apps/" ${NDNSIM_PATH}${EXAMPLES_PATH}/$FILE_NAME

chown -R cian ${NDNSIM_PATH}${EXAMPLES_PATH}

cd $NDNSIM_PATH/ns-3

function run_test {
    LOG_DIR=""
    LOG_FILE=""
    STRATEGY=""

    exec &>/dev/tty
    echo "$1 cache budget $2 alpha: $3"

    if [ "$1" = "magic" ]; then
        LOG_FILE="magic_$4"
        STRATEGY="nfd::cs::popularity_priority_queue"
    elif [ "$1" = "popularity" ]; then
        LOG_FILE="popularity_$4"
        STRATEGY="nfd::cs::popularity_priority_queue"
    elif [ "$1" = "dweb_broadcast" ]; then
        LOG_FILE="dweb_broadcast_$4"
        STRATEGY="dweb_broadcast"
    else
        LOG_FILE="lru_$4"
        STRATEGY="nfd::cs::lru"
    fi

    LOG_FILE=${LOG_FILE//./_}
    LOG_DIR="/home/cian/Documents/GitHub/DWeb/scripts/logs/$LOG_FILE"

    if [ -d "$LOG_DIR" ]; then
        rm -r $LOG_DIR
    fi

    mkdir $LOG_DIR
    exec >> $LOG_DIR/$LOG_FILE.txt
    exec 2>&1
    chown -R cian $LOG_DIR

    echo "$1 cache budget $2"
    echo "Alpha: $3"

    if [ "$1" = "magic" ]; then
        time NS_LOG=DWebConsumer:DWebProducer:ndn-cxx.nfd.Magic:ndn-cxx.nfd.ContentStore ./waf --run=$FILE_NAME --command-template="gdb -ex 'run' -ex 'q' --args %s $STRATEGY 1 $2 $3"
    else    
        time NS_LOG=DWebConsumer:DWebProducer:ndn-cxx.nfd.Magic:ndn-cxx.nfd.ContentStore ./waf --run=$FILE_NAME --command-template="gdb -ex 'run' -ex 'q' --args %s $STRATEGY 0 $2 $3"
    fi
    

    cp "app-delays-trace.txt" $LOG_DIR
    cd /home/cian/Documents/GitHub/DWeb/scripts/
    python3 read_logs.py $LOG_FILE
    chown -R cian $LOG_DIR/$LOG_FILE.txt
    cd $NDNSIM_PATH/ns-3

}


if [ -z "$1" ]; 
then
    NS_LOG=DWebConsumer:DWebProducer:ndn-cxx.nfd.Magic:ndn-cxx.nfd.ContentStore:ndn-cxx.nfd.FibManager:ndn-cxx.nfd.Forwarder:ndn-cxx.nfd.Strategy ./waf --run=$FILE_NAME
elif [ "$1" = "-v" ];
then
    NS_LOG=DWebConsumer:DWebProducer:ndn-cxx.nfd.Magic:ndn-cxx.nfd.ContentStore ./waf --run=$FILE_NAME --vis
elif [ "$1" = "-d" ];
then
    ./waf --command-template="gdb %s" --run=$FILE_NAME 
elif [ "$1" = "-l" ];
then
    LOG_DIR="/home/cian/Documents/GitHub/DWeb/scripts/logs/$2"
    if [ -d "$LOG_DIR" ]; then
        rm -r $LOG_DIR
    fi

    mkdir $LOG_DIR
    exec >> $LOG_DIR/$2.txt
    exec 2>&1
    chown -R cian $LOG_DIR

    NS_LOG=DWebConsumer:DWebProducer:ndn-cxx.nfd.Magic:ndn-cxx.nfd.ContentStore:ndn-cxx.nfd.Forwarder  ./waf --run=$FILE_NAME
    
    cp "app-delays-trace.txt" $LOG_DIR
    
    cd /home/cian/Documents/GitHub/DWeb/scripts/
    python3 read_logs.py $2
    cd $NDNSIM_PATH/ns-3

elif [ "$1" = "-t" ];
then
    ./waf --run=create_topology
elif [ "$1" = "-run_test" ];
then
    for cache_buget in 0.01 0.02 0.03 0.04 0.05 0.06 0.07 0.08 0.09 0.1 0.15 0.2; do
        for strategy in "nfd::cs::lru" "magic"; do
            run_test $strategy $cache_buget 0.7 $cache_buget
        done
    done
elif [ "$1" = "-run_test_alpha" ];
then
    for alpha in 0.5 0.6 0.7 0.8 0.9 1.0 1.5; do
        for strategy in  "nfd::cs::lru" "magic"; do
            run_test $strategy 0.01 $alpha $alpha
        done
    done
elif [ "$1" = "-run_test_baseline" ];
then
    for cache_buget in 0.01 0.02 0.03 0.04 0.05 0.06 0.07 0.08 0.09 0.1 0.15 0.2; do
        for strategy in  "popularity" "dweb_broadcast"; do
            run_test $strategy $cache_buget 0.7 $cache_buget
        done
    done
fi
