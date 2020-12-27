# Enter in super user
#su
# Create the bridges, counts equal to number of containers
# Number of docker containers required, i.e. number of supernodes/routing/gateway nodes except genesis block 


# Create the containers which will hold blocks of Ethereum. The number of containers should be equal to number of routing nodes

# create pool of IP addresses

eval ip_cont[1]='6'
eval ip_cont[2]='14'
eval ip_cont[3]='22'
eval ip_cont[4]='30'
eval ip_cont[5]='38'
eval ip_cont[6]='46'
eval ip_cont[7]='54'
eval ip_cont[8]='62'
eval ip_cont[9]='70'
eval ip_cont[10]='78'
eval ip_cont[11]='86'
eval ip_cont[12]='94'
eval ip_cont[13]='104'
eval ip_cont[14]='112'
eval ip_cont[15]='120'
eval ip_cont[16]='128'
eval ip_cont[17]='136'
eval ip_cont[18]='144'
eval ip_cont[19]='152'
eval ip_cont[20]='158'
eval ip_cont[21]='166'
eval ip_cont[22]='174'
eval ip_cont[23]='182'
eval ip_cont[24]='190'
eval ip_cont[25]='198'
eval ip_cont[26]='206'
eval ip_cont[27]='214'
eval ip_cont[28]='222'
eval ip_cont[29]='230'
eval ip_cont[30]='238'
eval ip_cont[31]='246'
eval ip_cont[32]='254'

eval ipbr_cont[1]='5'
eval ipbr_cont[2]='13'
eval ipbr_cont[3]='21'
eval ipbr_cont[4]='29'
eval ipbr_cont[5]='37'
eval ipbr_cont[6]='45'
eval ipbr_cont[7]='53'
eval ipbr_cont[8]='61'
eval ipbr_cont[9]='69'
eval ipbr_cont[10]='77'
eval ipbr_cont[11]='85'
eval ipbr_cont[12]='93'
eval ipbr_cont[13]='101'
eval ipbr_cont[14]='109'
eval ipbr_cont[15]='117'
eval ipbr_cont[16]='125'
eval ipbr_cont[17]='133'
eval ipbr_cont[18]='141'
eval ipbr_cont[19]='149'
eval ipbr_cont[20]='157'
eval ipbr_cont[21]='165'
eval ipbr_cont[22]='173'
eval ipbr_cont[23]='181'
eval ipbr_cont[24]='189'
eval ipbr_cont[25]='197'
eval ipbr_cont[26]='205'
eval ipbr_cont[27]='213'
eval ipbr_cont[28]='221'
eval ipbr_cont[29]='229'
eval ipbr_cont[30]='237'
eval ipbr_cont[31]='245'
eval ipbr_cont[32]='253'

eval ipbr_rot[1]='2'
eval ipbr_rot[2]='10'
eval ipbr_rot[3]='18'
eval ipbr_rot[4]='26'
eval ipbr_rot[5]='34'
eval ipbr_rot[6]='42'
eval ipbr_rot[7]='50'
eval ipbr_rot[8]='58'
eval ipbr_rot[9]='66'
eval ipbr_rot[10]='74'
eval ipbr_rot[11]='82'
eval ipbr_rot[12]='90'
eval ipbr_rot[13]='98'
eval ipbr_rot[14]='106'   
eval ipbr_rot[15]='114'
eval ipbr_rot[16]='122'
eval ipbr_rot[17]='130'
eval ipbr_rot[18]='138'
eval ipbr_rot[19]='146'
eval ipbr_rot[20]='154' 
eval ipbr_rot[21]='162'
eval ipbr_rot[22]='170'
eval ipbr_rot[23]='178'
eval ipbr_rot[24]='186'
eval ipbr_rot[25]='194'
eval ipbr_rot[26]='202'
eval ipbr_rot[27]='210'
eval ipbr_rot[28]='218'
eval ipbr_rot[29]='226'
eval ipbr_rot[30]='234'
eval ipbr_rot[31]='242'
eval ipbr_rot[32]='250'

i=$1
nLocality=$2
declare -i ip_c=$3
declare -i ip_c1=$4


contname=multi${i}
#IMGNAME="ethereum/client-go:v1.8.12"
IMGNAME="ramansingh1984/eth-smart"
DETACH_FLAG=${DETACH_FLAG:-"-itd"}
DATA_ROOT=${DATA_ROOT:-"$(pwd)/.ether-$contname"}
DATA_HASH=${DATA_HASH:-"$(pwd)/.ethash"}
#RPC_PORTMAP=
#RPC_ARG=
if [[ ! -z $RPC_PORT ]]; then
#    RPC_ARG='--ws --wsaddr=0.0.0.0 --wsport 8546 --wsapi=db,eth,net,web3,personal --wsorigins "*" --rpc --rpcaddr=0.0.0.0 --rpcport 8545 --rpcapi=db,eth,net,web3,personal --rpccorsdomain "*"'
    RPC_ARG='--nousb --rpc --rpcaddr=0.0.0.0 --rpcport 8545 --rpcapi=db,eth,net,web3,personal --rpccorsdomain "*"'
    RPC_PORTMAP="-p $RPC_PORT:8545"
fi
BOOTNODE_URL=${BOOTNODE_URL:-$(./getbootnodeurl.sh)}
echo "Running new container $contname..."

docker run -itd --name ${contname} --network none --publish-all=true  -v $DATA_ROOT:/root/.ethereum -v $DATA_HASH:/root/.ethash -v $(pwd)/genesis.json:/opt/genesis.json $IMGNAME 

# Create brdiges to connect docker containers with ns-3 nodes
sudo brctl addbr br-${contname}


# Create tap devices, counting equal to number of containers


sudo tunctl -t tap_${contname}

sudo ifconfig tap_${contname} 0.0.0.0 promisc up


# Connecting tap devices and bridges


sudo brctl addif br-${contname} tap_${contname}
sudo ifconfig br-${contname} 192.168.${ip_c}.${ipbr_cont[ip_c1]} netmask 255.255.255.248 up   #IP address of bridge, also change IP address of VM Machine
sudo ifconfig br-${contname} up

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

sudo ip link add internal${i} type veth peer name external${i}
sudo brctl addif br-${contname} internal${i}
sudo ip link set internal${i} up

# Random MAC address
hexchars="0123456789ABCDEF"
end=$( for i in {1..8} ; do echo -n ${hexchars:$(( $RANDOM % 16 )):1} ; done | sed -e 's/\(..\)/:\1/g' )
MAC_ADDR="12:34"$end

sudo ip link set external${i} netns $pidmulti
sudo ip netns exec $pidmulti ip link set dev external${i} name eth0
sudo ip netns exec $pidmulti ip link set eth0 address $MAC_ADDR
sudo ip netns exec $pidmulti ip link set eth0 up
sudo ip netns exec $pidmulti ip addr add 192.168.${ip_c}.${ip_cont[ip_c1]}/29 dev eth0    #IP address of VM Machine also change IP address of bridge
sudo ip netns exec $pidmulti ip route add default via 192.168.${ip_c}.${ipbr_rot[ip_c1]} dev eth0;




# Connect the ethereum nodes with peers using bootnode

docker exec -itd ${contname} geth --bootnodes=$BOOTNODE_URL

# Create a new account in each ethereum nodes

docker cp pass_file.txt ${contname}:/pass_file.txt
echo  ${contname}> pass_file.txt
docker cp pass_file.txt ${contname}:/pass_file.txt
#docker exec -it ${contname} pkill -f "geth"
docker exec -it ${contname} geth account new --password pass_file.txt
docker exec -it ${contname} geth --password pass_file.txt --unlock primary --rpccorsdomain localhost --verbosity 4 2>> geth.log
#docker exec -it ${contname} geth --mine





