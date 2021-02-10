import traceback
import hashlib
import json
from web3 import Web3, HTTPProvider
import logging

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
        self.indexed_objects[metadata] = blockchain_index


class EthereumWrapper:

    def __init__(self, contract_path, contract_addr, test_mode=True):
        self.contract_addr = contract_addr
        self.search_engine = SearchEngine(test_mode)
        self.test_mode = test_mode

        if not test_mode:
            try:
                compile_contract = open(contract_path)
            except OSError as e:
                print(f"Failed to open contract {contract_path}")
                raise e
            else:
                contract_json = json.load(compile_contract)
                self.contract_abi = contract_json['abi']
        else:
            self.object_data = []

        # calculate index offset from contract

    def calculate_oid(self, data, metadata):
        hash_gen = hashlib.sha512()

        combined = data + metadata
        hash_gen.update(combined.encode())

        return hash_gen.hexdigest()
        # return data[0]

    def set_object_data(self, data, metadata='', producer_node=''):
        if self.test_mode:
            oid = self.calculate_oid(data, metadata)
            self.search_engine.index_object(metadata, len(self.object_data))
            self.object_data.append((oid, metadata, producer_node))
            return

    def get_object_data(self, metadata):
        index = self.search_engine.lookup_object(metadata)
        if index is None:
            logger.error(f"Got request for unknown object {metadata}")
            return "None"

        if self.test_mode:  
            return self.object_data[index][0]
