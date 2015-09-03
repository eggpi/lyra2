#!/usr/bin/env python

from matplotlib import rc, pyplot as plt
import numpy as np

import sys
import itertools
import os
import json

# use LaTeX fonts
rc('font', **{'family': 'serif', 'serif': ['Computer Modern']})
rc('text', usetex = True)

IMAGE_DIR = sys.argv[1]

all_results = {}
for results_file in sys.argv[2:]:
    env, _ = os.path.splitext(os.path.basename(results_file))
    with open(results_file) as f:
        results = json.load(f)

    all_results[env.lower()] = []
    for params in results:
        all_results[env.lower()].append(
            (env + ', ' + params, results[params].items()))

for _, results in all_results.items():
    for _, timings in results:
        assert len(timings) == len(results[0][1]), 'missing timings?'

for figname, results in all_results.items():
    fig, axes = plt.subplots(
        ncols = len(results), nrows = 1, facecolor = 'white', sharey = True)
    for a, (test, timings) in zip(axes, results):
        width = 0.3
        bars = [] # save the last bar graphs we've built for the legend
        ntimings = len(timings)
        a.get_xaxis().set_visible(False)

        colors = [
            '#f1595f',
            '#727272',
            '#79c36a',
            '#599ad3',
            '#9e66ab',
            '#f9a65a',
            '#d77fb3',
            '#cd7058',
        ]

        step = 0.0125
        left = step
        color_picker = itertools.cycle(colors)
        for build, (time, sdev) in timings:
            b = a.bar(left = left,
                      height = float(time) / timings[0][1][0],
                      width = width,
                      color = next(color_picker),
                      label = build)
            bars.append(b)
            left += width + step

        a.set_xlim(0, left) # Finish the x axis at the last bar
        a.set_title(test, fontsize = 18, fontweight = 'bold')

    # Finish all y axes in the same value
    ymin, ymax = plt.ylim()
    plt.yticks(np.arange(ymin, max(ymax, 1.1), 0.1))
    plt.ylim(ymin, ymax + 0.1)
    fig.set_size_inches(16.0, 6.5)

    # Move figure up a little, and add a legend to the bottom
    plt.subplots_adjust(bottom = 0.15)
    fig.legend(
        bars, [b.get_label() for b in bars],
        ncol = ntimings / 2 if ntimings > 2 else 2,
        loc = 'lower center',
        fontsize = 18, frameon = False)

    plt.savefig(os.path.join(IMAGE_DIR, figname), dpi = 100)
