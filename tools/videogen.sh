#! /bin/bash

rm -f output*

videogeneration $1 $2 $3 && ffmpeg -framerate 24 -i output%01d.png output.mp4
