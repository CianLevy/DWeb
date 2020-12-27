#!/bin/bash
#
# Runs a bootnode with ethereum official "alltools" image.
#
docker stop ethereum-bootnode
docker rm ethereum-bootnode
IMGNAME="ethereum/client-go:alltools-v1.8.12"
DATA_ROOT=${DATA_ROOT:-$(pwd)}
# generate bootnode key if needed
mkdir -p $DATA_ROOT/.bootnode
if [ ! -f $DATA_ROOT/.bootnode/boot.key ]; then
    echo "$DATA_ROOT/.bootnode/boot.key not found, generating..."
    docker run --rm \
        -v $DATA_ROOT/.bootnode:/opt/bootnode \
        $IMGNAME bootnode --genkey /opt/bootnode/boot.key
    echo "...done!"
fi
echo $DATA_ROOT
docker run -d --name ethereum-bootnode -v $DATA_ROOT/.bootnode:/opt/bootnode --network none --publish-all=true ethereum/client-go:alltools-v1.8.12 bootnode -nodekey /opt/bootnode/boot.key -verbosity 9 -addr :30301

# Create tap devices, for bootnode device

sudo tunctl -t tap_genblk

sudo ifconfig tap_genblk 0.0.0.0 promisc up


sudo brctl addbr br-genblk
# Connecting tap devices and bridges

sudo brctl addif br-genblk tap_genblk
sudo ifconfig br-genblk 192.168.0.5 netmask 255.255.255.248 up   #IP address of bridge, also change IP address of VM Machine
sudo ifconfig br-genblk up

# Allow traffic across bridges

pushd /proc/sys/net/bridge
for f in bridge-nf-*; do echo 0 > $f; done
popd

# Start all container, sometime some docker stops automatically

#sudo docker container start $(docker container ls -aq)

#Find the pid of containers
pid_genblk=$(docker inspect --format '{{ .State.Pid }}' ethereum-bootnode)



# Setting namespaces and interfaces with MAC and IP address for genblk block

sudo mkdir -p /var/run/netns
ln -s /proc/$pid_genblk/ns/net /var/run/netns/$pid_genblk


#Now, We need to create "peer" interfaces: internal-left and external-left. The internal-left will be attached with the bridge.

sudo ip link add internal-genblk type veth peer name external-genblk
sudo brctl addif br-genblk internal-genblk
sudo ip link set internal-genblk up


# Random MAC address
hexchars="0123456789ABCDEF"
end=$( for i in {1..8} ; do echo -n ${hexchars:$(( $RANDOM % 16 )):1} ; done | sed -e 's/\(..\)/:\1/g' )
MAC_ADDR="12:34"$end

sudo ip link set external-genblk netns $pid_genblk
sudo ip netns exec $pid_genblk ip link set dev external-genblk name eth0
sudo ip netns exec $pid_genblk ip link set eth0 address $MAC_ADDR
sudo ip netns exec $pid_genblk ip link set eth0 up
sudo ip netns exec $pid_genblk ip addr add 192.168.0.6/29 dev eth0   #IP address of VM Machine also change IP address of bridge
sudo ip netns exec $pid_genblk ip route add default via 192.168.0.2 dev eth0

