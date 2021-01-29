
f = open('../output.txt', 'r')

res = {}

for line in f:
    if not line:
        break

    if "return" in line:
        line = line.split()
        nonce = line[4]

        if nonce not in res:
            res[nonce] = {}
        
        if 'return' not in res[nonce]:
            res[nonce]['return'] = []

        res[nonce]['return'].append((line[7], line[9]))

    elif "outbound" in line:
        line = line.split()
        nonce = line[4]

        if nonce not in res:
            res[nonce] = {}
        
        if 'req' not in res[nonce]:
            res[nonce]['req'] = []
        # import pdb; pdb.set_trace()
        res[nonce]['req'].append((line[7], line[9]))
        


for nonce, paths in res.items():
    print(f"############################ Nonce: {nonce} ############################")
    
    for direction, path in paths.items():
        if direction == 'req':
            print("Outbound")
        else:
            print("Inbound")

        for hop in path:
            print(f"max {hop[0]} curr {hop[1]}")


# if __name__ == '__main__':