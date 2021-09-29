import sys
commit = sys.argv[1]
for line in open("lastbuild.log"):
    if not (line.startswith("WORK=") or line.startswith("cd ") or "llvm-goc -c" in line):
        continue
    if "llvm-goc -c" in line:
        l = line.replace(" -O2 ", " -O0 ").split()
        name = [i for i in l if i.startswith("-fgo-pkgpath=")]
        if name:
            name = name[0].replace("-fgo-pkgpath=", "")
        else:
            name = "main"
        l.insert(1, "-S")
        l.insert(1, "-emit-llvm")
        l[l.index("-o")+1] = "/out."+commit+"/"+name+".ll"
        #print(l)
        print("mkdir -p /out."+commit+"/"+"/".join(name.split("/")[:-1]))
        #line = " ".join(l)+" \n"
        line = " ".join(l)+" &\n"
    print(line, end="")
