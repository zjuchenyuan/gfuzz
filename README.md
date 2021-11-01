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

```
cd /g
./scripts/build/build_syzkaller.sh
```

This will build our modified syzkaller in `/g/syzkaller`, which contains many gfuzz modifications.

### 2. Build target gVisor to LLVM IR, and get global CFG

Here we take gVisor version `release-20210125.0` as an example.

Also we provide a Dockerfile for building and analyzing, see [here](./dockerfiles/Dockerfile.buildstatic).

```
docker build -t gfuzz:release-20210125.0 -f dockerfiles/Dockerfile.buildstatic
```

After this build, you will get a folder inside the image `/g/gvisor_bin/release-20210125.0`, which includes these files:

- edge.txt: the edge relationship given by `go-callvis`, using default [pointer analysis](https://pkg.go.dev/golang.org/x/tools/go/pointer)
- rtaedge.txt: the edge relationship given by our modified `go-callvis`, using [Rapid Type Analysis](https://pkg.go.dev/golang.org/x/tools/go/callgraph/rta), see also our [patch file](./scripts/build/gocallvis_rta.patch)
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

We take general experiment setting as an example, to show you how to generate conf files for fuzzing.

24 targets in version `release-20210125.0` are involved in the general experiment, and the corresponding pc list is given in the [data.py](./fuzz_experiments/general/data.py).

```
cd /g/fuzz_experiments/general
python3 data.py
```

This will conduct several analysis towards the target, and generate these files:

- \*\_bbdis.json: bb level distance of each reachable pc
- \*\_cgdis.json: function level distance of each reachable pc
- \*\_aflgodis.json: use aflgo distance calculation method to estimate the distance
- \*\_syscalls.json: inferred syscalls for the target

> If you want to check the pc meaning, try this:
>     python3 /g/scripts/static/showpc.py release-20210125.0 0x4dd0000

### 4. Generate fuzzing configuration

Next step is generate `*.conf` files for fuzzing, and we need to write the template and `genconfig.py`.

You can related files for general fuzzing here: [./fuzz_experiments/general](./fuzz_experiments/general). 

This [genconfig.py](./fuzz_experiments/general/genconfig.py) will generate 6 batch experiment scripts, and in each batch, 4 targets are tested with 4 repetitions and 2 different settings (gfuzz and aflgo), with a baseline setting (origin) 4 repetitions. To conduct 20 repetitions, you may run each `start.sh` on 5 different machines, and each machine should have at least 36 CPUs available.

```
cd /g/fuzz_experiments/general
python3 data.py
python3 genconfig.py
```

After running this python script, these files will be generated: (`M`, `N`, `BATCH`, `VERSION` are variables, links are given to an example file)

- [M\_aflgodis.json](./fuzz_experiments/general/1_aflgodis.json): distance for each basic block using aflgo distance approximation calculation method for this target M
- [M\_bbdis.json](./fuzz_experiments/general/1_bbdis.json): distance for each basic block using gfuzz distance calculation method for this target M
- [M\_cgdis.json](./fuzz_experiments/general/1_cgdis.json): distance for each basic block using only call graph for this target M, a function have only one distance
- [M\_syscalls.json](./fuzz_experiments/general/1_syscalls.json): inferred syscalls for this target M by gfuzz static analysis
- [general\_aflgo\_M\_N.conf](./fuzz_experiments/general/general_aflgo_1_1.conf): aflgo fuzzing conf for target M and repeat N (1<=M<=24, 1=<N<=4 in this experiment)
- [general\_gfuzz\_M\_N.conf](./fuzz_experiments/general/general_gfuzz_1_1.conf): gfuzz fuzzing conf for target M and repeat N
- [general\_originBATCH\_VERSION\_N.conf](./fuzz_experiments/general/general_origin1_release-20210125.0_1.conf): baseline conf for gvisor VERSION in batch BATCH (1<=BATCH<=6, VERSION=release-20210125.0, 1<=N<=4 in this experiment)
- [start.sh](./fuzz_experiments/general/start.sh): the script to run the whole experiment batches on one machine
- [startBATCH.sh](./fuzz_experiments/general/start1.sh): the script to start the batch BATCH experiment

