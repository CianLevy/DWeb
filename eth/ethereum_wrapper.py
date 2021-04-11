import hashlib
import json
import logging
import traceback
import time

import solcx
from solcx import compile_source
from web3 import HTTPProvider, Web3

solcx.set_solc_version('0.4.22')
logger = logging.getLogger(__name__)

class SearchEngine:
# """
# Responsible for mapping metadata (user queries) to blockchain values.
# i.e. Ethereum contract index.
# """
    def __init__(self, test_mode=True):
        self.indexed_objects = {}
        self.test_mode = test_mode

    def lookup_object(self, metadata):
        return self.indexed_objects.get(metadata)

    def index_object(self, metadata, blockchain_index):
        if metadata in self.indexed_objects:
            logger.error(f'Attempted to index pre-existing value: {metadata}')
            # return False
            self.indexed_objects[metadata] = blockchain_index
            logger.info(f'Indexed new value {metadata}')
            return True
        else:
            self.indexed_objects[metadata] = blockchain_index
            logger.info(f'Indexed new value {metadata}')
            return True

class EthereumWrapper:

    def __init__(self, contract_path, blockchain_address, test_mode=True):
        self.search_engine = SearchEngine(test_mode)
        self.test_mode = test_mode

        if not test_mode:
            self.web3 = Web3(HTTPProvider(blockchain_address))
            self.web3.eth.defaultAccount = self.web3.eth.accounts[0]

            try:
                self.contract = self.deploy_contract(contract_path)
            except OSError as e:
                print(f"Failed to deploy contract {contract_path}")
                raise e
            
            try:
                self.index = self.contract.functions.GetObjectCount().call()
            except Exception as e:
                raise e
            else:
                logger.info("Successfully deployed contract")
        else:
            self.object_data = []

    def deploy_contract(self, contract_path):
        compiled_contract = None

        with open(contract_path, u'r') as f:
            source = f.read()
            compiled_contract = compile_source(source)

        contract_id, contract_interface = compiled_contract.popitem()

        tx_hash = self.web3.eth.contract(abi=contract_interface['abi'],
                                  bytecode=contract_interface['bin']).constructor().transact()

        time.sleep(10)
        address = self.web3.eth.getTransactionReceipt(tx_hash)['contractAddress']

        return self.web3.eth.contract(address=address, abi=contract_interface["abi"])

    def calculate_oid(self, data, metadata):
        hash_gen = hashlib.sha512()

        combined = data + metadata
        hash_gen.update(combined.encode())

        return hash_gen.hexdigest()

    def set_object_data(self, data, metadata='', producer_node=''):
        oid = self.calculate_oid(data, metadata)

        if self.test_mode:
            self.search_engine.index_object(metadata, len(self.object_data))
            self.object_data.append((oid, metadata, producer_node))
            return oid
        else:
            res = self.search_engine.index_object(metadata, self.index)
            if res:
                self.index += 1
                res = self.contract.functions.SetObjectInfo(oid, metadata).transact()

            return oid

    def get_object_data(self, metadata):
        index = self.search_engine.lookup_object(metadata)

        if index is None:
            logger.error(f"Got request for unknown object {metadata}")
            return "None"

        if self.test_mode:  
            return self.object_data[index][0]
        else:
            res = self.contract.functions.GetObjectInfo(index).call()
            return res[0]
    
    def verify_object(self, oid, metadata, data):
        test_oid = self.calculate_oid(data, metadata)

        return test_oid == oid

