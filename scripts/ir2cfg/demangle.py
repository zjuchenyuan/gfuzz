import sys
import re

t = ["gvisor.x2edev..z2fgvisor..z2fpkg..z2flog.gvisor.x2edev..z2fgvisor..z2fpkg..z2flog..init0",
"gvisor.x2edev..z2fgvisor..z2fpkg..z2flog.gvisor.x2edev..z2fgvisor..z2fpkg..z2flog..thunk0",
"gvisor.x2edev..z2fgvisor..z2fpkg..z2flog.gvisor.x2edev..z2fgvisor..z2fpkg..z2flog.Writer..eq",
"gvisor.x2edev..z2fgvisor..z2fpkg..z2ftcpip..z2fnetwork..z2fipv4..1gvisor.x2edev..z2fgvisor..z2fpkg..z2ftcpip..z2fnetwork..z2fipv4.igmpState.init..func1",
"gvisor.x2edev..z2fgvisor..z2frunsc..z2fsandbox.struct.4Size.0uint32.2Mallocs.0uint64.2Frees.0uint64.5..eq",
"gvisor.x2edev..z2fgvisor..z2fpkg..z2flog.Level.UnmarshalJSON",
"gvisor.x2edev..z2fgvisor..z2fpkg..z2flog.CopyStandardLogTo..func1",
"reflect.rtype.Field"]

llvm_fname_filename = "all_name.txt"
demangled_llvm_fname_filename = "demangled_llvm_fname.txt"
excep_words = ["..init", "..thunk", "..stub", "..import", "..eq", "..hash"]

with open(llvm_fname_filename, "r") as f_in:
    with open(demangled_llvm_fname_filename, "w") as f_out:
        # for i in t:
        for i in f_in:
            i = i.strip()
            if "gvisor" not in i: continue
            mark = False
            for word in excep_words:
                if word in i: 
                    mark = True
                    break
            if mark == True: continue
            x = re.sub(r"\.x2e", ".", i)
            x = re.sub(r"\.\.z2f", "/", x)
            # print(i)
            # print(x)
            end_part = re.split(r"gvisor\.dev", x)[-1]
            # print("gvisor.dev" + end_part)
            # print("")
            f_out.write("{}@{}\n".format(i, "gvisor.dev" + end_part))