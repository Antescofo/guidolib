#! /bin/bash

rm -f output* $4

videogeneration $1 $2 && \
    ffmpeg -framerate 24 -i output%01d.png output.mp4 && \
    rm -f output*.png && \
    ffmpeg -i output.mp4 -i $3 -c copy $4
