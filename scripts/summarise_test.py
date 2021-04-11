from os import listdir
from os.path import isfile, join, isdir
import sys
import os
import re
import mmap
import pandas as pd


def main():
    folder_name = sys.argv[1]
    path = f"{LOGS_FOLDER}/{folder_name}"

    result = []

    for f in listdir(f"{LOGS_FOLDER}/{folder_name}"):
        # loop through log files and create row entries in the csv

        if isdir(join(path, f)):
            hop_count = read_hop_count(path, f)
            cache_summary = read_cache_summary(path, f)

            new_row = [f, hop_count]
            new_row.extend(cache_summary)
            result.append(new_row)
   
    to_csv(result, folder_name)


def read_hop_count(path, folder_name):
    log = open(f"{path}/{folder_name}/hop_count.txt", 'r')

    for line in log:
        if not line:
            break

        pattern = "Average hop count: ([0-9.]+)"
        m = re.search(pattern, line)
        
        if m:
            hop_count = m.group(1)
            return float(hop_count)
        
        return -1


def read_cache_summary(path, folder_name):
    log = open(f"{path}/{folder_name}/cache_log_{folder_name}.txt", 'r+')

    data = mmap.mmap(log.fileno(), 0)

    pattern = """Hit ratio: ([0-9.]+)
Miss ratio: ([0-9.]+)

Hit count: ([0-9.]+)
Miss count: ([0-9.]+)
Insert count: ([0-9.]+)
Evict count: ([0-9.]+)"""
    m = re.search(pattern.encode(), data)
    
    if m:
        hit_ratio = m.group(1)
        insert_count = m.group(5)
        evict_count = m.group(6)
        return [float(hit_ratio), float(insert_count), float(evict_count)]
    
    return [-1, -1, -1]


def to_csv(rows, file_name):
    table = []

    for row in rows:
        pattern = "([a-zA-Z_]+)_([0-9_]+)"
        
        m = re.search(pattern, row[0])

        if m:
            new_row = [m.group(1), float(m.group(2).replace('_', '.'))]
            new_row.extend(row[1:])
            table.append(new_row)

    df = pd.DataFrame(table)

    df.to_csv(f'{SUMMARY_FOLDER}/summary_{file_name}.csv', index=False, header=['strategy', 'cache_budget', 'hop_count', 'hit_ratio',
                                                'insert_count', 'evict_count'])


def to_results_csv(rows):
    """
    Generates an output csv which replicates the excel structure with columns
    test_no,strategy,q,s,cache_budget,initial_objects,magic_seconds,hit_ratio,hop_count
    """
    table = []

    for row in rows:
        pattern = "([a-zA-Z]+)_([0-9_]+)"
        
        m = re.search(pattern, row[0])

        if m:
            new_row = ['None', m.group(1), 0.7, 0.7, float(m.group(2).replace('_', '.')), 5, 15]
            new_row.append(row[2])
            new_row.append(row[1])
            table.append(new_row)

    df = pd.DataFrame(table)
    df = df.sort_values(4)
    df.to_csv('results167.csv', index=False)    


LOGS_FOLDER = 'logs'
SUMMARY_FOLDER = 'test_summaries'


if __name__ == '__main__':
    main()