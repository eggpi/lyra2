#
# This file is part of Lyra2, a password hashing scheme
# Copyright (c) 2013-2014 by Lyra2 Project - - <http://www.lyra2.net/>
#

CC?=gcc
NVCC=nvcc
CFLAGS=-std=c99 -Wall -pedantic -O3 -msse2 -funroll-loops -march=native -Ofast -Wno-unused-variable
#CFLAGS=-std=c99 -Wall -pedantic -msse2 -fopenmp -funroll-loops -O0 -g -Wno-unused-variable #valgrind

BINDIR=../bin
BIN=$(BINDIR)/Lyra2
BINCUDA=$(BINDIR)/Lyra2CUDA
nCols=256
bSponge=12
Sponge=0
nThreads=2
nSyncs=2
Bench=0

parameters=-DN_COLS=$(nCols) -DBLOCK_LEN_INT64=$(bSponge) -DSPONGE=$(Sponge) -DnPARALLEL=$(nThreads) -DSIGMA=$(nSyncs) -DBENCH=$(Bench)

SSEDIR=sse/

# CUDA Flags
PTX = 10  # Must suits the GPU board used.
SM = 20   # Must suits the GPU board used.
CUDADIR=cuda/

default:
	
	@echo " "
	@echo "To build Lyra2, type:"
	@echo "      make OPTION [nCols=(number of columns, default 256)] [nThreads=(number of threads, default 2)] [nSyncs=(numbers of synchronism, obs.: just in parallel version of Lyra2, default 2)] [bSponge=(8 or 10 or 12, default 12)] [Sponge=(0 or 1, default 0) 0 means Bleak2b and 1 means BlaMka]"
	@echo " "
	@echo "where OPTION can be one of the following:"
	@echo "generic-x86-64                      For x86-64 Unix-like system, with gcc (i.e., Linux, FreeBSD, Mac OS, etc.)"
	@echo "linux-x86-64-sse2                   Linux x86-64, with SSE2 (beta version, needs improvement)"
	@echo "windows-x86-64                      Windows x86-64, Cygwin"
	@echo "win-cygwin-x86-64-sse2              Windows x86-64, Cygwin, with SSE2 (beta version, needs improvement)"
	@echo " "
	@echo "Notes:"
	@echo "Lyra2 was tested with nCols=16, nCols=32, nCols=64, nCols=96, nCols=128, nCols=256, nCols=512, nCols=1024 and nCols=2046"
	@echo "This implementation is known to have issues for large memory usage. The maximum tested memory usage was 1.6 GB "
	@echo " "
	@echo "***When using OpenMP 3.1, execute 'export GOMP_CPU_AFFINITY=0,2,4,6,1,3,5,7' to define affinity in parallel version of Lyra2! (verify computer architecture!!)***"
	@echo "***When using OpenMP 4, execute 'export OMP_PLACES=CORES' to define affinity in parallel version of Lyra2! ***"


generic-x86-64:	    Lyra2.c Sponge.c Main.c Lyra2.h Sponge.h
	mkdir -p $(BINDIR)	
	$(CC) $(CFLAGS) Sponge.c Lyra2.c Main.c -o $(BIN) $(parameters)
	@echo "Build completed, binaries in $(BIN)"

linux-x86-64-sse2:	$(SSEDIR)Lyra2.c $(SSEDIR)Sponge.c $(MAINC) $(SSEDIR)Lyra2.h $(SSEDIR)Sponge.h
	mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(SSEDIR)Sponge.c $(SSEDIR)Lyra2.c $(MAINC) -o $(BIN) $(parameters)
	@echo "Build completed, binaries in $(BIN)"
	
#linux-x86-64-cuda:  $(CUDADIR)Lyra2.cu $(CUDADIR)Sponge.cu $(CUDADIR)Main.cu $(CUDADIR)Lyra2.h $(CUDADIR)Sponge.h
#	mkdir -p $(BINDIR)
#	$(NVCC) $(CUDADIR)Lyra2.cu $(CUDADIR)Sponge.cu $(CUDADIR)Main.cu -O3 -o $(BINCUDA) -lcudart -DN_COLS=$(nCols)
#	@echo "Build completed, binaries in $(BINCUDA)"

#linux-x86-64-cuda-tested-arch:	$(CUDADIR)Lyra2.cu $(CUDADIR)Sponge.cu $(CUDADIR)Main.cu $(CUDADIR)Lyra2.h $(CUDADIR)Sponge.h
#	mkdir -p $(BINDIR)
#	$(NVCC) $(CUDADIR)Lyra2.cu $(CUDADIR)Sponge.cu $(CUDADIR)Main.cu -O3  -arch=compute_$(PTX)  -code=sm_$(SM) -o $(BINCUDA) -lcudart -DN_COLS=$(nCols)
#	@echo "Build completed, binaries in $(BINCUDA)"
	
windows-x86-64:	    Lyra2.c Sponge.c Main.c Lyra2.h Sponge.h
	mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) Sponge.c Lyra2.c Main.c -o $(BIN) $(parameters)
	@echo "Build completed, binaries in $(BIN)"

win-cygwin-x86-64-sse2:	$(SSEDIR)Lyra2.c $(SSEDIR)Sponge.c Main.c $(SSEDIR)Lyra2.h $(SSEDIR)Sponge.h
	mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) $(SSEDIR)Sponge.c $(SSEDIR)Lyra2.c Main.c -o $(BIN) $(parameters)
	@echo "Build completed, binaries in $(BIN)"
	
clean:
	rm -rf *.o $(BIN) $(BINCUDA) $(BINDIR)


