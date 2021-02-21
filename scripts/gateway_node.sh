# Enter in super user
#su
# Create the bridges, counts equal to number of containers
# Number of docker containers required, i.e. number of supernodes/routing/gateway nodes except genesis block 


# Create the containers which will hold blocks of Ethereum. The number of containers should be equal to number of routing nodes

# create pool of IP addresses


contname=multi_gateway
#IMGNAME="ethereum/client-go:v1.8.12"
IMGNAME="ramansingh1984/eth-smart"
DETACH_FLAG=${DETACH_FLAG:-"-itd"}
DATA_ROOT=${DATA_ROOT:-"$(pwd)/.ether-$contname"}
DATA_HASH=${DATA_HASH:-"$(pwd)/.ethash-$contname"}
#RPC_PORTMAP=
#RPC_ARG=
if [[ ! -z $RPC_PORT ]]; then
#    RPC_ARG='--ws --wsaddr=0.0.0.0 --wsport 8546 --wsapi=db,eth,net,web3,personal --wsorigins "*" --rpc --rpcaddr=0.0.0.0 --rpcport 8545 --rpcapi=db,eth,net,web3,personal --rpccorsdomain "*"'
    RPC_ARG='--nousb --rpc --rpcaddr=0.0.0.0 --rpcport 8545 --rpcapi=db,eth,net,web3,personal --rpccorsdomain "*"'
    RPC_PORTMAP="-p $RPC_PORT:8545"
fi
BOOTNODE_URL=${BOOTNODE_URL:-$(./getbootnodeurl.sh)}
echo "Running new container $contname..."

docker run -p 127.0.0.1:8545:8545 -itd --name ${contname} --network bridge --publish-all=true  -v $DATA_ROOT:/root/.ethereum -v $DATA_HASH:/root/.ethash -v $(pwd)/genesis.json:/opt/genesis.json $IMGNAME 

docker exec -it ${contname} apk add gcc libc-dev python3-dev python3 py3-pip busybox-extras 
docker exec -it ${contname} pip3 install web3
docker exec -it ${contname} pip3 install py-solc
docker exec -it ${contname} pip3 install py-solc-x
docker exec -it ${contname} python3 -c "import solcx; solcx.install_solc('0.4.22')"
docker cp /home/cian/Documents/GitHub/DWeb/eth multi_gateway:/eth
#docker exec -itd ${contname} geth --nousb --rpc --rpcaddr=0.0.0.0 --rpcport 8545 --rpcapi=db,eth,net,web3,personal --rpccorsdomain "*"

# Connect the ethereum nodes with peers using bootnode



#docker exec -itd ${contname} geth --networkid 987 --syncmode=fast --nousb --rpc --rpcaddr=0.0.0.0 --rpcport 8545 --rpcapi=db,eth,net,web3,personal --rpccorsdomain "*" init /opt/genesis.json
# RPCADDR=ip route|grep "default via" \
# |grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' \
# |grep -m2 "" \
# |tail -1
docker exec -it ${contname} geth --networkid 987 --syncmode=fast --nousb --rpc --rpcapi --rpcaddr=0.0.0.0 --rpcport 8545 --rpcapi=db,eth,net,web3,personal --rpccorsdomain "*" init /opt/genesis.json

docker exec -itd ${contname} geth --bootnodes=$BOOTNODE_URL

# Create a new account in each ethereum nodes

docker cp pass_file.txt ${contname}:/pass_file.txt
echo  ${contname}> pass_file.txt
docker cp pass_file.txt ${contname}:/pass_file.txt
#docker exec -it ${contname} pkill -f "geth"
docker exec -itd ${contname} geth account new --password pass_file.txt
docker exec -itd ${contname} geth --password pass_file.txt --unlock primary --rpccorsdomain localhost --verbosity 4 2>> geth.log
docker exec -itd ${contname} geth --mine --minerthreads 1


# Create brdiges to connect docker containers with ns-3 nodes
sudo brctl addbr br-gate


# Create tap devices, counting equal to number of containers


sudo tunctl -t tap_gate

sudo ifconfig tap_gate 0.0.0.0 promisc up


# Connecting tap devices and bridges


sudo brctl addif br-gate tap_gate
sudo ifconfig br-gate 192.168.1.5 netmask 255.255.255.248 up   #IP address of bridge, also change IP address of VM Machine
sudo ifconfig br-gate up

# Allow traffic across bridges

pushd /proc/sys/net/bridge
for f in bridge-nf-*; do echo 0 > $f; done
popd

# Start all container, sometime some docker stops automatically

#sudo docker container start $(docker container ls -aq)

#Find the pid of containers


pidmulti=$(docker inspect --format '{{ .State.Pid }}' ${contname})

# Setting namespaces and interfaces with MAC and IP address for Other blocks

sudo mkdir -p /var/run/netns
ln -s /proc/$pidmulti/ns/net /var/run/netns/$pidmulti


#Now, We need to create "peer" interfaces: internal-left and external-left. The internal-left will be attached with the bridge.

sudo ip link add internal1 type veth peer name external1
sudo brctl addif br-gate internal1
sudo ip link set internal1 up

# Random MAC address
hexchars="0123456789ABCDEF"
end=$( for i in {1..8} ; do echo -n ${hexchars:$(( $RANDOM % 16 )):1} ; done | sed -e 's/\(..\)/:\1/g' )
MAC_ADDR="12:34"$end

sudo ip link set external1 netns $pidmulti
sudo ip netns exec $pidmulti ip link set dev external1 name eth1
sudo ip netns exec $pidmulti ip link set eth1 address $MAC_ADDR
sudo ip netns exec $pidmulti ip link set eth1 up
sudo ip netns exec $pidmulti ip addr add 192.168.1.6/29 dev eth1    #IP address of VM Machine also change IP address of bridge
sudo ip netns exec $pidmulti ip route add default via 192.168.1.2 dev eth1;


