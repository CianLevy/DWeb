import asyncio
import logging
import os
import random
import socket

from ethereum_wrapper import EthereumWrapper

logger = logging.getLogger(__name__)


def make_singleton(wrapped_class):
    instances = {}
    def getinstance(*args, **kwargs):
        if wrapped_class not in instances:
            instances[wrapped_class] = wrapped_class(*args, **kwargs)
        return instances[wrapped_class]
    return getinstance 


@make_singleton
class UDPDatagramProcessingProtocol:
    def __init__(self):
        super().__init__()
        self.processing_func = None

    def connection_made(self, transport):
        self.transport = transport

    def datagram_received(self, data, addr):
        if self.processing_func:
            self.processing_func(data, addr)

    def register_datagram_processing_callback(self, callback_func):
        self.processing_func = callback_func


class RequestHandler:
    def __init__(self, ethereum_wrapper):
        self.ethereum_wrapper = ethereum_wrapper

    async def process_req(self, req):
        split_req = req.split('/')

        if 'set' in req and len(split_req) == 5:
            res = self.ethereum_wrapper.set_object_data(split_req[4], split_req[3], split_req[2])
            # 'set' request structure is: set/req_id/producer_id/metadata/data
            logger.debug(f"Processed set for {req}")
            return f"{split_req[1]}/oid/{res}/{split_req[3]}"

        elif 'get' in req and len(split_req) == 3:
            # 'get' request structure is: get/req_id/metadata
            res = self.ethereum_wrapper.get_object_data(split_req[2])
            logger.debug(f"Processed get for {req} returning {res}")
            return f"{split_req[1]}/oid/{res}/{split_req[2]}"
            # split_req[1] + "/oid/" + res "/" + (split_req[2]

        elif 'verify' in req and len(split_req) == 5:
            # 'verify' request structure is: verify/req_id/oid/metadata/data
            res = self.ethereum_wrapper.verify_object(split_req[2], split_req[3], split_req[4])
            logger.info(f"Result of verfication request for {split_req[2]} is {res}")

            return f"{split_req[1]}/oid/{res}/{split_req[2]}"
        else:
            logger.error(f'Received unknown request {req}')
            return None
        

class UDPServer:
    def __init__(self, host_addr, port):
        self.host_addr = host_addr
        self.port = port
        self.socket = sock = socket.socket(socket.AF_INET,
                                           socket.SOCK_DGRAM)
        self.eth_wrapper = EthereumWrapper('contracts/publishobject.sol',
                                           'http://127.0.0.1:8545', False)
        self.req_handler = RequestHandler(self.eth_wrapper)
        
        self.loop = asyncio.get_event_loop()
        self.processing_loop = asyncio.get_event_loop()

        protocol = UDPDatagramProcessingProtocol()
        protocol.register_datagram_processing_callback(self.recieve)


        t = self.loop.create_datagram_endpoint(UDPDatagramProcessingProtocol, local_addr=(host_addr, port))

        self.loop.run_until_complete(t)
        self.loop.run_forever()


    def recieve(self, data, addr):
        self.loop.create_task(self.async_recieve(data, addr))

    async def async_recieve(self, data, addr):
        logger.debug(f"Recieved: {data}")
        decoded_data = None

        try:
            decoded_data = data.decode()
        except UnicodeDecodeError:
            decoded_data = data[:len(data)- 7].decode()

        res = await self.req_handler.process_req(decoded_data)

        if res:
            self.send(res, addr)


    def send(self, message, addr):
        self.socket.sendto(message.encode(), addr)



logging.basicConfig(level=logging.INFO)


if __name__ == '__main__':
    server = UDPServer("192.168.1.6", 3000)
