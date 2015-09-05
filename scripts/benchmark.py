#!/usr/bin/env python

import os
import json
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

def parse_stdev(s):
    sdelim = "Standard deviation: "
    assert s.startswith(sdelim)
    return float(s[len(sdelim):])

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
    "lyra2-clang": "make CC=clang NO_AVX2=1",
    "lyra2-gcc": "make CC=gcc NO_AVX2=1",
    "lyra2-avx2-clang": "make CC=clang",
    "lyra2-avx2-gcc": "make CC=gcc",
    "ref-clang": "make bench-ref CC=clang",
    "ref-gcc": "make bench-ref CC=gcc"
}

if len(sys.argv) == 1:
    usage()

BINARIES_DIR = "bench-binaries"
if os.path.isdir(BINARIES_DIR):
    shutil.rmtree(BINARIES_DIR)
if not os.path.isdir(BINARIES_DIR):
    os.mkdir(BINARIES_DIR)

RESULTS_DIR = "bench-results"
if os.path.isdir(RESULTS_DIR):
    shutil.rmtree(RESULTS_DIR)
if not os.path.isdir(RESULTS_DIR):
    os.mkdir(RESULTS_DIR)

if sys.platform.startswith('linux'):
    RESULTS_FILE = "Linux.json"
elif sys.platform.startswith('darwin'):
    RESULTS_FILE = "OSX.json"
else:
    assert False, "Unknown platform?"

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

results = {}
for results_lines in it:
    for l in results_lines:
        assert l == results_lines[0], "different paramenters"

    _, parameters = results_lines[0].split(' ', 1)
    print parameters + ": "
    results[parameters] = {}

    outputs_lines = next(it)
    for i, l in enumerate(outputs_lines):
        if l != outputs_lines[-1]:
            bname = build_names[i]
            print >>sys.stderr, \
                "warning: %s has a different output from the reference" % bname

    timings = map(parse_time, next(it))
    sdevs = map(parse_stdev, next(it))
    for i, (name, timing, sdev) in enumerate(zip(build_names, timings, sdevs)):
        print "    %s: %d us (stdev: %.2f)" % (name, timing, sdev),
        results[parameters][name] = (timing, sdev)
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

with open(os.path.join(RESULTS_DIR, RESULTS_FILE), "w") as f:
    json.dump(results, f, indent = 2)
