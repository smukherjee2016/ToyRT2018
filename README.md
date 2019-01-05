# ToyRT2018

Just another ray tracing engine written in C++. Tested on Windows and Linux(Ubuntu/Linux Mint only).

## Dependencies
The followings need to exist on the host machine:
[Embree](https://embree.github.io/downloads.html)

## Execution

Clone the repository alongwith its submodules:

```
git clone --recursive https://github.com/smukherjee2016/ToyRT2018
```

If you have cloned without the ``` --recursive``` flag, please run the following:

```
git submodule update --init --recursive
```

Then it should build and run like a standard CMake project.

### Linux

There is an extra step needed for Linux: for Ubuntu/Linux Mint/Debian(?),  you need to add /usr/lib64 to LD_LIBRARY_PATH so the executable finds the embree .so. [This](https://stackoverflow.com/a/13428971) has been tested to work.

## TODO
- [x] Ray-sphere intersection
- [x] Path Tracing with Next Event Estimation
- [x] Direct lighting
- [x] Path Tracing with MIS
- [ ] Ray-triangle intersection
- [ ] OBJ loading
- [ ] Use Embree for Ray-triangle intersection
- [ ] Replace OpenMP with threadpool or custom threading
- [x] Emitter selection heuristic
- [x] Discrete CDF class for use with emitter sampling
- [ ] Remove static version of Sampler
- [ ] Photon Mapping integrator

## License

Apache 2.0.
