#! /bin/bash

# URI THUMB: https://s3-eu-west-1.amazonaws.com/public.antescofo.com/final_thumbnail.png
rm -f output* $6 *.raw *.wav merged.mp4 almost_done.mp4 output_dwatermarked.mp4 final_audio.mp3 final_thumbnail.png thumbnail_right.png
echo -e "\n\e[32mGenerating Images with guidolib\e[39m\n" && videogeneration $1 $2 && \
    echo -e "\n\e[32mGenerating Thumbnail\e[39m\n" && convert output0.png -crop 640x720+640+0 thumbnail_right.png && \
    echo -e "\n\e[32mMerging thumbnail\e[39m\n" && ffmpeg -i test_output.png -i thumbnail_right.png -filter_complex "overlay=640:0" final_thumbnail.png && aws s3 cp final_thumbnail.png s3://public.antescofo.com/final_thumbnail.png && \
    echo -e "\n\e[32mMerging images into video\e[39m\n" && ffmpeg -framerate 24 -i output%01d.png output.mp4 && \
    echo -e "\n\e[32mAdding watermark on video\e[39m\n" && ffmpeg -i output.mp4 -i watermark_16a.png -filter_complex "overlay=0:0" output_watermarked.mp4 && \
    echo -e "\n\e[32mAdding qrcode watermark on video\e[39m\n" && ffmpeg -i output_watermarked.mp4 -i qrcode.png -filter_complex "overlay=W-200:H-200" output_dwatermarked.mp4 && \
    echo -e "\n\e[32mDeleting generated images\e[39m\n" && rm -f output*.png && \
    echo -e "\n\e[32mConverting raw audio midi solo to wav\e[39m\n" && sox -t f32 -r 48000 -c 1 audio.raw audio.wav && \
    echo -e "\n\e[32mConverting accomp mp3 to wav\e[39m\n" && ffmpeg -i $3 out.wav && \
    (([ "$4" == "0" ] && cp out.wav out_pitched.wav) || ([ "$4" -ne "0" ] && echo -e "\n\e[32mPitch shift accomp\e[39m\n" && sox out.wav out_pitched.wav pitch $4 20 19)) && \
    # ffmpeg -i out_pitched.wav final_audio.mp3 && \
    (([ "$5" != "" ] && echo -e "\n\e[32mMerging solo recording mp3 and accompaniment mp3\e[39m\n" && ffmpeg -i $5 -i out_pitched.wav -filter_complex amix=inputs=2:duration=longest final_audio.mp3) || ([ "$5" == "" ] && echo -e "\n\e[32mMerging solo synth wav and accompaniment mp3\e[39m\n" && ffmpeg -i out_pitched.wav final_audio.mp3)) && \
    echo -e "\n\e[32mAdding audio to generated video\e[39m\n" && ffmpeg -i output_dwatermarked.mp4 -i final_audio.mp3 -c copy almost_done.mp4 && \
    echo -e "\n\e[32mMerging intro, generated video and outro\e[39m\n" && ffmpeg -i intro.mp4 -i almost_done.mp4 -i outro.mp4 -filter_complex "[0:v] [0:a] [1:v] [1:a] [2:v] [2:a] concat=n=3:v=1:a=1 [v] [a]" -map "[v]" -map "[a]" -strict -2 merged.mp4 && \
    echo -e "\n\e[32mAdding text watermark\e[39m\n" && ffmpeg -i merged.mp4 -vf drawtext="fontfile=HelveticaNeueBd.ttf: text='Metronaut App': fontcolor=0x00000060: fontsize=20: x=(w-text_w - 10): y=(h-text_h - 10)" -codec:a copy $6
