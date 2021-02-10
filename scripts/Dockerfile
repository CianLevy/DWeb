#!/bin/bash
FROM ethereum/client-go:alltools-v1.8.25

RUN apk add --update git bash npm perl
    

RUN cd /root &&\
    git clone https://github.com/cubedro/eth-net-intelligence-api &&\
    cd eth-net-intelligence-api &&\
    npm install &&\
    npm install -g pm2 &&\
    npm i -g --unsafe-perm=true --allow-root truffle &&\
    npm install -g solc

RUN apk add --update --no-cache python3 && ln -sf python3 /usr/bin/python
RUN python3 -m ensurepip
RUN pip3 install --no-cache --upgrade pip setuptools
RUN pip3 install web3
