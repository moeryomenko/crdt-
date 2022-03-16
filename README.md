# crdtxx

A family of CRDT's supporting both State and Op based replication.

## What?

CRDT's are the solution to highly available mutable state.

## Build

For build you need:

- [CMake](https://cmake.org/download/)
- [conan](https://docs.conan.io/en/latest/installation.html)
- gcc 10+ or clang 10+

```sh
# configure project and download dependencies.
$ cmake -GNinja -H. -Bbuild -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
# build
$ cmake --build build
# run test
$ ctest --test-dir build
```

## Usage

TODO
