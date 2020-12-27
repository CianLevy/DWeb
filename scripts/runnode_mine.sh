#!/bin/bash
IMGNAME="ethereum/client-go:v1.8.12"
NODE_NAME=$1
NODE_NAME=${NODE_NAME:-"node1"}
DETACH_FLAG=${DETACH_FLAG:-"-d"}
CONTAINER_NAME="ethereum-$NODE_NAME"
DATA_ROOT=${DATA_ROOT:-"$(pwd)/.ether-$NODE_NAME"}
DATA_HASH=${DATA_HASH:-"$(pwd)/.ethash"}
echo "Destroying old container $CONTAINER_NAME..."
docker stop $CONTAINER_NAME
docker rm $CONTAINER_NAME
RPC_PORTMAP=
RPC_ARG=
if [[ ! -z $RPC_PORT ]]; then
#    RPC_ARG='--ws --wsaddr=0.0.0.0 --wsport 8546 --wsapi=db,eth,net,web3,personal --wsorigins "*" --rpc --rpcaddr=0.0.0.0 --rpcport 8545 --rpcapi=db,eth,net,web3,personal --rpccorsdomain "*"'
    RPC_ARG='--nousb --rpc --rpcaddr=0.0.0.0 --rpcport 8545 --rpcapi=db,eth,net,web3,personal --rpccorsdomain "*"'
    RPC_PORTMAP="-p $RPC_PORT:8545"
fi
BOOTNODE_URL=${BOOTNODE_URL:-$(./getbootnodeurl.sh)}
if [ ! -f $(pwd)/genesis.json ]; then
    echo "No genesis.json file found, please run 'genesis.sh'. Aborting."
    exit
fi
if [ ! -d $DATA_ROOT/keystore ]; then
    echo "$DATA_ROOT/keystore not found, running 'geth init'..."
    docker run --rm \
        -v $DATA_ROOT:/root/.ethereum \
        -v $(pwd)/genesis.json:/opt/genesis.json \
        $IMGNAME init /opt/genesis.json
    echo "...done!"
fi
echo "Running new container $CONTAINER_NAME..."
docker run $DETACH_FLAG --name $CONTAINER_NAME \
    --network none \
    --publish-all=true \
    -v $DATA_ROOT:/root/.ethereum \
    -v $DATA_HASH:/root/.ethash \
    -v $(pwd)/genesis.json:/opt/genesis.json \
    $RPC_PORTMAP \
    $IMGNAME --bootnodes=$BOOTNODE_URL $RPC_ARG --cache=512 --verbosity=4 --maxpeers=3 ${@:2}

# Create tap devices, counting equal to number of containers

sudo tunctl -t tap_miner

sudo ifconfig tap_miner 0.0.0.0 promisc up


sudo brctl addbr br-miner
# Connecting tap devices and bridges

sudo brctl addif br-miner tap_miner
sudo ifconfig br-miner 192.168.0.13 netmask 255.255.255.248 up   #IP address of bridge, also change IP address of VM Machine
sudo ifconfig br-miner up

# Allow traffic across bridges

pushd /proc/sys/net/bridge
for f in bridge-nf-*; do echo 0 > $f; done
popd

# Start all container, sometime some docker stops automatically

#sudo docker container start $(docker container ls -aq)

#Find the pid of containers
pid_miner=$(docker inspect --format '{{ .State.Pid }}' ethereum-miner1)




# Setting namespaces and interfaces with MAC and IP address for miner block

sudo mkdir -p /var/run/netns
ln -s /proc/$pid_miner/ns/net /var/run/netns/$pid_miner


#Now, We need to create "peer" interfaces: internal-left and external-left. The internal-left will be attached with the bridge.

sudo ip link add internal-miner type veth peer name external-miner
sudo brctl addif br-miner internal-miner
sudo ip link set internal-miner up


# Random MAC address
hexchars="0123456789ABCDEF"
end=$( for i in {1..8} ; do echo -n ${hexchars:$(( $RANDOM % 16 )):1} ; done | sed -e 's/\(..\)/:\1/g' )
MAC_ADDR="12:34"$end

sudo ip link set external-miner netns $pid_miner
sudo ip netns exec $pid_miner ip link set dev external-miner name eth0
sudo ip netns exec $pid_miner ip link set eth0 address $MAC_ADDR
sudo ip netns exec $pid_miner ip link set eth0 up
sudo ip netns exec $pid_miner ip addr add 192.168.0.14/29 dev eth0   #IP address of VM Machine also change IP address of bridge
sudo ip netns exec $pid_miner ip route add default via 192.168.0.10 dev eth0

