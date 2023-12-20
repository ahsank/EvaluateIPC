To run benchmarks

Using docker
============

From root of this repo

```console
./bin/rundocker --build
./bin/rundocker
```

Inside the docker container

```console
/build# cmake -DCMAKE_BUILD_TYPE=Release /workspace
```

Or

```console
/build# /workspace/bin/runcmakeindocker.sh
```

Then

```console
/build# make
/build# ./tests/mutexbench
/build# ./tests/boostbench
```

etc

From Local Ubuntu machine
================

Run

```console
./docker/cmake/install-dependencies.sh
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
./tests/mutexbench
```
