import sys
import numpy as np
import math
import random

def random_points(node_count):

    total_area = node_count * (1 / TARGET_DENSITY)
    square_dim = math.sqrt(total_area)
    
    positions = []

    centre = square_dim / 2

    # x = np.random.normal(centre, centre * 0.7, node_count)
    # y = np.random.normal(centre, centre * 0.7, node_count)
    x = np.random.uniform(0,square_dim,node_count)
    y = np.random.uniform(0,square_dim,node_count)

    return np.array([x, y]).T


def get_closest(points, point, ignore_indices={}):
    dist = lambda a: np.linalg.norm(a - point)


    min_dist = 999999
    min_index = -1

    for i in range(len(points)):
        if i not in ignore_indices:
            distance = dist(points[i])

            if distance < min_dist:
                min_dist = distance
                min_index = i

    return min_index

def connect_points(points):
    adjacency_list = [[] for i in range(len(points))]
    connected = set()
    random.seed(0)

    for i in range(len(points)):
        point = points[i]

        # edges = random.randint(1,4)
        for j in range(EDGES_PER_NODE):
            if j == 0:
                connected.add(i)
                closest = get_closest(points, point, connected)
                if closest == -1:
                    continue
                adjacency_list[i].append(closest)
            else:
                closest = get_closest(points, point, set([i] + adjacency_list[i]))
                if closest == -1:
                    continue
                adjacency_list[i].append(closest)

    return adjacency_list


def write_topology(connections, points, topology_file):
    output_file = open(f'../topologies/{topology_file}.txt', 'w+')

    output_file.write("router\n")

    max_id = len(connections)

    for i in range(max_id):
        # output_file.write(f"Node{i}   NA          {int(points[i][0] * 10)}       {int(points[i][1] * 10)}\n")
        output_file.write(f"Node{i}   NA          0       0\n")

    output_file.write("\n")
    output_file.write("link\n")

    for i in range(max_id):
        connection = connections[i]

        for val in connection:
            output_file.write(f"Node{i}       Node{val}       {BANDWIDTH}Mbps       1       {LATENCY}ms    {QUEUE}\n")


def main():
    node_count = int(sys.argv[1])
    topology_file = sys.argv[2]

    # import pdb; pdb.set_trace()
    points = random_points(node_count)
    connections = connect_points(points)

    write_topology(connections, points, topology_file)



EDGES_PER_NODE = 2
TARGET_DENSITY = 10 # nodes per unit area
NODE_PREFIX = 'Node'
BANDWIDTH = 100
LATENCY = 10
QUEUE = 100


if __name__ == '__main__':
    main()