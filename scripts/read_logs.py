import sys
import re
import collections
from tabulate import tabulate
import matplotlib.pyplot as plt
import numpy as np

from scipy.stats import pearsonr

class LogReader:
    def __init__(self, base_log_name, file_name):
        self.output_file = open(f"{LOGS_FOLDER}/{base_log_name}/{file_name}", 'w+')

    def __del__(self):
        self.write_output()

    def write_output(self):
        pass

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
        self.file_name = file_name
        self.insert_count = 0
        self.evict_count = 0

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

        elif 'insert' in line:
            self.insert_count += 1

        elif 'Evicting' in line:
            self.evict_count += 1

    def write_output(self):
        self.write_line_to_log(f"####################### Summary ########################")
        total = self.miss_count + self.hit_count

        total = 1 if total == 0 else total

        self.write_line_to_log(f"Hit ratio: {(self.hit_count / total) * 100}")
        self.write_line_to_log(f"Miss ratio: {(self.miss_count / total) * 100}\n")
        self.write_line_to_log(f"Hit count: {self.hit_count}")
        self.write_line_to_log(f"Miss count: {self.miss_count}")
        self.write_line_to_log(f"Insert count: {self.insert_count}")
        self.write_line_to_log(f"Evict count: {self.evict_count}")


        self.write_line_to_log(f"####################### Cache log ########################")

        for record in self.res:
            self.write_line_to_log(f"{record[0]}: {record[1]} {record[3]} {record[2]}")

    def update_hit_ratio(self, line, hit):
        pattern = "/([\w.]+)/%FE%"
        
        m = re.search(pattern, line)
        oid_length = 128 


        if m and len(m.group(1)) == oid_length:
            if hit:
                self.hit_count += 1
            else:
                self.miss_count += 1

    
    def calculate_per_object_hit_ratio(self):
        oid_to_record = {}

        for val in self.res:
            oid = val[2]

            if oid not in oid_to_record:
                oid_to_record[oid] = [0, 0]

            if val[3] == 'matched':
                oid_to_record[oid][0] += 1
            else:
                oid_to_record[oid][1] += 1

        for key, val in oid_to_record.items():
            oid_to_record[key] = val[0] / (val[0] + val[1])

        return oid_to_record

    def get_popularity_vs_cache_hit(self, req_history):
        oid_to_hit = self.calculate_per_object_hit_ratio()
        hit_popularity_pairs = []

        for hist in req_history:
            oid = f"/{hist[5]}/%FE%00"

            if oid in oid_to_hit:
                # [total requests, hit ratio]
                hit_popularity_pairs.append([hist[2], oid_to_hit[oid]])

        self.hit_popularity_pairs = np.array(hit_popularity_pairs)

        return self.hit_popularity_pairs

    def plot_popularity_vs_cache_hit(self, req_history):
        hit_popularity_pairs = self.get_popularity_vs_cache_hit(req_history)

        fig = plt.figure()
        plt.grid()

        popularity_values = hit_popularity_pairs.T[0].astype(float)
        min_pop = min(popularity_values)
        max_pop = max(popularity_values)
        normalised_popularity = (popularity_values - min_pop) / (max_pop - min_pop)

        plt.scatter(normalised_popularity, hit_popularity_pairs.T[1].astype(float))
        plt.xlabel('Normalised request count')
        plt.ylabel('Cache hit ratio')
        plt.title("Hit ratio vs popularity: LRU 1% cache budget")

        plt.savefig(f"{LOGS_FOLDER}/{self.file_name}/cache_plot.png")


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
            try:
                self.write_line_to_log(f"Callback time: {record['publication_record']['time']}")
                self.write_line_to_log(f"OID: {record['publication_record']['oid']}")
        
                self.write_line_to_log('\n')
            except Exception:
                print(f"No successful publication record for attempt: {record['attempt_record']}")


class ConsumerLogReader(LogReader):
    def __init__(self, file_name):
        super().__init__(file_name, f"consumer_log_{file_name}.txt")
        self.res = {}
        self.file_name = file_name

    def read_line(self, line):
        if 'Received oid for metadata' in line:
            pattern = "Received oid for metadata ([\w.]+) ([\w.]+)"

            m = re.search(pattern, line)
            try:
                metadata = int(m.group(1))
                oid = m.group(2)
            except Exception:
                print(f"Error parsing line: {line}")
            else:
                if metadata in self.res:
                    self.res[metadata]['sent_count'] += 1
                else:
                    self.res[metadata] = {}
                    self.res[metadata]['sent_count'] = 1
                    self.res[metadata]['oid'] = oid
        
        elif 'Data integrity check result for oid' in line:
            pattern = "Data integrity check result for oid: ([\w.]+) res: ([\w.]+) metadata ([\w.]+)"
            m = re.search(pattern, line)

            if m:
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

                # assert(oid == self.res[metadata]['oid'])

    def write_output_and_save(self):
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

        self.np_table = np.array(table).T

        fig = plt.figure()
        plt.plot(self.np_table[1].astype(float))
        plt.xlabel('Metadata Values')
        plt.ylabel('Number of requests sent')

        plt.savefig(f"{LOGS_FOLDER}/{self.file_name}/request_plot.png")
        return self.np_table.T


    def increment_or_initialise_count(self, dictionary, key):
        if key in dictionary:
            dictionary[key] += 1
        else:
            dictionary[key] = 1


class AppTraceReader(LogReader):
    def __init__(self, file_name, app_trace_file):
        super().__init__(file_name, f"hop_count.txt")
        self.res = {}
        self.app_trace_file = app_trace_file
        self.hop_sum = 0
        self.count = 0
        self.file_name = file_name

        self.retx = 0

    def read_line(self, line):
        split_line = line.split()

        try:
            self.hop_sum += int(split_line[8])
            self.count += 1
            self.retx += int(split_line[7])
        except Exception:
            pass

    def run_complete_pass(self):
        trace = open(f"{LOGS_FOLDER}/{self.file_name}/{self.app_trace_file}.txt", 'r')

        for line in trace:
            if not line:
                break
            
            self.read_line(line)

    def write_output(self):
        average = -1

        if self.count > 0:
            average = self.hop_sum / self.count

        self.write_line_to_log(f"Average hop count: {average}")
        self.write_line_to_log(f"Average retx count: {self.retx / self.count}")



def read_log(log_name):

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

    req_hist = conl.write_output_and_save()
    cl.plot_popularity_vs_cache_hit(req_hist)

    atr = AppTraceReader(log_name, "app-delays-trace")
    atr.run_complete_pass()

    return cl


def polyfit(x, y, degree=1):
    results = {}

    coeffs = np.polyfit(x, y, degree)
    p = np.poly1d(coeffs)

    results['determination'], _ = pearsonr(x, y)
    results['fit'] = p

    return results

def compare_cache_plots(file_name, dataset_1, dataset_2, label_1, label_2):
    fig = plt.figure()
    plt.grid()

    def normalise_popularity(dataset):
        popularity_values = dataset.T[0].astype(float)
        min_pop = min(popularity_values)
        max_pop = max(popularity_values)
        normalised_popularity = (popularity_values - min_pop) / (max_pop - min_pop)

        return normalised_popularity
    
    normalised_popularity_1 = normalise_popularity(dataset_1)
    normalised_popularity_2 = normalise_popularity(dataset_2)

    plt.scatter(normalised_popularity_1, dataset_1.T[1].astype(float), c='r', label=label_1, marker='x')
    plt.scatter(normalised_popularity_2, dataset_2.T[1].astype(float), edgecolors='b', label=label_2, marker='^', facecolors='none')
    plt.xlabel('Popularity (Normalised request count)')
    plt.ylabel('Cache hit ratio')
    plt.title("Hit ratio vs popularity: LCE and MAGIC (1% Cache budget)")
    
    fit_1 = polyfit(normalised_popularity_1, dataset_1.T[1].astype(float))
    fit_2 = polyfit(normalised_popularity_2, dataset_2.T[1].astype(float))

    plt.plot(normalised_popularity_1, fit_1['fit'](normalised_popularity_1), c='r', linestyle='--', label=f"{label_1} linear fit ($R^2={fit_1['determination']:.2f}$)")
    plt.plot(normalised_popularity_2, fit_2['fit'](normalised_popularity_2), c='b', linestyle='--', label=f"{label_2} linear fit ($R^2={fit_2['determination']:.2f}$)")
    
    
    plt.legend()
    plt.savefig(f"images/comparison.png")


LOGS_FOLDER = "logs"

if __name__ == '__main__':
    if len(sys.argv) == 2:
        log_name = sys.argv[1]
        read_log(log_name)
    elif len(sys.argv) == 3:
        log_1 = sys.argv[1]
        log_1_cl = read_log(log_1)

        log_2 = sys.argv[2]
        log_2_cl = read_log(log_2)

        compare_cache_plots(' ', log_1_cl.hit_popularity_pairs, log_2_cl.hit_popularity_pairs, 'MAGIC', 'LCE (LRU eviction)')


