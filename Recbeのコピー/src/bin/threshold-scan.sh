#!/bin/bash

#====== 引数チェックと展開 =================================================
if [ $# -ne 5 ]; then
  echo "Usage: ./acquisition_only_scan.sh <SNo> <FST_CH> <IP> <FG_START> <DAC_START>"
  exit 1
fi

SNo=$1
FST_CH=$2
IP=$3
FG_START=$4
DAC_START=$5
NUM_CH=8
LAST_CH=$((FST_CH + NUM_CH - 1))
NUM_EVENT=300

RANGE_MIN=3350
RANGE_MAX=3850
DAC_STEP=1
#=============================================================================

#====== 関数定義 =============================================================

set_tdc_threshold() {
  local v_th_mv=$1
  ./configRECBE3.pl ${IP} setTDCThreshold ${v_th_mv}
  sleep 0.5
}

setup_fg() {
  local fg_mv=$1
  local amplitude_V=$(echo "scale=3; ${fg_mv} / 1000" | bc)
  local offset_V=$(echo "scale=3; ${amplitude_V} / -2" | bc)

  ./FG_control 1 -s amplitude ${amplitude_V}
  ./FG_control 1 -s offset ${offset_V}
}

run_acquisition() {
  local current_fg_mv=$1
  local current_vth_mv=$2

  local dat_dir="../dat/${SNo}/thresholdscan"
  local root_dir="../ROOT/${SNo}/thresholdscan"

  mkdir -p "${dat_dir}"
  mkdir -p "${root_dir}"
  mkdir -p "${root_dir}/fig"
  mkdir -p "${root_dir}/fitting"

  local filename_prefix="${FST_CH}_${LAST_CH}_FG${current_fg_mv}mV_DAC${current_vth_mv}mV"
  local dat_file="${dat_dir}/${filename_prefix}.dat"
  local root_file="${root_dir}/${filename_prefix}.root"

  ./48chDebugMac ${IP} 24 ${NUM_EVENT} "${dat_file}"
  ./binary2root2 "${dat_file}" "${root_file}"
}

#====== メイン処理 ===========================================================

./FG_control 1 -s outputSwitch on
./FG_control 2 -s outputSwitch on

FG_FIRST_DONE=0

for fg_mv in $(seq ${FG_START} 10 50); do
  setup_fg ${fg_mv}

  if [ ${FG_FIRST_DONE} -eq 0 ]; then
    current_dac_start=${DAC_START}
    FG_FIRST_DONE=1
  else
    current_dac_start=${RANGE_MIN}
  fi

  for vth_mV in $(seq ${current_dac_start} ${DAC_STEP} ${RANGE_MAX}); do
    set_tdc_threshold ${vth_mV}
    run_acquisition ${fg_mv} ${vth_mV}
  done

  ./thresholdscan ${SNo} ${fg_mv} ${FST_CH}
done

./FG_control 1 -s outputSwitch off
./FG_control 2 -s outputSwitch off
