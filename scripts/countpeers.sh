#!/bin/sh
NODE=$1
NODE=${NODE:-"node1"}
CONTAINER_NAME="ethereum-$NODE"

for i in $(seq 10); do
    RES=$(docker exec -ti "multi$i" geth --exec 'admin.peers.length' attach)
    echo "multi$i peer count: $RES"
done
