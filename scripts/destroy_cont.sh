# Delete genesis block 
sudo ip link set br-genblk down
sudo brctl delif br-genblk tap_genblk
sudo brctl delbr br-genblk
sudo tunctl -d tap_genblk
sudo ip link delete internal-genblk

sudo ip link set br-miner down
sudo brctl delif br-miner tap_miner
sudo brctl delbr br-miner
sudo tunctl -d tap_miner
sudo ip link delete internal-miner


nLocality=12
for ((i = 1; i <= nLocality; i++))
do
contname=multi${i}

# Down bridges

sudo ip link set br-${contname} down

#Remove the taps from the bridges


sudo brctl delif br-${contname} tap_${contname}

# Delete the bridges
sudo brctl delbr br-${contname}

#Delete the taps

sudo tunctl -d tap_${contname}

# Delete interfaces

sudo ip link delete internal${i}
sudo ip link delete external${i}

# Destroy all images and containers
#docker system prune -a


# Stop all running container
sudo docker container stop $(docker container ls -aq)

# Destroy all stopped container
sudo docker container rm $(docker container ls -aq)


done

# Verify if still any container existed
sudo docker container ls -a


