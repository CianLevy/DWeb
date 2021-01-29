NDNSIM_PATH="../ndnSIM"

EXAMPLES_PATH="/ns-3/src/ndnSIM/examples"
NFD_FW_PATH="/ns-3/src/ndnSIM/NFD/daemon/fw"

FILE_NAME="ndn-dweb_both"

cp "../sim_files/${FILE_NAME}.cpp" "${NDNSIM_PATH}${EXAMPLES_PATH}/${FILE_NAME}.cpp"
rsync -r "../ndnSIM_modifications/" ${NDNSIM_PATH}"/ns-3/src/ndnSIM/NFD/daemon"

cd $NDNSIM_PATH/ns-3

NS_LOG=ndn-cxx.nfd.Magic  ./waf --command-template="gdb %s" --run=ndn-dweb_both
