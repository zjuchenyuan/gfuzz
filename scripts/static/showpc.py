# given a version and pc, show the code lines around this pc
from runscinfo import *

if __name__ == "__main__":
    if "0x" in sys.argv[1] and len(sys.argv)==2:
        version, pc = sys.argv[1].split("0x")
        pc = "0x"+pc
    else:
        version = sys.argv[1]
        pc = sys.argv[2]
    r = RUNSC_wrapper(version)
    fpath,funcname, (filepath, startline, startcol, endline, endcol, instrumented_line) = r.pc2info(pc)
    codelines = open(r.rfolder+"/"+filepath.replace("/","#")).readlines()
    C=int(os.environ.get("C", 5))
    for lineno in range(instrumented_line-C,instrumented_line+C):
        print(lineno+1, codelines[lineno], end="")
    print()
    print(fpath, instrumented_line)
