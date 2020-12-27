#!/bin/bash
FROM ethereum/client-go:alltools-v1.8.12

RUN apk add --update git bash nodejs nodejs-npm perl

RUN cd /root &&\
    git clone https://github.com/cubedro/eth-net-intelligence-api &&\
    cd eth-net-intelligence-api &&\
    npm install &&\
    npm install -g pm2 &&\
    npm i -g --unsafe-perm=true --allow-root truffle && \
    npm install -g solc



