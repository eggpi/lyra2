#! /usr/bin/env python

import sys

class measure:
    def __init__ (self):
        self.t = 0
        self.r = 0
        self.c = 0
        self.parrallelism = 0
        self.syn = 0
        self.sponge = ""
        self.memory = 0
        self.measures = []
        self.average = 0.0

    def calcAverage(self):
        for m in self.measures:
            self.average = self.average + float(m)
        self.average = self.average / len(self.measures)
        self.average = self.average / 1000000.0

    def toString(self):
        return (str(self.t) + "|" + str(self.r) + "|" + str(self.c) + '|' + str(self.parrallelism) + "|" + str(self.syn) + "|" + self.sponge + "|" + str(self.memory) + "|" + str(self.measures) + "|" + str(self.average)).replace("[", "").replace(", ", "|").replace("]", "").replace("L", "")


def main(argv=None):
    if(len(sys.argv) != 2):
        print('Usage: ./parser.py fileName')
    else:
        f = open(sys.argv[1])

        line = f.readline()
        
        m = measure()
        mList = []

        count = 0
        samples = 6
        
        print ("T | R | C | Parallelism | Synchronism | Sponge | Memory | Execution Time")

        while line != '':
            line = f.readline() 
            if 'T:' in line:
                m.t = long(line.split('T: ')[1])
            if 'R:' in line:
                m.r = long(line.split('R: ')[1])               
            if 'C:' in line:
                m.c = long(line.split('C: ')[1]) 
            if 'Parallelism:' in line:
                m.parrallelism = long(line.split('Parallelism: ')[1]) 
            if 'Synchronism (used just if Parallelism > 1):' in line:
                m.syn = long(line.split('Synchronism (used just if Parallelism > 1): ')[1]) 
            if 'Sponge:' in line:
                m.sponge = line.split('Sponge: ')[1].rstrip('\n')
            if 'Sponge Blocks (bitrate):' in line:
                m.spongBlocks = long(line.split('Sponge Blocks (bitrate): ')[1].split(" =")[0]) 
            if 'Memory:' in line:
                m.memory = long(line.split('Memory: ')[1].split(" bytes")[0]) 
            if 'Execution Time:' in line:
                time = line.split(": ")[1].split(" us")[0]
                m.measures.append(long(time))
                count = count + 1
                if count == samples:
                    m.calcAverage()
                    mList.append(m)
                    m = measure()
                    count = 0

        for a in mList:
            print(a.toString())

if __name__ == "__main__":
    main()

