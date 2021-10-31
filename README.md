# gfuzz
Directed fuzzing framework for gVisor.

## Pipeline

Note: We assume all gfuzz files placed under folder `/g` on the host machine.

### 1. Prepare environment

We use [GoLLVM](https://go.googlesource.com/gollvm/) for compiling gVisor code to a LLVM bc file.

Commands for preparing a gollvm environment are given as a [Dockerfile](./dockerfiles/Dockerfile.gollvm), and you may build it as follows:

```
git clone https://github.com/zjuchenyuan/gfuzz /g
cd /g
docker build -t zjuchenyuan/gollvm -f dockerfiles/Dockerfile.gollvm
```

This will build and install gollvm version `d30fc0bf` in the image `zjuchenyuan/gollvm`.

### 2. Build target gVisor to LLVM IR

Here we take gVisor version `release-20210125.0` as an example.

Also we provide a Dockerfile for building and analyzing, see [here](./dockerfiles/Dockerfile.buildstatic).

```
docker build -t gfuzz:release-20210125.0 -f dockerfiles/Dockerfile.buildstatic
```

After this build, you will get a folder inside the image `/g/gvisor_bin/release-20210125.0`, which includes these files:

- edge.txt: the edge relationship given by `go-callvis`, using default [pointer analysis](https://pkg.go.dev/golang.org/x/tools/go/pointer)
- rtaedge.txt: the edge relationship given by our modified `go-callvis`, using [Rapid Ttype Analysis](https://pkg.go.dev/golang.org/x/tools/go/callgraph/rta), see also our [patch file](./scripts/build/gocallvis_rta.patch)
- global_cfg.txt: basic block level control flow graph analysis result given by our `ir2cfg` component, which is used in distance calculation afterwards
- rtaglobal_cfg.txt: Same as global_cfg.txt, but rely on rtaedge.txt instead
- r.zip: packed instrumented Go files, can be used for source code level analysis
- runsc: the build binary, used for fuzzing experiment

You may copy these build files to the host machine, as we will conduct the fuzzing experiment outside the Docker container.

```
docker run -it --rm --name tmp gfuzz:release-20210125.0 /bin/bash
# in another host terminal
docker cp tmp:/g/gvisor_bin /g/
```

### 3. Distance calculation, syscall inference and so on...

