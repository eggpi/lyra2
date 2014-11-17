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
a_output = subprocess.check_output(["./" + bins[0]])[:-1] # remove trailing \n
b_output = subprocess.check_output(["./" + bins[1]])[:-1]
a_output_lines = a_output.split("\n")
b_output_lines = b_output.split("\n")
assert len(a_output_lines) == len(b_output_lines)

os.remove(bins[0])
os.remove(bins[1])

print "Done running builds, speedup will be relative to '%s'" % nB

it = itertools.izip(a_output_lines, b_output_lines)
for al, bl in it:
    assert al == bl, "different inputs"
    if not al:
        break

for al, bl in it:
    assert al == bl, "different paramenters"
    _, parameters = al.split(' ', 1)
    print parameters + ": "

    al, bl = next(it)
    assert al == bl, "different outputs"

    at, bt = map(parse_time, next(it))
    print "    %s: %d us" % (nA, at),
    speedup = 100 * (bt - at) / float(bt)
    if speedup > 0:
        print "{term.green}{:.2f}% faster{term.normal}".format(speedup, term = term)
    else:
        print "{term.red}{:.2f}% slower{term.normal}".format(-speedup, term = term)
    print "    %s: %d us" % (nB, bt)

    al, bl = next(it)
    assert not al and not bl

