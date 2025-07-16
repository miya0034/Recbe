#! /usr/bin/perl
use feature ':5.10';

$ConfigCommand = "/Users/miyaiharuki/Recbe/src/bin/BoardSettings/Setparm_command";
$UDPPort = 4660;
$debug = 0;
$numOfChannels = 48;

@commandList = (
    "All setting",
    "Daq mode", 
    "Windw size",
    "Delay",
    "Threshold for ADC integration",
    "Manchester Enalbe with FCT",
    "TOT function",
    "Threshold for TDC discrimination",
    "Internal busy function",
    "Internal busy width",
    "ADC pedestal",
    "LEMO OUT Channel",
    "LEMO OUT Coincidence Window"
);

&checkArgv;
exit();

#====================================================================================
sub checkArgv {
    if (@ARGV <1) {
    &showUsage;
    exit();
    } 
    $IPAddress = $ARGV[0]; 
    
    if (@ARGV==1) {
    &welcomeMessage;
    &showAllSetting;
    &showMenu;
    }
    if (@ARGV>1) {
    given ($ARGV[1]) {
        when('showAll')           { &showAllSetting; }
        when('showDaqMode')       { &showDaqMode; }
        when('showWindowSize')    { &showWindowSize; }
        when('showDelay')         { &showDelay; }
        when('showADCThreshold')  { &showADCThreshold; }
        when('showManchester')    { &showManchester; }
        when('showTOT')           { &showTOT; }
        when('showTDCThreshold')  { &showTDCThreshold; }
        when('showBusyFunction')  { &showBusyFunction; }
        when('showBusyWidth')     { &showBusyWidth; }
        when('showAllADCPedestal'){ &showAllADCPedestal; }
        when('showLemoout')       { &showLemoout; }
        when('showADCPedestal'){ 
        if (@ARGV<3) { printf("Error: Needs more arguments\n"); &showUsage; exit(); } 
        else { &showADCPedestal($ARGV[2]); }
        }
        when('setDaqMode'){ 
        if (@ARGV<3) { printf("Error: Needs more arguments\n"); &showUsage; exit(); } 
        else { &setDaqMode($ARGV[2]); } 
        }
        when('setWindowSize'){ 
        if (@ARGV<3) { printf("Error: Needs more arguments\n"); &showUsage; exit(); } 
        else { &setWindowSize($ARGV[2]); } 
        }
        when('setDelay'){ 
        if (@ARGV<3) { printf("Error: Needs more arguments\n"); &showUsage; exit(); } 
        else { &setDelay($ARGV[2]); } 
        }
        when('setManchester'){ 
        if (@ARGV<3) { printf("Error: Needs more arguments\n"); &showUsage; exit(); } 
        else { &setManchester($ARGV[2]); } 
        }
        when('setTOT'){ 
        if (@ARGV<3) { printf("Error: Needs more arguments\n"); &showUsage; exit(); } 
        else { &setTOT($ARGV[2]); } 
        }
        when('setTDCThreshold'){ 
        if (@ARGV<3) { printf("Error: Needs more arguments\n"); &showUsage; exit(); } 
        else { &setTDCThreshold($ARGV[2]); } 
        }
        when('setADCThreshold'){ 
        if (@ARGV<3) { printf("Error: Needs more arguments\n"); &showUsage; exit(); } 
        else { &setADCThreshold($ARGV[2]); } 
        }
        when('setBusyFunction'){ 
        if (@ARGV<3) { printf("Error: Needs more arguments\n"); &showUsage; exit(); } 
        else { &setBusyFunction($ARGV[2]); } 
        }
        when('setBusyWidth'){ 
        if (@ARGV<3) { printf("Error: Needs more arguments\n"); &showUsage; exit(); } 
        else { &setBusyWidth($ARGV[2]); } 
        }
        when('setADCPedestal'){ 
        if (@ARGV<4) { printf("Error: Needs more arguments\n"); &showUsage; exit(); } 
        else { &setADCPedestal($ARGV[2],$ARGV[3]); } 
        }
        when('setLemooutChannel'){ 
        if (@ARGV<4) { printf("Error: Needs more arguments\n"); &showUsage; exit(); } 
        else { &setLemooutChannel($ARGV[2],$ARGV[3]); } 
        }
        when('setLemooutWindow'){ 
        if (@ARGV<3) { printf("Error: Needs more arguments\n"); &showUsage; exit(); } 
        else { &setLemooutWindow($ARGV[2]); } 
        }
        default { &showUsage; exit(); }
    }
    }
}

sub showUsage {
    print "usage: configRECBE.pl IPAddress                                            \n";
    print "usage: configRECBE.pl IPAddress showAll                                    \n";
    print "usage: configRECBE.pl IPAddress (set/show)DaqMode         [raw/suppress]   \n";
    print "usage: configRECBE.pl IPAddress (set/show)WindowSize      [value (clock)]  \n";
    print "usage: configRECBE.pl IPAddress (set/show)Delay           [value (clock)]  \n";
    print "usage: configRECBE.pl IPAddress (set/show)ADCThreshold    [value (mV)]     \n";
    print "usage: configRECBE.pl IPAddress (set/show)Manchester      [value (bit)]    \n";
    print "usage: configRECBE.pl IPAddress (set/show)TOT             [off/on (0/1)]   \n";
    print "usage: configRECBE.pl IPAddress (set/show)TDCThreshold    [value (mV)]     \n";
    print "usage: configRECBE.pl IPAddress (set/show)BusyFunction    [off/on (0/1)]   \n";
    print "usage: configRECBE.pl IPAddress (set/show)BusyWidth       [value (clock))] \n";
    print "usage: configRECBE.pl IPAddress showAllADCPedestal                         \n";
    print "usage: configRECBE.pl IPAddress (set/show)ADCPedestal ch  [value (mV)]     \n";
    print "usage: configRECBE.pl IPAddress setLemooutChannel         [value x2]       \n";
    print "usage: configRECBE.pl IPAddress setLemooutWindow          [value(1/120MHz)]\n";
    print "usage: configRECBE.pl IPAddress showLemoout                                \n";
}

sub welcomeMessage {
    print "=====================================================\n";
    print "=         Configuration script for RECBE            =\n";
    print "=              v.3.0 : 12th Apr. 2016               =\n";
    print "=               by Hisataka Yoshida                 =\n";
    print "=       original version made by Akira SATO         =\n";
    print "=====================================================\n";
}

sub showMenu {
    print "\n";
    printf("=== Configuration Menu ===================\n");
    printf("%6d : show All setting\n",0);
    for ($i=1; $i<@commandList; $i++) {
    printf("%6d :  set %s\n", $i, $commandList[$i]);
    }
    printf("%6d : exit\n", 99);
    print "\n";
    
    print "Input command number : ";
    $menuCommandNumber = <STDIN>;
    
    given ($menuCommandNumber) {
    when(0){
        &showAllSetting;
    }
    when(1){
        print "Input value (1=raw,2=suppress) : ";
        $dataValue1 = <STDIN>;
        &setDaqMode($dataValue1);
    }
    when(2){
        print "Input value (unit is clock of 120/4MHz) : ";
        $dataValue1 = <STDIN>;
        &setWindowSize($dataValue1);
    }
    when(3){
        print "Input value (unit is clock of 120/4MHz) : ";
        $dataValue1 = <STDIN>;
        &setDelay($dataValue1);
    }
    when(4){
        print "Input value (unit is mV) : ";
        $dataValue1 = <STDIN>;
        &setADCThreshold($dataValue1);
    }
    when(5){
        print "Input value (0=disable,[1]=trigger,[2]=busy,[3]=pulse) : ";
        $dataValue1 = <STDIN>;
        &setManchester($dataValue1);
    }
    when(6){
        print "Input value (0=disable,1=enable) : ";
        $dataValue1 = <STDIN>;
        &setTOT($dataValue1);
    }
    when(7){
        print "Input value (unit is mV) : ";
        $dataValue1 = <STDIN>;
        chomp($dataValue1);
        &setTDCThreshold($dataValue1);
    }
    when(8){
        print "Input value (0=disable,1=enable) : ";
        $dataValue1 = <STDIN>;
        &setBusyFunction($dataValue1);
    }
    when(9){
        print "Input value (unit is usec) : ";
        $dataValue1 = <STDIN>;
        &setBusyWidth($dataValue1);
    }
    when(10){
        print "Input ch number : ";
        $ch = <STDIN>;
        print "Input value (unit is mV) : ";
        $dataValue1 = <STDIN>;
        &setADCPedestal($ch,$dataValue1);
    }
    when(11){
        print "Input ASD number (0-6) : ";
        $ch = <STDIN>;
        print "Input number to identify channels ";
        $dataValue1 = <STDIN>;
        &setLemooutChannel($ch,$dataValue1);
    }
    when(12){
        print "Input value (unit is 1/120MHz~8.33nsec) : ";
        $dataValue1 = <STDIN>;
        &setLemooutWindow($dataValue1);
    }
    when(99){
        exit();
    }
    default {
        print "Invalid commnand number\n";
        &showMenu;
    }
    }
    &showMenu;
}

sub execConfigCommand {
    @commandReturn = {""};
    my ($regCommand, $regAddress, $regData) = @_;
    $configCommand = sprintf("%s %s %d %s 0x%02x %s",
                 $ConfigCommand, $IPAddress, $UDPPort,
                 $regCommand, $regAddress, $regData); 
    @commandReturn = split(/ /,`$configCommand`);
    if (debug==1) {
    print "rval = @commandReturn";
    for ($i=0;$i<@commandReturn;$i++) {
        printf("i=%d: %s\n",$i,$commandReturn[$i]);
    }
    }
}

#----------------------------------------------------------------------
# Functions to show current register setting
#----------------------------------------------------------------------
sub showAllSetting {
    print "----------------------------------------------------------------------------\n";
    printf("- Register setting of RECBE IP:%s              \n",$IPAddress);
    print "----------------------------------------------------------------------------\n";
    &showSynDate;
    &showFwVersion;
    &showDaqMode;
    &showWindowSize;
    &showDelay;
    &showADCThreshold;
    &showManchester;
    &showTOT;
    &showTDCThreshold;
    &showBusyFunction;
    &showBusyWidth;
 #   &showAllADCPedestal;
    &showLemoout;
    print "----------------------------------------------------------------------------\n";
    print "\n";
}

sub showSynDate{
    $regAddress = 0x0;
    &execConfigCommand("rd",$regAddress,4);
    $v_SynDate = "$commandReturn[3]$commandReturn[4]$commandReturn[5]$commandReturn[6]" ;
    $YY = "$commandReturn[3]" ;
    $MM = "$commandReturn[4]" ;
    $DD = "$commandReturn[5]" ;
    $NN = "$commandReturn[6]" ;

    printf("%35s (0x%02x-0x%02x) : %s (0x%s)\n", 
       "synthesis date",
       $regAddress, $regAddress+3, "$YY-$MM-$DD $NN", $v_SynDate);     
}

sub showFwVersion {
    $regAddress = 0x4;
    &execConfigCommand("rd",$regAddress,1);
    $v_FwVersion = hex($commandReturn[3]);
    printf("%35s (0x%02x)      :          %02d (0x%02x)\n", 
       "firmware version",
       $regAddress, $v_FwVersion, $v_FwVersion) ;
}

sub showDaqMode {
    $regAddress = 0x5;
    &execConfigCommand("rd",$regAddress,1);
    #print "rval = @commandReturn";
    $v_DaqMode = hex($commandReturn[3]);
    if ($v_DaqMode==1) {
    $str_DaqMode = "raw data";
    } elsif ($v_DaqMode==2) {
    $str_DaqMode = "suppress data";
    } else {
    $str_DaqMode = "invalid";
    } 
    printf("%35s (0x%02x)      : %-10s  (0x%02x)\n", 
       "DAQ mode",
       $regAddress, $str_DaqMode, $v_DaqMode);     
}

sub showWindowSize {
    $regAddress = 0x6;
    &execConfigCommand("rd",$regAddress,1);
    #print "rval = @commandReturn";
    $v_WindowSize = hex($commandReturn[3]);
    printf("%35s (0x%02x)      : %8.1f ns (0x%02x clocks)\n", 
       "window size",
       $regAddress, $v_WindowSize*(1/120*4*1000), $v_WindowSize);     
}

sub showDelay {
    $regAddress = 0x7;
    &execConfigCommand("rd",$regAddress,1);
    #print "rval = @commandReturn";
    $v_Delay = hex($commandReturn[3]);
    printf("%35s (0x%02x)      : %8.1f ns (0x%02x clocks)\n", 
       "delay",
       $regAddress, $v_Delay*(1/120*4*1000), $v_Delay);     
}


sub showBusyFunction {
    $regAddress = 0x1E;
    &execConfigCommand("rd",$regAddress,1);
    #print "rval = @commandReturn";
    $v_BusyFunction = hex($commandReturn[3]);
    if ($v_BusyFunction==0) {
    $str_BusyFunction = "disable";
    } elsif ($v_BusyFunction==1) {
    $str_BusyFunction = "enable";
    } else {
    $str_BusyFunction = "invalid";
    } 
    printf("%35s (0x%02x)      : %-10s  (0x%02x)\n", 
       "internal busy function",
       $regAddress, $str_BusyFunction, $v_BusyFunction);     
}

sub showManchester {
    $regAddress = 0x0A;
    &execConfigCommand("rd",$regAddress,1);
    $v_Manchester = hex($commandReturn[3]);
    $str_Manchester = "";
#    printf(($v_Manchester&7));
    if (($v_Manchester&1)==1) {
    $str_Manchester = "trig ";
    }
    if (($v_Manchester&2)==2) {
    $str_Manchester = $str_Manchester."busy ";
    }
    if (($v_Manchester&4)==4) {
    $str_Manchester = $str_Manchester."pulse";
    }
    if(($v_Manchester&7)==0) {
    $str_Manchester = "disable";
    } 
    printf("%35s (0x%02x)      : %-10s  (0x%02x)\n", 
       "Manchester Enable with FCT",
       $regAddress, $str_Manchester, $v_Manchester);     
}

sub showTOT {
    $regAddress = 0x0B;
    &execConfigCommand("rd",$regAddress,1);
    $v_TOT = hex($commandReturn[3]);
    if ($v_TOT==0) {
    $str_TOT = "disable";
    } elsif ($v_TOT==1) {
    $str_TOT = "enable";
    } else {
    $str_TOT = "invalid";
    } 
    printf("%35s (0x%02x)      : %-10s  (0x%02x)\n", 
       "TOT function",
       $regAddress, $str_TOT, $v_TOT);     
}

sub showBusyWidth {
    $regAddress = 0x1F;
    &execConfigCommand("rd",$regAddress,1);
    #print "rval = @commandReturn";
    $v_BusyWidth = hex($commandReturn[3]);
    printf("%35s (0x%02x)      : %8.1f us (0x%02x clocks)\n", 
       "busy width",
       $regAddress, $v_BusyWidth, $v_BusyWidth);     
}


sub showTDCThreshold {
    $regAddressUpper = 0x1c;
    $regAddressLower = 0x1d;
    &execConfigCommand("rd",$regAddressUpper,1);
    $v_TDCThresholdUpper = hex($commandReturn[3]);
    &execConfigCommand("rd",$regAddressLower,1);
    $v_TDCThresholdLower = hex($commandReturn[3]);

    $v_TDCThreshold = $v_TDCThresholdUpper*256+$v_TDCThresholdLower;
    printf("%35s (0x%02x,0x%02x) : %8d mV (0x%02x,0x%02x)\n", 
       "threshold level for ASD discri",
       $regAddressUpper, $regAddressLower,
       $v_TDCThreshold, $v_TDCThresholdUpper, $v_TDCThresholdLower);     
}

sub showADCThreshold {
    $regAddressUpper = 0x8;
    $regAddressLower = 0x9;
    &execConfigCommand("rd",$regAddressUpper,1);
    $v_ADCThresholdUpper = hex($commandReturn[3]);
    &execConfigCommand("rd",$regAddressLower,1);
    $v_ADCThresholdLower = hex($commandReturn[3]);

    $v_ADCThreshold = $v_ADCThresholdUpper*256+$v_ADCThresholdLower;
    printf("%35s (0x%02x,0x%02x) : %8d mV (0x%02x,0x%02x)\n", 
       "threshold level for ADC integration",
       $regAddressUpper, $regAddressLower,
       $v_ADCThreshold, $v_ADCThresholdUpper, $v_ADCThresholdLower);     
}

sub showADCPedestal {
    my ($ch) = @_;
    $regAddressUpper = 0x20 + $ch*2;
    $regAddressLower = 0x21 + $ch*2;
    &execConfigCommand("rd",$regAddressUpper,1);
    $v_ADCPedestalUpper = hex($commandReturn[3]);
    &execConfigCommand("rd",$regAddressLower,1);
    $v_ADCPedestalLower = hex($commandReturn[3]);

    $v_ADCPedestal = $v_ADCPedestalUpper*256+$v_ADCPedestalLower;
    printf("%33s%02d (0x%02x,0x%02x) : %8d mV (0x%02x,0x%02x)\n", 
       "ADC pedestal for ch",$ch,
       $regAddressUpper, $regAddressLower,
       $v_ADCPedestal, $v_ADCPedestalUpper, $v_ADCPedestalLower);     
}

sub showAllADCPedestal {
    for ($i=0;$i<$numOfChannels;$i++) {
    &showADCPedestal($i);
    }
}

sub showLemoout {
    $regAddressASD0   = 0x0c ;
    $regAddressASD1   = 0x0d ;
    $regAddressASD2   = 0x0e ;
    $regAddressASD3   = 0x0f ;
    $regAddressASD4   = 0x10 ;
    $regAddressASD5   = 0x11 ;
    $regAddressWindow = 0x12 ;

    &execConfigCommand("rd",$regAddressASD0,1);
    $v_ASD0 = sprintf("%02x",hex($commandReturn[3]));
    &execConfigCommand("rd",$regAddressASD1,1);
    $v_ASD1 = sprintf("%02x",hex($commandReturn[3]));
    &execConfigCommand("rd",$regAddressASD2,1);
    $v_ASD2 = sprintf("%02x",hex($commandReturn[3]));
    &execConfigCommand("rd",$regAddressASD3,1);
    $v_ASD3 = sprintf("%02x",hex($commandReturn[3]));
    &execConfigCommand("rd",$regAddressASD4,1);
    $v_ASD4 = sprintf("%02x",hex($commandReturn[3]));
    &execConfigCommand("rd",$regAddressASD5,1);
    $v_ASD5 = sprintf("%02x",hex($commandReturn[3]));
    &execConfigCommand("rd",$regAddressWindow,1);
    $v_Window = hex($commandReturn[3]);

    $str_Lemoout = "enable";
    if (($v_ASD0)==0 && ($v_ASD1)==0 && ($v_ASD2)==0 && ($v_ASD3)==0 && ($v__ASD4)==0 && ($v_ASD5)==0) {
    $str_Lemoout = "disable";
    }

    printf("%35s (0x%02x-0x%02x) : %s \n", 
       "LEMO OUT Function",
       $regAddressASD0, $regAddressASD5, $str_Lemoout);
    printf("%35s (0x%02x)      : %-10s  (0x%02x)\n", 
       " enabled channel in ASD0",
       $regAddressASD0, unpack("B8", pack("H2","$v_ASD0")), $v_ASD0);
    printf("%35s (0x%02x)      : %-10s  (0x%02x)\n", 
       " enabled channel in ASD1",
       $regAddressASD1, unpack("B8", pack("H2","$v_ASD1")), $v_ASD1);
    printf("%35s (0x%02x)      : %-10s  (0x%02x)\n", 
       " enabled channel in ASD2",
       $regAddressASD2, unpack("B8", pack("H2","$v_ASD2")), $v_ASD2);
    printf("%35s (0x%02x)      : %-10s  (0x%02x)\n", 
       " enabled channel in ASD3",
       $regAddressASD3, unpack("B8", pack("H2","$v_ASD3")), $v_ASD3);
    printf("%35s (0x%02x)      : %-10s  (0x%02x)\n", 
       " enabled channel in ASD4",
       $regAddressASD4, unpack("B8", pack("H2","$v_ASD4")), $v_ASD4);
    printf("%35s (0x%02x)      : %-10s  (0x%02x)\n", 
       " enabled channel in ASD5",
       $regAddressASD5, unpack("B8", pack("H2","$v_ASD5")), $v_ASD5);
    printf("%35s (0x%02x)      : %8.1f ns (0x%02x clocks)\n", 
       "LEMO OUT Coincidence Window ",
       $regAddressWindow, $v_Window*(1/120*1000), $v_Window);
}


#----------------------------------------------------------------------
# Functions to set register
#----------------------------------------------------------------------
sub setDaqMode {
    $regAddress = 0x5;
    my ($setValue) = @_;  
    &execConfigCommand("wrb",$regAddress,$setValue);
    &showDaqMode;
}

sub setWindowSize {
    $regAddress = 0x6;
    my ($setValue) = @_;  
    &execConfigCommand("wrb",$regAddress,$setValue);
    &showWindowSize;
}

sub setDelay {
    $regAddress = 0x7;
    my ($setValue) = @_;  
    &execConfigCommand("wrb",$regAddress,$setValue);
    &showDelay;
}

sub setManchester {
    $regAddress = 0xA;
    my ($setValue) = @_;  
    &execConfigCommand("wrb",$regAddress,$setValue);
    &showManchester;
}

sub setTOT {
    $regAddress = 0xB;
    my ($setValue) = @_;  
    &execConfigCommand("wrb",$regAddress,$setValue);
    &showTOT;
}

sub setBusyFunction {
    $regAddress = 0x1E;
    my ($setValue) = @_;  
    &execConfigCommand("wrb",$regAddress,$setValue);
    &showBusyFunction;
}

sub setBusyWidth {
    $regAddress = 0x1F;
    my ($setValue) = @_;  
    &execConfigCommand("wrb",$regAddress,$setValue);
    &showBusyWidth;
}

sub setTDCThreshold {
    my ($setValue) = @_;
    if ($setValue =~ /^0x/) { $setValue = hex($setValue); }
    $upperValue = ($setValue & 0xff00) >> 8;
    $lowerValue = ($setValue & 0x00ff);
    $regAddress = 0x1c; &execConfigCommand("wrb",$regAddress,$upperValue);
    $regAddress = 0x1d; &execConfigCommand("wrb",$regAddress,$lowerValue);
    &showTDCThreshold;
}

sub setADCThreshold {
    my ($setValue) = @_;
    if ($setValue =~ /^0x/) { $setValue = hex($setValue); }
    $upperValue = ($setValue & 0xff00) >> 8;
    $lowerValue = ($setValue & 0x00ff);
    $regAddress = 0x8; &execConfigCommand("wrb",$regAddress,$upperValue);
    $regAddress = 0x9; &execConfigCommand("wrb",$regAddress,$lowerValue);
    &showADCThreshold;
}

sub setADCPedestal {
    my ($ch, $setValue) = @_; 
    &showADCPedestal($ch);
    if ($setValue =~ /^0x/) { $setValue = hex($setValue); }
    $upperValue = ($setValue & 0xff00) >> 8;
    $lowerValue = ($setValue & 0x00ff);
    $regAddress = 0x20 + $ch*2; &execConfigCommand("wrb",$regAddress,$upperValue);
    $regAddress = 0x21 + $ch*2; &execConfigCommand("wrb",$regAddress,$lowerValue);
    &showADCPedestal($ch);
}

sub setLemooutChannel {
    my ($ch, $setValue) = @_;
    $regAddress = 0x0c + $ch;
    &execConfigCommand("wrb",$regAddress,$setValue);
    &showLemoout;
}

sub setLemooutWindow {
    $regAddress = 0x12;
    my ($setValue) = @_;  
    &execConfigCommand("wrb",$regAddress,$setValue);
    &showLemoout;
}