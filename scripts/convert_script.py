import sys

def read_topology(topology_file):
    file = open(f'../topologies/{topology_file}.txt', 'r') 

    max_id = 0
    pairs = []

    while True: 
        line = file.readline() 
    
        if not line: 
            break

        split_line = line.split()

        if int(split_line[0]) > max_id:
            max_id =  int(split_line[0])

        if int(split_line[1]) > max_id:
            max_id = int(split_line[1])

        pairs.append((int(split_line[0]), int(split_line[1])))

    return max_id, pairs


def write_topology(connections, max_id, topology_file):
    output_file = open(f'../topologies/{topology_file}_converted.txt', 'w+')

    
    output_file.write("router\n")

    for i in range(max_id + 1):
        output_file.write(f"Node{i}   NA          0       0\n")

    output_file.write("\n")
    output_file.write("link\n")

    for connection in connections:
        output_file.write(f"Node{connection[0]}       Node{connection[1]}       {BANDWIDTH}Mbps       1       {LATENCY}ms    {QUEUE}\n")


def main():
    topology_file = sys.argv[1]

    max_node, connections = read_topology(topology_file)
    write_topology(connections, max_node, topology_file)
    

NODE_PREFIX = 'Node'
BANDWIDTH = 100
LATENCY = 10
QUEUE = 100


if __name__ == '__main__':
    main()