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
docker run -d --name ethereum-bootnode -v $DATA_ROOT/.bootnode:/opt/bootnode --network bridge --publish-all=true ethereum/client-go:alltools-v1.8.12 bootnode -nodekey /opt/bootnode/boot.key -verbosity 9 -addr :30301


