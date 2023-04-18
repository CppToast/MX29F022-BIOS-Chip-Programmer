import sys,getopt

filename = "amibios.bin"
outfilename = "commands.txt"
blocksize = 16
size = 0x3ffff

offset = 0
with open(filename,"rb") as f, open(outfilename,"w") as o:
    while True:
        allff = True
        block = f.read(blocksize)
        str = "p " + hex(offset)[2:] + " "
        for ch in block:
            if hex(ch)[2:] != "ff": allff = False
            str += hex(ch)[2:] + " "
        if not allff: o.write(str + "\n")
        offset += blocksize
        if offset > size:
            o.close()
            print("Done!")
            while True: pass
    
