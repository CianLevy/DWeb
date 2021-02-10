NDNSIM_PATH="../ndnSIM"

EXAMPLES_PATH="/ns-3/src/ndnSIM/examples"
NFD_FW_PATH="/ns-3/src/ndnSIM/NFD/daemon/fw"

FILE_NAME="ndn-dweb_both5"

cp "../sim_files/${FILE_NAME}.cpp" "${NDNSIM_PATH}${EXAMPLES_PATH}/${FILE_NAME}.cpp"
rsync -r "../ndnSIM_modifications/" ${NDNSIM_PATH}"/ns-3/src/ndnSIM"

cd $NDNSIM_PATH/ns-3

NS_LOG=ndn-cxx.nfd.Magic:ndn.Consumer:ndn-cxx.nfd.Forwarder ./waf --run=$FILE_NAME #--vis
# NS_LOG=ndn-cxx.nfd.Magic:ndn.Consumer:ndn-cxx.nfd.Forwarder ./waf --command-template="gdb %s" --run=$FILE_NAME #--vis
# NS_LOG=ndn-cxx.nfd.Magic   --command-template="gdb %s" 