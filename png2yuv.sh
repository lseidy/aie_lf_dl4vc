#!/bin/sh -x

SOURCE_DIR="/home/machado/Lytro2.0_Inria_sirocco/Lytro2.0_dataset_INRIA_SIROCCO/Lenslet_RGB/2.0/"
TARGET_DIR="./sequences/Lytro2.0_inria_8x8_RGB"

cd $SOURCE_DIR
F_L=$(ls *.png)
cd -

for F in $F_L; do
        F_N=$(echo $F  | sed s/'&'/''/g | sed s/'__'/'_'/g | sed s/'_'/'-'/g | sed s/'.png'/'_4960x3448.yuv'/g)
        IS_10BITS=0
        #$(echo $SOURCE_DIR/$F | grep '10bit' | wc -l)
        if [ $IS_10BITS -eq 1 ]; then
                PIX_FMT='yuv420p10le'
        else
                PIX_FMT='yuv420p'
        fi
        ffmpeg -i $SOURCE_DIR/$F  -pix_fmt $PIX_FMT $TARGET_DIR/$F_N
done
