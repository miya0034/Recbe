#!/bin/zsh

alias r++="g++ $(root-config --cxx --cflags --libs) -lMinuit2"

cd DAQ || exit
make || exit

cd ../binary2 || exit
r++ binary2root.cxx -o ../binary2root



