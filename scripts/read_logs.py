import sys
import re
import collections
from tabulate import tabulate
import matplotlib.pyplot as plt
import numpy as np

class LogReader:
    def __init__(self, base_log_name, file_name):
        self.output_file = open(f"{LOGS_FOLDER}/{base_log_name}/{file_name}", 'w+')

    def __del__(self):
        self.write_output()

    def write_line_to_log(self, line):
        self.output_file.write(line + "\n")


class MagicLogReader(LogReader):
    def __init__(self, file_name):
        super().__init__(file_name, f"magic_log_{file_name}.txt")
        self.res = {}

  
    def read_line(self, line):

        try:
            if "return" in line:
                line = line.split()
                node = line[4]
                nonce = line[5]

                if nonce not in self.res:
                    self.res[nonce] = {}
                    self.res[nonce]['req'] = line[12]
                
                if 'return' not in self.res[nonce]:
                    self.res[nonce]['return'] = []

                self.res[nonce]['return'].append((node, line[8], line[10]))

            elif "outbound" in line:
                line = line.split()
                node = line[4]
                nonce = line[5]

                if nonce not in self.res:
                    self.res[nonce] = {}
                    self.res[nonce]['req'] = line[12]
                
                if 'req_path' not in self.res[nonce]:
                    self.res[nonce]['req_path'] = []
                # import pdb; pdb.set_trace()
                self.res[nonce]['req_path'].append((node, line[8], line[10]))
        except Exception:
            import pdb; pdb.set_trace()
            

    def write_output(self):
        total_outbound_hops = 0

        for nonce, paths in self.res.items():
            # if len(paths.get('req_path', [])) < 4 or len(paths.get('req_path', [])) != len(paths.get('return', [])):
            #     continue

            self.write_line_to_log(f"############################ Nonce: {nonce} ############################")
            self.write_line_to_log(f"Request: {paths.pop('req')}")
            for direction, path in paths.items():
                if direction == 'req_path':
                    self.write_line_to_log("Request Path")
                    total_outbound_hops += len(path)
                else:
                    self.write_line_to_log("Return Path")

                for hop in path:
                    self.write_line_to_log(f"{hop[0]} Max gain {hop[1]} Local gain {hop[2]}")

        self.write_line_to_log("\n")
        self.write_line_to_log(f"Total outbound hops {total_outbound_hops}")

class CacheLogReader(LogReader):
    def __init__(self, file_name):
        super().__init__(file_name, f"cache_log_{file_name}.txt")
        self.res = []
        self.miss_count = 0
        self.hit_count = 0

    def read_line(self, line):
        split_line = line.split()

        if 'no-match' in line:
            # line structure: time proc_id func log_level "find" req_name "no-match"
            no_match_record = (split_line[0], split_line[1], split_line[5], "no-match")
            self.res.append(no_match_record)
            self.update_hit_ratio(line, False)
            
        
        elif 'matching' in line:
            # line structure: time proc_id func log_level "find" req_name "matching" match_name
            match_record = (split_line[0], split_line[1], split_line[5], "matched")
            self.res.append(match_record)
            self.update_hit_ratio(line, True)

    def write_output(self):
        self.write_line_to_log(f"####################### Summary ########################")
        total = self.miss_count + self.hit_count

        total = 1 if total == 0 else total

        self.write_line_to_log(f"Hit ratio: {(self.hit_count / total) * 100}")
        self.write_line_to_log(f"Miss ratio: {(self.miss_count / total) * 100}\n")
        self.write_line_to_log(f"Hit count: {self.hit_count}")
        self.write_line_to_log(f"Miss count: {self.miss_count}")


        self.write_line_to_log(f"####################### Cache log ########################")

        for record in self.res:
            self.write_line_to_log(f"{record[0]}: {record[1]} {record[3]} {record[2]}")

    def update_hit_ratio(self, line, hit):
        pattern = "/([\w.]+)/%FE%"
        

        m = re.search(pattern, line)
        oid_length = 128 
        # if m:
        #     import pdb; pdb.set_trace()

        if m and len(m.group(1)) == oid_length:
            if hit:
                self.hit_count += 1
            else:
                self.miss_count += 1


class ProducerLogReader(LogReader):
    def __init__(self, file_name):
        super().__init__(file_name, f"producer_log_{file_name}.txt")
        self.res = {}

    def read_line(self, line):
        split_line = line.split()
        
        new_publish = 'Attempting to publish metadata:'
        if new_publish in line:
            pattern = "([\w.]+): Attempting to publish metadata: ([\w.]+)"
            m = re.search(pattern, line)
            
            metadata = m.group(2)
            if metadata not in self.res:
                self.res[m.group(2)] = {
                    'attempt_record': {
                        'time': split_line[0],
                        'source': m.group(1)
                    }
                }
            else:
                import pdb; pdb.set_trace()
                raise Exception("Found duplicate metadata publication")
            
        elif 'Published new object. Metadata' in line:
            pattern = "([\w.]+): Published new object. Metadata: ([\w.]+) oid: ([\w./]+)"

            m = re.search(pattern, line)
            metadata = m.group(2)

            if metadata in self.res:
                self.res[metadata]['publication_record'] = {
                    'time': split_line[0],
                    'oid': m.group(3)
                }
                assert(self.res[metadata]['attempt_record']['source'] == m.group(1))
            else:
                raise Exception("Publication record appears before attempt record")

        elif 'Publication failed for metadata:' in line:
            pattern = "([\w.]+): Publication failed for metadata: ([\w.]+)"
            m = re.search(pattern, line)
            metadata = m.group(2)

            if metadata in self.res:
                self.res[metadata]['publication_record'] = {
                    'time': split_line[0],
                    'oid': "None"
                }
                assert(self.res[metadata]['attempt_record']['source'] == m.group(1))
            else:
                raise Exception("Publication fail record appears before attempt record")


    def write_output(self):
        for metadata, record in self.res.items():
            self.write_line_to_log(f"############# Object record. Metadata: {metadata} #############")
            self.write_line_to_log(f"Source: {record['attempt_record']['source']}")
            self.write_line_to_log(f"Attempt start time: {record['attempt_record']['time']}")
    
            # if 'publication_record' in record:
            self.write_line_to_log(f"Callback time: {record['publication_record']['time']}")
            self.write_line_to_log(f"OID: {record['publication_record']['oid']}")
        
            self.write_line_to_log('\n')


class ConsumerLogReader(LogReader):
    def __init__(self, file_name):
        super().__init__(file_name, f"consumer_log_{file_name}.txt")
        self.res = {}
        self.file_name = file_name

    def read_line(self, line):
        if 'Received oid for metadata' in line:
            pattern = "Received oid for metadata ([\w.]+) ([\w.]+)"

            m = re.search(pattern, line)
            metadata = int(m.group(1))
            oid = m.group(2)

            if metadata in self.res:
                self.res[metadata]['sent_count'] += 1
            else:
                self.res[metadata] = {}
                self.res[metadata]['sent_count'] = 1
                self.res[metadata]['oid'] = oid
        
        elif 'Data integrity check result for oid' in line:
            pattern = "Data integrity check result for oid: ([\w.]+) res: ([\w.]+) metadata ([\w.]+)"
            m = re.search(pattern, line)

            metadata = int(m.group(3))
            oid = m.group(1)

            if metadata in self.res:
                self.increment_or_initialise_count(self.res[metadata], 'response_count')

                if m.group(2) == 'True':
                    self.increment_or_initialise_count(self.res[metadata], 'success_count')
                else:
                    self.increment_or_initialise_count(self.res[metadata], 'fail_count')

            else:
                raise Exception(f"Integrity check for unknown metadata: {line}")

            assert(oid == self.res[metadata]['oid'])

    def write_output(self):
        table = []
        total_sent = 0
        total_succesful_count = 0
        total_fail_count = 0
        
        for metadata, record in dict(sorted(self.res.items())).items():
            table.append([
                metadata,                
                record.get('sent_count', 0),                
                record.get('success_count', 0),
                record.get('fail_count', 0),
                record.get('response_count', 0),
                record.get('oid', 'empty'),
            ])
            total_sent += record.get('sent_count', 0)
            total_succesful_count += record.get('success_count', 0)
            total_fail_count += record.get('fail_count', 0)

        self.write_line_to_log(tabulate(table, headers=['Metadata', 'Sent count', 'Success count', 'Fail count', 'Response count', 'OID']))
        self.write_line_to_log("\n")

        self.write_line_to_log(f"Total interests sent: {total_sent}")
        self.write_line_to_log(f"Total successfully verified requests: {total_succesful_count}")
        self.write_line_to_log(f"Total unverified requests: {total_fail_count}")

        np_table = np.array(table).T

        fig = plt.figure()
        plt.plot(np_table[1].astype(float))
        plt.xlabel('Metadata Values')
        plt.ylabel('Number of requests sent')

        # plt.yscale('log')
        # plt.gca().invert_yaxis()
        # plt.autoscale(enable=True, axis='both', tight=None)
        # import pdb; pdb.set_trace()
        plt.savefig(f"{LOGS_FOLDER}/{self.file_name}/request_plot.png")


    def increment_or_initialise_count(self, dictionary, key):
        if key in dictionary:
            dictionary[key] += 1
        else:
            dictionary[key] = 1


def main():
    log_name = sys.argv[1]
    log = open(f"{LOGS_FOLDER}/{log_name}/{log_name}.txt", 'r')
    
    # LOGS_FOLDER += f"/{log_name}"

    ml = MagicLogReader(log_name)
    cl = CacheLogReader(log_name)
    pl = ProducerLogReader(log_name)
    conl = ConsumerLogReader(log_name)

    for line in log:
        if not line:
            break

        split_line = line.split()

        if 'ndn-cxx.nfd.Magic' in line:
            ml.read_line(line)
        elif 'ContentStore' in line:
            cl.read_line(line)
        elif 'DWebProducer' in line:
            pl.read_line(line)
        elif 'DWebConsumer' in line:
            conl.read_line(line)



LOGS_FOLDER = "logs"

if __name__ == '__main__':
    main()