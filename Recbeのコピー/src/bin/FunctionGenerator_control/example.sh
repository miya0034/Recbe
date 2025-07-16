#!/bin/zsh

FGDIR="`pwd`"

Frequency=50000 #Hz
Vol=3.3 #V

#trigger signal setup
$FGDIR/FG_control 2 -s waveType pulse
sleep 0.5
$FGDIR/FG_control 2 -s frequency $Frequency
sleep 0.5
$FGDIR/FG_control 2 -s outputLoad 50
sleep 0.5
$FGDIR/FG_control 2 -s amplitude $Vol
sleep 0.5
$FGDIR/FG_control 2 -s riseTime 1e-9
sleep 0.5
$FGDIR/FG_control 2 -s fallTime 1e-9
sleep 0.5
$FGDIR/FG_control 2 -s pulseWidth 1e-5
sleep 0.5
$FGDIR/FG_control 2 -s outputSwitch on
sleep 0.5

$FGDIR/FG_control 2 -s outputSwitch off
