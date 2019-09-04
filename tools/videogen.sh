#! /bin/bash

rm -f output* $4 *.raw *.wav merged.mp4 almost_done.mp4 final_audio.mp3

echo -e "\n\e[32mGenerating Images with guidolib\e[39m\n" && videogeneration $1 $2 && \
    echo -e "\n\e[32mMerging images into video\e[39m\n" && ffmpeg -framerate 24 -i output%01d.png output.mp4 && \
    echo -e "\n\e[32mAdding watermark on video\e[39m\n" && ffmpeg -i output.mp4 -i watermark_16a.png -filter_complex "overlay=0:0" output_watermarked.mp4 && \
    echo -e "\n\e[32mDeleting generated images\e[39m\n" && rm -f output*.png && \
    echo -e "\n\e[32mConverting raw audio midi solo to wav\e[39m\n" && sox -t f32 -r 48000 -c 1 audio.raw audio.wav && \
    echo -e "\n\e[32mMerging solo wav and accompaniment mp3\e[39m\n" && ffmpeg -i audio.wav -i $3 -filter_complex amix=inputs=2:duration=longest final_audio.mp3 && \
    echo -e "\n\e[32mAdding audio to generated video\e[39m\n" && ffmpeg -i output_watermarked.mp4 -i final_audio.mp3 -c copy almost_done.mp4 && \
    echo -e "\n\e[32mMerging intro, generated video and outro\e[39m\n" && ffmpeg -i intro.mp4 -i almost_done.mp4 -i outro.mp4 -filter_complex "[0:v] [0:a] [1:v] [1:a] [2:v] [2:a] concat=n=3:v=1:a=1 [v] [a]" -map "[v]" -map "[a]" -strict -2 merged.mp4 && \
    echo -e "\n\e[32mAdding text watermark\e[39m\n" && ffmpeg -i merged.mp4 -vf drawtext="fontfile=HelveticaNeueBd.ttf: text='Metronaut App': fontcolor=0x00000060: fontsize=20: x=(w-text_w - 10): y=(h-text_h - 10)" -codec:a copy $4
