#!/bin/bash
export LC_ALL="C"; PWD=$(pwd)

if [ $# -lt 1 ]; then
  echo "ERROR Usage: $0 <proposed encode_batch_summary.txt> [<reference encode_batch_summary.txt>]"
  exit 10
fi

THIS_DIR=$(pwd)

# See also https://sourceforge.net/projects/transpose/
if [ ! -f transpose ]; then
  gcc transpose.c -o transpose
fi

# Summary of the encoding results in terms of BITS-YPSNR as produced by encode_batch.sh
# This by default will be used for both proposed and reference scheme
SUMMARY_FILE_PROP="${1}"

# We may specify to take the reference from a different file
if [ $# -eq 2 ]; then
  SUMMARY_FILE_REF="${2}"
else
  SUMMARY_FILE_REF=$SUMMARY_FILE_PROP
fi


SEQUENCE_LIST=$(cat $SUMMARY_FILE_PROP | awk '{print $4}'| sort | uniq)
QP_LIST=$(cat $SUMMARY_FILE_REF | awk '{print $6}'| sort | uniq)

echo "Found sequences list is: "${SEQUENCE_LIST}
echo "Found QPs list is: "${QP_LIST}

for SEQUENCE in $SEQUENCE_LIST; do
  # We compute the BDRATE for each sequence
  echo "Plotting sequence "${SEQUENCE}
#  for QP in $QP_LIST; do
    # Rate and PSNR for the reference
    R1=$(cat $SUMMARY_FILE_REF | grep 'ref' | grep $SEQUENCE | awk '{printf($8" ")}');
    PSNR1=$(cat $SUMMARY_FILE_REF | grep 'ref' | grep $SEQUENCE | awk '{printf($10" ")}');
    # Rate and PSNR for the proposed
    R2=$(cat $SUMMARY_FILE_PROP | grep 'prop' | grep $SEQUENCE | awk '{printf($8" ")}');
    PSNR2=$(cat $SUMMARY_FILE_PROP | grep 'prop' | grep $SEQUENCE | awk '{printf($10" ")}');
    echo "#R1[kbps] ${R1}" > bdrate_t.gdata
    echo "PSNR1[db] ${PSNR1}" >> bdrate_t.gdata
    echo "R2[kbps] ${R2}" >> bdrate_t.gdata
    echo "PSNR2[db] ${PSNR2}" >> bdrate_t.gdata
    ./transpose -t --fsep " " bdrate_t.gdata > bdrate.gdata
    rm bdrate_t.gdata
    export GNUPLOT_TITLE="${SEQUENCE} QPs ${QP_LIST}"
    export GNUPLOT_FILE="$(dirname ${SUMMARY_FILE_PROP})/${SEQUENCE}.png"
    gnuplot < plot.gscript
done


