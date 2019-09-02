#! /bin/bash

rm -f output* $4 final_audio.mp3 audio.raw audio.wav merged.mp4 almost_done.mp4

# audioselector $3
# exit 1
videogeneration $1 $2 && \
    ffmpeg -framerate 24 -i output%01d.png output.mp4 && \
    ffmpeg -i output.mp4 -i watermark_10a.png -filter_complex "overlay=0:0" output_watermarked.mp4 && \
    rm -f output*.png && \
    sox -t f32 -r 48000 -c 1 audio.raw audio.wav && \
    ffmpeg -i audio.wav -i $3 -filter_complex amix=inputs=2:duration=longest final_audio.mp3 && \
    ffmpeg -i output_watermarked.mp4 -i final_audio.mp3 -c copy almost_done.mp4 && \
    ffmpeg -i intro.mp4 -i almost_done.mp4 -i outro.mp4 -filter_complex "[0:v] [0:a] [1:v] [1:a] [2:v] [2:a] concat=n=3:v=1:a=1 [v] [a]" -map "[v]" -map "[a]" -strict -2 merged.mp4 && \
    cp merged.mp4 $4
