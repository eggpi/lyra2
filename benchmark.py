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

BINARIES_DIR = "bench-binaries"
if os.path.isdir(BINARIES_DIR):
    shutil.rmtree(BINARIES_DIR)
if not os.path.isdir(BINARIES_DIR):
    os.mkdir(BINARIES_DIR)

def usage():
    print >>sys.stderr, "Usage: " + sys.argv[0] + " <build A> ... <ref build>"
    print >>sys.stderr, "Available builds are:",
    print >>sys.stderr, ", ".join(AVAILABLE_BUILDS.keys())
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

        for name, cmd in builds:
            subprocess.check_call(["make", "clean"], **redirects)
            subprocess.check_call(cmd.split(" "), **redirects)
            binaries.append(os.path.join(BINARIES_DIR, name))
            shutil.move("lyra2", binaries[-1])
    return binaries

AVAILABLE_BUILDS = {
    "lyra2-clang": "make CC=clang",
    "lyra2-gcc": "make CC=gcc",
    "lyra2-avx2-clang": "make CC=clang AVX=1",
    "ref-clang": "make bench-ref CC=clang",
    "ref-gcc": "make bench-ref CC=gcc"
}

if len(sys.argv) == 1:
    usage()

build_names = sys.argv[1:]
build_commands = map(AVAILABLE_BUILDS.get, sys.argv[1:])
if None in build_commands:
    print >>sys.stderr, \
        "Invalid build '%s'!" % build_names[build_commands.index(None)]
    usage()

binaries = build(zip(build_names, build_commands))
outputs = []
for binary in binaries:
    outputs.append(
        subprocess.check_output([binary])[:-1].split("\n"))
shutil.rmtree(BINARIES_DIR)

for o in outputs:
    assert len(o) == len(outputs[0])

print "Done running builds, speedup will be relative to '%s'" % build_names[-1]

it = itertools.izip(*outputs)
for header_line in it:
    for l in header_line:
        assert l == header_line[0], "different inputs"
    if not header_line[0]:
        break

for i, results_lines in enumerate(it):
    for l in results_lines:
        assert l == results_lines[0], "different paramenters"

    _, parameters = results_lines[0].split(' ', 1)
    print parameters + ": "

    outputs_lines = next(it)
    for l in outputs_lines:
        if l != outputs_lines[0]:
            bname = build_names[i]
            print >>sys.stderr, \
                "warning: %s has a different output from the reference" % bname

    timings = map(parse_time, next(it))
    for i, (name, timing) in enumerate(zip(build_names, timings)):
        print "    %s: %d us" % (name, timing),
        if i == len(timings) - 1:
            print
            continue

        speedup = 100 * (timings[-1] - timing) / float(timings[-1])
        if speedup > 0:
            print "{term.green}{:.2f}% faster{term.normal}".format(speedup, term = term)
        else:
            print "{term.red}{:.2f}% slower{term.normal}".format(-speedup, term = term)

    blank_lines = next(it)
    for l in blank_lines:
        assert not l
