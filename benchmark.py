#!/usr/bin/env python

import os
import subprocess
import itertools
import shutil
import sys

try:
    import blessings
    term = blessings.Terminal()
except ImportError:
    class FakeTerminal(object):
        def __getattr__(self, a):
            return ''
    term = FakeTerminal()

def usage():
    print >>sys.stderr, "Usage: " + sys.argv[0] + " <build A> <build B>"
    print >>sys.stderr, "Available builds are:",
    print >>sys.stderr, ", ".join(name for name, _ in AVAILABLE_BUILDS.items())
    sys.exit(1)

def parse_time(s):
    sdelim = "Median time: "
    edelim = " us"
    assert s.startswith(sdelim)
    assert s.endswith(edelim)
    return int(s[len(sdelim):-len(edelim)])

def build(builds):
    binaries = []
    with open(os.devnull, "w") as devnull:
        redirects = {"stdout": devnull, "stderr": devnull}
        subprocess.check_call(["make", "clean"], **redirects)

        for name, cmd in builds:
            subprocess.check_call(cmd.split(" "), **redirects)
            binaries.append("lyra2-" + name)
            shutil.move("lyra2", binaries[-1])
    return binaries

AVAILABLE_BUILDS = {
    "lyra2-clang": "make CC=clang",
    "lyra2-gcc": "make CC=gcc",
    "ref-clang": "make bench-ref CC=clang",
    "ref-gcc": "make bench-ref CC=gcc"
}

if len(sys.argv) != 3:
    usage()

nA, nB = sys.argv[1:]
bA, bB = map(AVAILABLE_BUILDS.get, sys.argv[1:])
if None in (bA, bB):
    print >>sys.stderr, "Invalid build '%s'!" % (nA if bB is not None else nB)
    usage()

bins = build([(nA, bA), (nB, bB)])
output = subprocess.check_output(["./" + bins[0]])[:-1] # remove trailing \n
ref_output = subprocess.check_output(["./" + bins[1]])[:-1]
os.remove(bins[0])
os.remove(bins[1])

print "Done running builds, speedup will be relative to '%s'" % nB

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

