#!/usr/bin/env python

from matplotlib import rc, pyplot as plt
import numpy as np

import itertools
import os

# use LaTeX fonts
rc('font', **{'family': 'serif', 'serif': ['Computer Modern']})
rc('text', usetex = True)

all_results = {
    'linux': [
        ('Linux, R = 16, T = 16, C = 256', (
            ('ref-gcc', 1787.0),
            ('ref-clang', 1947.0),
            ('clang', 1767.0),
            ('gcc', 1907.0),
            ('avx2-clang', 1207.0),
            ('avx2-gcc', 1242.0),
        )),
        ('Linux, R = 32, T = 32, C = 256', (
            ('ref-gcc', 7032.0),
            ('ref-clang', 7641.0),
            ('clang', 6917.0),
            ('gcc', 7490.0),
            ('avx2-clang', 4768.0),
            ('avx2-gcc', 4888.0),
        )),
        ('Linux, R = 64, T = 64, C = 256', (
            ('ref-gcc', 27889.0),
            ('ref-clang', 30267.0),
            ('clang', 27393.0),
            ('gcc', 29675.0),
            ('avx2-clang', 19013.0),
            ('avx2-gcc', 19529.0),
        ))
    ],
    'osx': [
        ('OSX, R = 16, T = 16, C = 256', (
            ('ref-clang', 2329.0),
            ('clang', 1896.0)
        )),
        ('OSX, R = 32, T = 32, C = 256', (
            ('ref-clang', 9148.0),
            ('clang', 7523.0),
        )),
        ('OSX, R = 64, T = 64, C = 256', (
            ('ref-clang', 36287.0),
            ('clang', 29994.0)
        ))
    ]
}

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
        for build, time in timings:
            b = a.bar(left = left,
                      height = time / timings[0][1],
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

    plt.savefig(os.path.join('img', figname), dpi = 100)
