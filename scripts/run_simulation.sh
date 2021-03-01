NDNSIM_PATH="../ndnSIM"

EXAMPLES_PATH="/ns-3/src/ndnSIM/examples"
NFD_FW_PATH="/ns-3/src/ndnSIM/NFD/daemon/fw"

FILE_NAME="ndn-dweb_both11"

# cp "../sim_files/${FILE_NAME}.cpp" "${NDNSIM_PATH}${EXAMPLES_PATH}/${FILE_NAME}.cpp"
rsync -r "../ndnSIM_modifications/" ${NDNSIM_PATH}"/ns-3/src/ndnSIM"
rsync -r "../sim_files/" ${NDNSIM_PATH}${EXAMPLES_PATH}
mkdir ${NDNSIM_PATH}${EXAMPLES_PATH}/$FILE_NAME
rsync -r "../sim_files/custom-apps/" ${NDNSIM_PATH}${EXAMPLES_PATH}/$FILE_NAME


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
    NS_LOG=DWebConsumer:DWebProducer:ndn-cxx.nfd.Magic:ndn-cxx.nfd.ContentStore ./waf --command-template="gdb %s" --run=$FILE_NAME
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
    NS_LOG=DWebConsumer:DWebProducer:ndn-cxx.nfd.Magic:ndn-cxx.nfd.ContentStore:ndn-cxx.nfd.Forwarder ./waf --run=$FILE_NAME
elif [ "$1" = "-t" ];
then
    ./waf --run=create_topology 
fi
