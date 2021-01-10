NDNSIM_PATH="../ndnSIM"

EXAMPLES_PATH="/ns-3/src/ndnSIM/examples"
NFD_FW_PATH="/ns-3/src/ndnSIM/NFD/daemon/fw"

FILE_NAME="ndn-dweb_both"

cp "../${FILE_NAME}.cpp" "${NDNSIM_PATH}${EXAMPLES_PATH}/${FILE_NAME}.cpp"
cp -r "../forwarding_modifications/." "${NDNSIM_PATH}${NFD_FW_PATH}"
cp -r "../cache_policy/." ${NDNSIM_PATH}"/ns-3/src/ndnSIM/NFD/daemon/table"

cd $NDNSIM_PATH/ns-3

# ./waf --run=$FILE_NAME --vis
# ./waf --command-template="gdb %s" --run=$FILE_NAME

# ./waf --run=ndn-tree-cs-tracers
./waf --command-template="gdb %s" --run=ndn-tree-cs-tracers