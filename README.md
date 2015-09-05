# Lyra2

An implementation of the [Lyra2](http://www.lyra-kdf.net/) key derivation
function targeting modern CPU architectures and compilers. This is an
alternative to the [reference implementation](https://github.com/leocalm/Lyra)
aiming at:

- Helping debug the specification with an independent implementation;
- Exploring the speedups provided by SIMD instruction sets;
- Being presented as my final graduation project at university ;)

## Building and running

Just type:

    $ make

and you should have a binary named `lyra2` in the current directory.  When
executed, that binary will run some performance tests (see `src/main.c`) with
different parameters for the same input.

The `ref` directory contains an updated reference implementation for comparison
purposes. Use the `benchmark.py` script to build both implementations and
compare their speeds:

    $ ./scripts/benchmark.py

This will perform the same tests as above for both this and the reference
implementation and output their relative speed.
