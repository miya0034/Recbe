#!/bin/sh
if [ $# != 1 ]; then
  echo "Usage: $0 <run number>"
  exit
fi

RUN_NUMBER=$1
FILE_NAME=`printf 'run_%06d-pedestal.txt' $RUN_NUMBER`

while read line;
do
  ich=`echo $line | cut -d ' ' -f 1`
  adc=`echo $line | cut -d ' ' -f 2`
  adc_ped=`printf '%0.0f' $adc`
  #adc_ped=`expr $adc_ped - 3`
  adc_ped=`printf ' %04x' $adc_ped`

  board=`expr $ich \/ 48 - 1`
  ch=`expr $ich \% 48`
  ip=`expr \( $board \/ 6 \) \* 12 + $board \% 6 + 18`

  echo "Board: $board,  CH: $ch,  ADC Pedestal: $adc_ped"

  offset=`expr $ch \* 2 + 32`
  
  for l in 0 1
  do
    addr=`expr $offset + $l`
    addr_hex=`printf '0x%02x' $addr`

    pos=`expr $l \* 2 + 1`
    echo "./Setparm_command 192.168.10.$ip 4660 wrb $addr_hex 0x${adc_ped:$pos:2}"
    ./Setparm_command 192.168.10.$ip 4660 rd $addr_hex 1
    #./Setparm_command 192.168.10.$ip 4660 wrb $addr_hex 0x${adc_ped:$pos:2}
  done

  echo 

done < $FILE_NAME
