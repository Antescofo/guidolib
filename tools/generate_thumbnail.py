#! /usr/bin/python3

# -*- coding: utf-8 -*-
import sys

from datetime import datetime
import subprocess


piece_title = sys.argv[1]
author = sys.argv[2]

to_display = ''
piece_title.strip()
line_len = 0
for f in piece_title.split(' '):
    line_len += len(f) + 1
    to_display += f + ' '
    if line_len > 18:
        line_len = 0
        to_display += '\n'

if line_len != 0:
    to_display += '\n'
to_display += '\n' + author
subprocess.run(['rm', '-f', 'test_output.png'])
subprocess.run(['ffmpeg', '-i', 'thumbnail_base_flute.png', '-vf', f'drawtext="fontfile=HelveticaNeueBd.ttf: text=\'{to_display}\': fontcolor=0xffffffff: fix_bounds=true: fontsize=45: x=(30): y=40', '-codec:a', 'copy', 'test_output.png'])
