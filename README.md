# State Fuzzing

This branch is intended to be used for savestate fuzzing. LLVM's LibFuzzer is used, documentation can be found here: https://llvm.org/docs/LibFuzzer.html

# How to fuzz

1. Setup standard gambatte build environment on MacOS or Linux, along with clang. Currently, LibFuzzer is unavailable on Windows. Windows users are recommended to use WSL2 for fuzzing.

2. cd to /libgambatte, make, mkdir CORPUS_DIR, copy savestates to CORPUS_DIR

3. ./gambatte CORPUS_DIR -workers=n -jobs=n, n is how many threads you wish to give the fuzzer
