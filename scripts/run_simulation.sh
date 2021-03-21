#!/bin/bash

NDNSIM_PATH="../ndnSIM"

EXAMPLES_PATH="/ns-3/src/ndnSIM/examples"
NFD_FW_PATH="/ns-3/src/ndnSIM/NFD/daemon/fw"

FILE_NAME="ndn-dweb_both11"

# cp "../sim_files/${FILE_NAME}.cpp" "${NDNSIM_PATH}${EXAMPLES_PATH}/${FILE_NAME}.cpp"
rsync -r "../ndnSIM_modifications/" ${NDNSIM_PATH}"/ns-3/src/ndnSIM"
rsync -r "../sim_files/" ${NDNSIM_PATH}${EXAMPLES_PATH}
rsync -r "../topologies/" ${NDNSIM_PATH}${EXAMPLES_PATH}"/topologies"
mkdir ${NDNSIM_PATH}${EXAMPLES_PATH}/$FILE_NAME
rsync -r "../sim_files/custom-apps/" ${NDNSIM_PATH}${EXAMPLES_PATH}/$FILE_NAME

chown -R cian ${NDNSIM_PATH}${EXAMPLES_PATH}

cd $NDNSIM_PATH/ns-3

# NS_LOG=DWebConsumer:DWebProducer:ndn-cxx.nfd.ContentStore ./waf --run=$FILE_NAME --vis
# NS_LOG=ndn-cxx.nfd.Magic:ndn.Consumer:ndn-cxx.nfd.Forwarder ./waf --command-template="gdb %s" --run=$FILE_NAME #--vis
# NS_LOG=DWebConsumer:DWebProducer:ndn-cxx.nfd.ContentStore ./waf --command-template="gdb %s" --run=$FILE_NAME
# NS_LOG=ndn-cxx.nfd.Magic   --command-template="gdb %s" 
# NS_LOG=DWebConsumer:DWebProducer ./waf --run=$FILE_NAME


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
elif [ "$1" = "-test" ];
then
    for cache_buget in 0.05 0.06 0.07 0.08 0.09 0.1 0.15 0.2; do
        for strategy in  "nfd::cs::lru" "nfd::cs::popularity_priority_queue"; do
        # in 0.01 0.02 0.03 0.04 0.05 0.06 0.07 0.08 0.09 0.1 0.15 0.2 0.3 0.4 0.5 0.6; do
            LOG_DIR=""
            LOG_FILE=""

            exec &>/dev/tty
            echo "$strategy cache budget $cache_buget"

            if [ "$strategy" = "nfd::cs::popularity_priority_queue" ]; then
                LOG_FILE="magic_$cache_buget"
            else
                LOG_FILE="lru_$cache_buget"
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

            echo "$strategy cache budget $cache_buget"

            if [ "$strategy" = "nfd::cs::popularity_priority_queue" ]; then
                time NS_LOG=DWebConsumer:DWebProducer:ndn-cxx.nfd.Magic:ndn-cxx.nfd.ContentStore ./waf --run=$FILE_NAME --command-template="%s $strategy 1 $cache_buget"
            else    
                time NS_LOG=DWebConsumer:DWebProducer:ndn-cxx.nfd.Magic:ndn-cxx.nfd.ContentStore ./waf --run=$FILE_NAME --command-template="%s $strategy 0 $cache_buget"
            fi

            cp "app-delays-trace.txt" $LOG_DIR
            cd /home/cian/Documents/GitHub/DWeb/scripts/
            python3 read_logs.py $LOG_FILE
            chown -R cian $LOG_DIR/$LOG_FILE.txt
            cd $NDNSIM_PATH/ns-3

        done
    done

fi
