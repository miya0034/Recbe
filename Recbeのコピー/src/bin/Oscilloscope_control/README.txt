Control software for Tektronix 3 series oscilloscope

How to use:
connect your computer with oscilloscope by LAN cable
Set IP of oscilloscope to 192.168.10.55(or 192.168.10.XX) ***notice this should consistent with the ip address in header file***
Set mask of oscilloscope to 255.255.255.0
Set IP of your computer to 192.168.10.10(or 192.168.10.XX)
Set mask of your computer to 255.255.255.0

compile osc_control.cxx
run osc_control and use -h or --help option to see how to use it

You can change verbose level in header file to control output level


-------------------------------------------------------------------------
