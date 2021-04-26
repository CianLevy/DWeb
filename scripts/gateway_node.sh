contname=multi_gateway
IMGNAME="ramansingh1984/eth-smart"
DETACH_FLAG=${DETACH_FLAG:-"-itd"}
DATA_ROOT=${DATA_ROOT:-"$(pwd)/.ether-$contname"}
DATA_HASH=${DATA_HASH:-"$(pwd)/.ethash-$contname"}

if [[ ! -z $RPC_PORT ]]; then
    RPC_ARG='--nousb --rpc --rpcaddr=0.0.0.0 --rpcport 8545 --rpcapi=db,eth,net,web3,personal --rpccorsdomain "*"'
    RPC_PORTMAP="-p $RPC_PORT:8545"
fi
BOOTNODE_URL=${BOOTNODE_URL:-$(./getbootnodeurl.sh)}
echo "Running new container $contname..."

docker run -p 127.0.0.1:3000:3000/udp -itd --name ${contname} --network bridge --publish-all=true  -v $DATA_ROOT:/root/.ethereum -v $DATA_HASH:/root/.ethash -v $(pwd)/genesis.json:/opt/genesis.json $IMGNAME 

docker exec -it ${contname} apk add gcc libc-dev python3-dev python3 py3-pip busybox-extras 
docker exec -it ${contname} pip3 install web3
docker exec -it ${contname} pip3 install py-solc
docker exec -it ${contname} pip3 install py-solc-x
docker exec -it ${contname} python3 -c "import solcx; solcx.install_solc('0.4.22')"
docker cp ../eth multi_gateway:/eth


docker exec -it ${contname} geth --networkid 987 --syncmode=fast --nousb --rpc --rpcapi --rpcaddr=0.0.0.0 --rpcport 8545 --rpcapi=db,eth,net,web3,personal --rpccorsdomain "*" init /opt/genesis.json

# Connect the ethereum nodes with peers using bootnode
docker exec -itd ${contname} geth --bootnodes=$BOOTNODE_URL

# Create a new account in each ethereum nodes
docker cp pass_file.txt ${contname}:/pass_file.txt
echo  ${contname}> pass_file.txt
docker cp pass_file.txt ${contname}:/pass_file.txt

docker exec -itd ${contname} geth account new --password pass_file.txt
docker exec -itd ${contname} geth --password pass_file.txt --unlock primary --rpccorsdomain localhost --verbosity 4 2>> geth.log
docker exec -itd ${contname} geth --mine --minerthreads 1
