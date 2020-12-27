NDNSIM_PATH="../ndnSIM"

EXAMPLES_PATH="/ns-3/src/ndnSIM/examples"

FILE_NAME="ndn-dweb_both"

cp "../${FILE_NAME}.cpp" "${NDNSIM_PATH}${EXAMPLES_PATH}/ndn-dweb_both.cpp"


cd $NDNSIM_PATH/ns-3
./waf --run=$FILE_NAME --vis
# ./waf --command-template="gdb %s" --run=$FILE_NAME 