#!/bin/bash
Cols=256
BlocksSponge=12
syncs=2
kLen=64

echo "Testing Lyra2 with nCols = $Cols, with T and R variable."
echo "Start time: "
date -u "+%d/%m/%Y %H:%M:%S"
echo " "

export OMP_PLACES=CORES
cd ..
for sponge in 0 1   # 0 = Blake     1 = BlaMka
do
    for parallelism in 1 2 4
    do
        make clean
        make linux-x86-64-sse2 nCols=$Cols bSponge=$BlocksSponge Sponge=$sponge nSigma=$syncs nThreads=$parallelism Bench=1
        for t in 1 2 3 4 5
        do
                for r in 2048 4096 8192 16384 32768 65536 131072
                do
                        for i in 1 2 3 4 5 6
                        do
                                ../bin/Lyra2 Lyra2Sponge saltsaltsaltsalt $kLen $t $r
                        done		
                done
        done
    done
done

echo "End time: "
date -u "+%d/%m/%Y %H:%M:%S"
