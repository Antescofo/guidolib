#! /bin/bash

rm -f output* $4 final_audio.mp3 audio.raw audio.wav

videogeneration $1 $2 && \
    ffmpeg -framerate 24 -i output%01d.png output.mp4 && \
    ffmpeg -i output.mp4 -i watermark_10a.png -filter_complex "overlay=0:0" output_watermarked.mp4 && \
    rm -f output*.png && \
    sox -t f32 -r 48000 -c 1 audio.raw audio.wav && \
    ffmpeg -i audio.wav -i $3 -filter_complex amix=inputs=2:duration=longest final_audio.mp3 && \
    ffmpeg -i output_watermarked.mp4 -i final_audio.mp3 -c copy $4
