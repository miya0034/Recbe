connect your computer with function generator by LAN cable
Set Fuction generator's ip to 192.168.10.2 (or 192.168.10.xx) ***notice this should be consistent with ip in header file***
Set Fuction generator's mask to 255.255.255.0
Set your computer's ip to 192.168.10.10 (or 192.168.10.xx)
Set your computer's mask to 255.255.255.0

compile FG_control.cxx
run FG_control and use -h or --help option to see how to use it



-----------------------------------------------------------------------
User Function Data Format

SDG6032X receives 2-byte(16-bit) signed hex number as data points, distribute pointes on waveform with equal distance.
e.g. If you sent 4 points as a user function you will see the 3rd point on the middle of waveform (aka at phase = 180).

See testwave.txt for example user function data file. 32 points are written in the text file, those points will appear on the waveform with equal distance inside one period. The result is a rough double stair down function. Add as more points as you can up to 32768 points to make the function precise.

-----------------------------------------------------------------------
One concern to c++ code: Why there is no need to convert data points to network endianness aka big-endian? Maybe it's a problem of software inside function generator?
