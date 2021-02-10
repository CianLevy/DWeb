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

    def connection_made(self, transport) -> "Used by asyncio":
        self.transport = transport

    def datagram_received(self, data, addr) -> "Main entrypoint for processing message":
        if self.processing_func:
            self.processing_func(data)

    def register_datagram_processing_callback(self, callback_func):
        self.processing_func = callback_func


class RequestHandler:
    def __init__(self, ethereum_wrapper):
        self.ethereum_wrapper = ethereum_wrapper

    async def process_req(self, req):
        # import pdb; pdb.set_trace()
        split_req = req.split('/')

        if 'set' in req and len(split_req) == 4:
            self.ethereum_wrapper.set_object_data(split_req[3], split_req[2], split_req[1])
            # 'set' request structure is: set/producer_id/metadata/data
            logger.info(f"Processed set for {req}")
            return None
        elif 'get' in req and len(split_req) == 2:
            # 'get' request structure is: get/metadata
            res = self.ethereum_wrapper.get_object_data(split_req[1])
            logger.info(f"Processed get for {req} returning {res}")
            return "oid/" + res
        else:
            logger.error(f'Received unknown request {req}')
            return None
        

class UDPServer:
    def __init__(self, host_addr, port):
        self.host_addr = host_addr
        self.port = port
        self.socket = sock = socket.socket(socket.AF_INET,
                                           socket.SOCK_DGRAM)
        self.eth_wrapper = EthereumWrapper('', '')
        self.req_handler = RequestHandler(self.eth_wrapper)
        
        self.loop = asyncio.get_event_loop()
        self.processing_loop = asyncio.get_event_loop()

        protocol = UDPDatagramProcessingProtocol()
        protocol.register_datagram_processing_callback(self.recieve)


        t = self.loop.create_datagram_endpoint(UDPDatagramProcessingProtocol, local_addr=(host_addr, port))

        self.loop.run_until_complete(t)
        # loop.run_until_complete(self.write_messages()) # Start writing messages (or running tests)
        self.loop.run_forever()


    def recieve(self, data):
        self.loop.create_task(self.recieve2(data))

    async def recieve2(self, data):
        logger.info(f"Recieved {data}")
        res = await self.req_handler.process_req(data.decode())

        if res:
            self.send(res)

    # def datagram_received(self, data, addr):
    #     loop = asyncio.get_event_loop()
    #     loop.create_task(self.handle_income_packet(data, addr))


    # async def handle_income_packet(self, data, addr):
    #     # echo back the message, but 2 seconds later
    #     await asyncio.sleep(2)
    #     self.transport.sendto(data, addr)

    def send(self, message):
        self.socket.sendto(message.encode(), (self.host_addr, self.port))



logging.basicConfig(level=logging.INFO)


if __name__ == '__main__':
    server = UDPServer("localhost", 3000)
