
f = open('out15.txt', 'r')

def read_line(line):
  
    line = line.split()

    return (line[4], line[8], line[10])

res = {}


for line in f:
    if not line:
        break
    try:
        if "return" in line:
            line = line.split()
            node = line[4]
            nonce = line[5]

            if nonce not in res:
                res[nonce] = {}
                res[nonce]['req'] = line[12]
            
            if 'return' not in res[nonce]:
                res[nonce]['return'] = []

            res[nonce]['return'].append((node, line[8], line[10]))

        elif "outbound" in line:
            line = line.split()
            node = line[4]
            nonce = line[5]

            if nonce not in res:
                res[nonce] = {}
                res[nonce]['req'] = line[12]
            
            if 'req_path' not in res[nonce]:
                res[nonce]['req_path'] = []
            # import pdb; pdb.set_trace()
            res[nonce]['req_path'].append((node, line[8], line[10]))
    except Exception:
        import pdb; pdb.set_trace()
        


for nonce, paths in res.items():
    if len(paths.get('req_path', [])) < 4 or len(paths.get('req_path', [])) != len(paths.get('return', [])):
        continue

    print(f"############################ Nonce: {nonce} ############################")
    print(f"Request: {paths.pop('req')}")
    for direction, path in paths.items():
        if direction == 'req_path':
            print("Request Path")
        else:
            print("Return Path")

        for hop in path:
            print(f"{hop[0]} Max gain {hop[1]} Local gain {hop[2]}")


# if __name__ == '__main__':