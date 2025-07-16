#!/bin/bash

FGDIR=""
OSCDIR=""
# 引数チェック
if [ $# -ne 3 ]; then
  echo "Usage: $0 <SNo> <FST_CH> <IP>"
  exit 1
fi

SNo=$1
IP=$3
NUM_EVENT=3
iVol=0.1 #V
iHalfV=-0.05 #V
NUM_CH=8
FST_CH=$2
LAST_CH=$((FST_CH + NUM_CH - 1))

Frequency=500 #Hz
Amplitude=0.2 #V
Half=-0.1 #V

#Function generator setup
#$FGDIR/./FG_control 1 -s waveType pulse
#$FGDIR/./FG_control 1 -s frequency $Frequency
#$FGDIR/./FG_control 1 -s outputLoad 50
#$FGDIR/./FG_control 1 -s riseTime 1e-9
#$FGDIR/./FG_control 1 -s fallTime 1e-9
#$FGDIR/./FG_control 1 -s pulseWidth 30e-9
#$FGDIR/./FG_control 1 -s amplitude 3.3
#$FGDIR/./FG_control 1 -s offset 1.65

#./FG_control 2 -s waveType pulse
#./FG_control 2 -s frequency $Frequency
#./FG_control 2 -s outputLoad 50
#./FG_control 2 -s riseTime 1e-9
#./FG_control 2 -s fallTime 1e-9
#./FG_control 2 -s pulseWidth 30e-9
#./FG_control 2 -s amplitude 3.3
#./FG_control 2 -s offset 1.65

#./FG_control 1 -s waveType square
#./FG_control 1 -s frequency $Frequency
#./FG_control 1 -s outputLoad 50
#./FG_control 1 -s amplitude $Amplitude
#./FG_control 1 -s offset $Half
#./FG_control 1 -s duty 50
#./FG_control 1 -s phase 180



./FG_control 1 -s outputSwitch on
./FG_control 2 -s outputSwitch on

#Oscilloscope setup
# osc_control -s channel1 on
# osc_control -s channel2 on
# osc_control -s tirggerType edge
# osc_control -s triggerType edge
# osc_control -s edgeSource 1
# osc_control -s edgeSlope rise
# osc_control -s triggerThreshold 1.65
# osc_control -s xOffset 0
# osc_control -s xScale 1E-8
# osc_control -s yOffset 0
# osc_control -s yScaleCH1 1
# osc_control -s yScaleCH2 $Amplitude

# ../ROOT/SNo フォルダが無ければ作成
if [ ! -d "../ROOT/${SNo}" ]; then
  mkdir -p "../ROOT/${SNo}"
fi

# フォルダの存在確認と作成
if [ ! -d "../ROOT/${SNo}/chargeVSadc" ]; then
    mkdir -p "../ROOT/${SNo}/chargeVSadc"
fi

if [ ! -d "../ROOT/${SNo}/fitting/" ]; then
    mkdir -p "../ROOT/${SNo}/fitting"
fi
if [ ! -d "../ROOT/${SNo}/waveforms" ]; then
    mkdir -p "../ROOT/${SNo}/waveforms"
fi
#Change input amplitude and save waveform
    Vol=0
	./48chDebugMac ${IP} 24 500 ${FST_CH}_${LAST_CH}ch_${Vol}mV_${SNo}.dat 
	./binary2root ${FST_CH}_${LAST_CH}ch_${Vol}mV_${SNo}.dat ${SNo}
	./wf-reconstructor-loop ${FST_CH} ${FST_CH}_${LAST_CH}ch_${Vol}mV_${SNo}.root ${SNo} ${Vol}

for i in `seq 1 10`;
do
	InputVol=$(echo "$i * 0.01" | bc)
	Vol=$(echo "$i * 10" | bc)
	InputOf=$(echo "$i * -0.005" | bc)
    echo "Set amplitude to $InputVol"
    ./FG_control 1 -s amplitude `printf "%.2f" $InputVol`
	sleep 0.5  # 0.5秒インターバル
    ./FG_control 1 -s offset $InputOf
	sleep 0.5  # 0.5秒インターバル
	
    #osc_control -s yScaleCH2 $InputVol
 
	#osc_control -r 1,2 3
	./48chDebugMac ${IP} 24 500 ${FST_CH}_${LAST_CH}ch_${Vol}mV_${SNo}.dat 
	./binary2root ${FST_CH}_${LAST_CH}ch_${Vol}mV_${SNo}.dat ${SNo}
	./wf-reconstructor-loop ${FST_CH} ${FST_CH}_${LAST_CH}ch_${Vol}mV_${SNo}.root ${SNo} ${Vol}
done

for i in `seq 1 20`;
do
	InputVol=$(echo "$i * 0.1" | bc)
	Vol=$(echo "$i * 100" | bc)
	InputOf=$(echo "$i * -0.05" | bc)
    echo "Set amplitude to $InputVol"
    ./FG_control 1 -s amplitude `printf "%.2f" $InputVol`
	sleep 0.5  # 0.5秒インターバル
    ./FG_control 1 -s offset $InputOf
	sleep 0.5  # 0.5秒インターバル
	
    #osc_control -s yScaleCH2 $InputVol
 
	#osc_control -r 1,2 3
	./48chDebugMac ${IP} 24 500 ${FST_CH}_${LAST_CH}ch_${Vol}mV_${SNo}.dat 
	./binary2root ${FST_CH}_${LAST_CH}ch_${Vol}mV_${SNo}.dat ${SNo}
	./wf-reconstructor-loop ${FST_CH} ${FST_CH}_${LAST_CH}ch_${Vol}mV_${SNo}.root ${SNo} ${Vol}
done
./chargeVSadc ${SNo} ${FST_CH}
read -p "if you want to finish, Hit enter:"

./FG_control 1 -s outputSwitch off
./FG_control 2 -s outputSwitch off

