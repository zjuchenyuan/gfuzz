#!/usr/bin/env python

# input: EDGE caller callee	file_path call_number

import re
import sys

gocallvis_output_filename = "gocallvis_fun.txt"
gocallvis_input_filename = "edge.txt"
anonymous_relation_filename = "anony_relation.txt"

with open(gocallvis_output_filename, "w") as f:
    wait_queue = []
    f2dollarf = {}
    handled = set()
    # {
    #   "gvisor.dev/gvisor/pkg/sentry/syscalls/linux.linkAt" : 
    #   [
    #       { "gvisor.dev/gvisor/pkg/sentry/syscalls/linux.linkAt$2$1" : '21' },
    #       {...}
    #   ]
    # }    
    with open(gocallvis_input_filename, "r") as f_in:
        for line in f_in:
            data_units = line.strip().split("\t")
            # print(line)
            # print(data_units)
            
            caller = data_units[1].replace("*", "").replace("(", "").replace(")", "").split("#")[0]
            callee = data_units[2].replace("*", "").replace("(", "").replace(")", "").split("#")[0]
            filename = data_units[3].split("/")[-1]
            call_line = data_units[4]
    
            if "gvisor" not in caller or "gvisor" not in callee: continue

            is_money = False
            for t in [caller, callee]:
                if "$" in t:
                    is_money = True
                    if t in handled:
                        continue
                    line_dollar_parts = t.split("$")
                    func = line_dollar_parts[0]
                    remainder = line_dollar_parts[1:]
                    f2dollarf.setdefault(func, []).append({t: "".join(remainder)})
                    handled.add(t)
            if is_money:
                wait_queue.append(data_units)
                continue

            f.write("{}@{}@{}:{}\n".format(caller.strip("\""), callee.strip("\""), filename, call_line))

    dollarf2llvmf = {}
    with open(anonymous_relation_filename, "w") as f_ano:
        for ff, df_parts in f2dollarf.items():
            df_parts.sort(key=lambda i:i[list(i.keys())[0]])
            for j, df_part in enumerate(df_parts):
                for df, part in df_part.items():
                    # print(df, "@", "func", j + 1)
                    dollarf2llvmf[df] = ff + "..func" + str(j+1)
                    f_ano.write("{}@{}\n".format(df, dollarf2llvmf[df]))

    for line in wait_queue:
        data_units = line
        caller = data_units[1].replace("*", "").replace("(", "").replace(")", "").split("#")[0]
        callee = data_units[2].replace("*", "").replace("(", "").replace(")", "").split("#")[0]
        filename = data_units[3].split("/")[-1]
        call_line = data_units[4]
        pair = [caller, callee]
        for i, t in enumerate(pair):
                if "$" in t:
                    if i == 0:
                        caller = dollarf2llvmf[t]
                    else:
                        callee = dollarf2llvmf[t]
        f.write("{}@{}@{}:{}\n".format(caller.strip("\""), callee.strip("\""), filename, call_line))
