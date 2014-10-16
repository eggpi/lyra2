#!/usr/bin/env python

import os
import subprocess
import itertools

try:
    import blessings
    term = blessings.Terminal()
except ImportError:
    class FakeTerminal(object):
        def __getattr__(self, a):
            return ''
    term = FakeTerminal()

def parse_time(s):
    sdelim = "Median time: "
    edelim = " us"
    assert s.startswith(sdelim)
    assert s.endswith(edelim)
    return int(s[len(sdelim):-len(edelim)])

with open(os.devnull, "w") as devnull:
    redirects = {"stdout": devnull, "stderr": devnull}
    subprocess.check_call(["make", "clean"], **redirects)
    subprocess.check_call(["make"], **redirects)
    subprocess.check_call(["make", "bench-ref"], **redirects)

output = subprocess.check_output(["./lyra2"])[:-1] # remove trailing \n
ref_output = subprocess.check_output(["./ref/Lyra2-v2.5_PHC/bin/Lyra2"])[:-1]

output_lines = output.split("\n")
ref_output_lines = ref_output.split("\n")
assert len(output_lines) == len(ref_output_lines)

it = itertools.izip(output_lines, ref_output_lines)
for l, rl in it:
    assert l == rl, "different inputs"
    if not l:
        break

for l, rl in it:
    assert l == rl, "different paramenters"
    _, parameters = l.split(' ', 1)
    print parameters + ": ",

    l, rl = next(it)
    assert l == rl, "different outputs"

    t, rt = map(parse_time, next(it))
    speedup = 100 * (rt - t) / float(rt)
    if speedup > 0:
        print "{term.green}{:.2f}% faster{term.normal}".format(speedup, term = term)
    else:
        print "{term.red}{:.2f}% slower{term.normal}".format(-speedup, term = term)

    l, rl = next(it)
    assert not l and not rl
