#!/bin/bash

FGDIR=""
OSCDIR=""

SNo="MPP100359094"

NUM_EVENT=3
iVol=0.1 #V
iHalfV=-0.05 #V
NUM_CH=4
FST_CH=24
LAST_CH=$((FST_CH + NUM_CH - 1))

Frequency=1000 #Hz
Amplitude=0.5 #V
Half=-0.25 #V

#Function generator setup
#$FGDIR/./FG_control 1 -s waveType pulse
#$FGDIR/./FG_control 1 -s frequency $Frequency
#$FGDIR/./FG_control 1 -s outputLoad 50
#$FGDIR/./FG_control 1 -s riseTime 1e-9
#$FGDIR/./FG_control 1 -s fallTime 1e-9
#$FGDIR/./FG_control 1 -s pulseWidth 30e-9
#$FGDIR/./FG_control 1 -s amplitude 3.3
#$FGDIR/./FG_control 1 -s offset 1.65

./FG_control 2 -s waveType pulse
./FG_control 2 -s frequency $Frequency
./FG_control 2 -s outputLoad 50
./FG_control 2 -s riseTime 1e-9
./FG_control 2 -s fallTime 1e-9
./FG_control 2 -s pulseWidth 30e-9
./FG_control 2 -s amplitude 3.3
./FG_control 2 -s offset 1.65
./FG_control 2 -s offset 1.65
./FG_control 2 -s delay 0.000499980

./FG_control 1 -s waveType square
./FG_control 1 -s frequency $Frequency
./FG_control 1 -s outputLoad 50
./FG_control 1 -s amplitude $Amplitude
./FG_control 1 -s offset $Half
./FG_control 1 -s duty 50
./FG_control 1 -s phase 0



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

#Change input amplitude and save waveform
