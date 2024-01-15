#!/bin/bash
#https://ottverse.com/guide-to-essential-video-coding-evc-mpeg5-part1/
#there are 5 intra prediction modes: DC (average value of the neighbours), horizontal, vertical, diagonal left, and diagonal right.

if [ $# -lt 1 ]; then
  echo "ERROR Usage: $0 <encoder log or directory with logs> [Sequence]"
  exit 1
fi


ENCODER_LOG=$1
# List of modes to analize (currently, we improve only mode 0)
MODE_LIST="0"
# CU sizes to filter
CUW_LIST="32 16 8 4"

# The log may be a single file or a stitch of multiple files
if [ -d $ENCODER_LOG ]; then
  # We may want to filter out a single sequence
  if [ $# -eq 1 ]; then
    ENCODER_LOGS=$(find $ENCODER_LOG -iname 'encoder.log' | grep 'prop')
  else
    ENCODER_LOGS=$(find $ENCODER_LOG -iname 'encoder.log' | grep 'prop' | grep $2)
  fi
  cat $ENCODER_LOGS > encoder_all.log
  ENCODER_LOGS=encoder_all.log
fi

for CUW in $CUW_LIST; do
  echo "                **** cuw $CUW ****"
  echo "Mode Improved   Totals  Ratio   CostEVC CostNN  CostDeltaPerc"
  for MODE in $MODE_LIST; do
    echo -n "$MODE    "
    cat $ENCODER_LOGS | egrep "cuw $CUW" > analyze.data
    CUS=$(wc -l analyze.data | awk '{print $1}')
    if [ $CUS -gt 0 ]; then
        cat $ENCODER_LOGS | egrep "cuw $CUW "| awk '{COST_EVC = COST_EVC+$12; COST_NN = COST_NN + $14; if($14 < $12){T=T+1}}END{if(NR==0){NR=1}; print T"\t"NR"\t"T/NR*100"\t"COST_EVC/NR"\t"COST_NN/NR"\t"(100*(COST_NN-COST_EVC)/COST_EVC)}'
    else
        echo "NA"
    fi
  done
done

# Cleanup time
rm 2>/dev/null encoder_all.log analyze.data

