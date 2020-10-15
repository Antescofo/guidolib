#! /usr/bin/python3

# -*- coding: utf-8 -*-
import sys
import os
import random
from datetime import datetime
import subprocess


piece_title = sys.argv[1]
author = sys.argv[2]
accom_instru_pk = sys.argv[3]

base_folder = './thumbnails'
base_instrument = 'violin'
if accom_instru_pk in ['35']:
    base_instrument = 'violin'
elif accom_instru_pk in ['38']:
    base_instrument = 'cello'
elif accom_instru_pk in ['37']:
    base_instrument = 'piano'
elif accom_instru_pk in ['34', '39', '56']:
    base_instrument = 'flute'
base_folder += '/' + base_instrument
random_file = base_folder + '/' + random.choices(os.listdir(base_folder))[0]
to_display = ''
piece_title.strip()
line_len = 0
for f in piece_title.split(' '):
    line_len += len(f) + 1
    to_display += f + ' '
    if line_len > 12:
        line_len = 0
        to_display += '\n'

if line_len != 0:
    to_display += '\n'
to_display += '\n' + author
subprocess.run(['rm', '-f', 'test_output.png'])
subprocess.run(['ffmpeg', '-i', random_file, '-vf', f'drawtext="x=0: fontcolor=0xffffffff: fontfile=/app/tools/HelveticaNeueBd.ttf: text={to_display}: fontsize=65: x=30: y=30', '-codec:a', 'copy', 'test_output.png'])
