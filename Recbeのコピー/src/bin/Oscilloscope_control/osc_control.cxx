#include "osc_control.hxx"

int const SocketConnect(){
	int socketOsc;
	struct sockaddr_in serverAddr;

	//create socket
	if(verbose >= 1) std::cout<<"Creating socket..."<<std::endl;
	socketOsc = socket(AF_INET, SOCK_STREAM, 0);
	if(socketOsc == -1){
		std::cerr<<"Failed to create socket"<<std::endl;
		exit(-1);
	}
	if(verbose >= 1) std::cout<<"Created socket SID = "<<socketOsc<<std::endl;
	
	//socket connect
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(portNum);
	serverAddr.sin_addr.s_addr = inet_addr(host_ip);

	if(verbose >= 1) std::cout<<"Connecting to remote host..."<<std::endl;
	if( connect(socketOsc, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0){
		std::cerr<<"Failed to connect to remote host"<<std::endl;
		exit(-1);
	}

	return socketOsc;

}

void SendCommand(int const socketOsc, char const* cmd){
	//send command
	if(verbose >= 2) std::cout<<"Sending command: "<<cmd<<std::endl;
	if( send(socketOsc, cmd, strlen(cmd), 0) <= 0){
		std::cerr<<"Failed to send command"<<std::endl;
		exit(-1);
	}
}

void SocketQuery(int const socketOsc, char const* cmd){
	char recv_str[256];
	//send command
	SendCommand(socketOsc, cmd);
        
	//receive reponse from remote host
	memset(recv_str, 0, sizeof(recv_str));         //memory initialize
	if(verbose >= 1) std::cout<<"Receiving reponse..."<<std::endl;
	while( int ret = recv(socketOsc, recv_str, sizeof(recv_str), 0) > 0){
		std::cout<<recv_str<<std::endl; 
		if(strchr(recv_str, '\n') != NULL) break;
		memset(recv_str, 0, sizeof(recv_str)); 
	}

	close(socketOsc);
}

std::string GetParameter(int const socketOsc, std::string const& cmd){
	char recv_str[256];
	SendCommand(socketOsc, cmd.c_str());
	
	//receive reponse from remote host
	memset(recv_str, 0, sizeof(recv_str));         //memory initialize
	if(verbose >= 2) std::cout<<"Receiving parameter..."<<std::endl;
	if( recv(socketOsc, recv_str, sizeof(recv_str), 0) <= 0){
		std::cerr<<"Failed to get parameter from oscilloscope"<<std::endl;
		exit(-1);
	}

	if(verbose >= 2) std::cout<<"Successed"<<std::endl;
	return recv_str;
}

std::string MakeString(double input){   //This function exists because the precision of std::to_string() is 6 and can not be changed.
	std::stringstream ss;
	ss<<input;
	return ss.str();
}

void ReadData(int const socketOsc, std::string const& Chs, int const nEvent, std::string const& dirNameExternal){
	//reset the parameters for xZero, xIncr, yZero, yMult. Without setting mode to RUNStop these parameters can not be successfully set.
	SendCommand(socketOsc, "ACQuire:STOPAfter RUNStop\n");
	SendCommand(socketOsc, "ACQuire:STATE RUN\n");
	//Calculate time cost
	clock_t tStart = clock();
	//get channel numbers
	std::vector<int> ch;
	std::istringstream ss_ch(Chs);
	std::string tmp;
	while(getline(ss_ch, tmp, ',')){
		ch.push_back(stoi(tmp));
	}
	int const nCh = ch.size();
	//create directory by present date and time
	time_t timep;
	time(&timep);
	char dirNameTmp[64];
	strftime(dirNameTmp, sizeof(dirNameTmp), "%Y-%m-%d-%H-%M-%S", localtime(&timep) );
	std::string dirName(dirNameTmp);
    //or decided externally
    if(dirNameExternal.size() != 0) dirName = dirNameExternal;
	if( mkdir(dirName.c_str(), S_IRWXU) != 0){
		std::cerr<<"Failed to create directory to save waveform data"<<std::endl;
		exit(-1);
	}
	//get parameters
	SendCommand(socketOsc, "HEADer OFF\n");
	std::vector<int> num_pt, num_byte;
	std::vector<double> xZero, yZero, xIncr, yMult, yPos, yScale;
	for(int iCh=0; iCh<nCh; ++iCh){
		std::string cmd_select = "SELect:CH" + std::to_string(ch.at(iCh)) + " ON\n";
		std::string cmd_source = "DATa:SOUrce CH" + std::to_string(ch.at(iCh)) + "\n";
		SendCommand(socketOsc, cmd_select.c_str());
		SendCommand(socketOsc, cmd_source.c_str());
		num_pt.push_back(   stoi(GetParameter(socketOsc, "WFMOutpre:NR_pt?\n")));
		yPos.push_back(     stod(GetParameter(socketOsc, "CH" + std::to_string(ch.at(iCh)) +":POSition?\n")));
		yScale.push_back(   stod(GetParameter(socketOsc, "CH" + std::to_string(ch.at(iCh)) +":SCAle?\n")));
		
		xZero.push_back(    stod(GetParameter(socketOsc, "WFMOutpre:XZEro?\n")));
		xIncr.push_back(    stod(GetParameter(socketOsc, "WFMOutpre:XINcr?\n")));
		yZero.push_back(    stod(GetParameter(socketOsc, "WFMOutpre:YZEro?\n")));
		yMult.push_back(    stod(GetParameter(socketOsc, "WFMOutpre:YMUlt?\n")));
		num_byte.push_back( stoi(GetParameter(socketOsc, "WFMOutpre:BYT_Nr?\n")));

		if(verbose >= 2){
			std::cout<<"ch"<<ch.at(iCh)<<":"<<std::endl;
			std::cout<<"xZero = "<<xZero.at(iCh)<<std::endl;
			std::cout<<"xIncr = "<<xIncr.at(iCh)<<std::endl;
			std::cout<<"yScale = "<<yScale.at(iCh)<<std::endl;
			std::cout<<"yZero = "<<yZero.at(iCh)<<std::endl;
			std::cout<<"yMult = "<<yMult.at(iCh)<<std::endl;
		}
	}

	//Read data
	std::string recv_str;
	char recv_char[9192];
	SendCommand(socketOsc, "ACQuire:STOPAfter SEQuence\n");
	for(int iEvent=0; iEvent < nEvent; ++iEvent){
		SendCommand(socketOsc, "ACQuire:STATE ON\n");
		SendCommand(socketOsc, "*WAI\n");
		for(int iCh=0; iCh < nCh; ++iCh){
			std::string cmd = "DATa:SOUrce CH" + std::to_string(ch.at(iCh)) + "\n";
			SendCommand(socketOsc, cmd.c_str());
			recv_str.clear();
			memset(recv_char, 0, sizeof(recv_char));
			SendCommand(socketOsc, "CURVe?\n");
			while( recv(socketOsc, recv_char, sizeof(recv_char), 0) > 0){
				recv_str.append( recv_char );
				if(strchr(recv_char, '\n') != NULL) break;
				memset(recv_char, 0, sizeof(recv_char));
			}
			//Save waveform to file
			std::string fileName = "Waveform_ch" + std::to_string(ch.at(iCh)) + "_event" + std::to_string(iEvent) + ".txt";
			std::ofstream f_out(dirName + "/" + fileName, std::ios::out);
			std::istringstream ss(recv_str);
			std::string pt;
			int index = 0;
			while(getline(ss, pt, ',')){
				f_out << xZero.at(iCh) + xIncr.at(iCh) * index
				<<" "
				<< yMult.at(iCh) * stod(pt) - ( yPos.at(iCh) * yScale.at(iCh) )
				<< std::endl;
				++index;
			}
			f_out.close();
		}
		if(verbose >= 1){
			if(iEvent == 0) std::cout<<iEvent + 1<<"/"<<nEvent<<" finished"<<std::endl;
			else std::cout<<"\033[1A\033[K\033[32m"<<iEvent + 1<<"/"<<nEvent<<" finished"<<"\033[0m"<<std::endl;
		}
	}
	if(verbose >= 1){
		time_t t_end;
		time(&t_end);
		std::cout<<"Finished reading waveform"<<std::endl;
		std::cout<<difftime(t_end, timep)<<" s time cost for waveform acquisition"<<std::endl;
	}
	if(verbose >= 2) std::cout<<std::fixed<<std::setprecision(5)<<(double)(clock() - tStart)/CLOCKS_PER_SEC<<" s CPU time for waveform acquisition"<<std::endl;
}

std::string TranslateCommand(std::string const& mode, std::string const& cmd){
	if(mode == "-q" || mode == "--query"){
		if(cmd == "identify") return "*IDN?\n";
		else if (cmd == "esr")               return "*ESR?\n";

		else if (cmd == "triggerType?")      return "TRIgger:A:TYPe?\n";
		else if (cmd == "triggerThreshold?") return "TRIgger:A:LEVel?\n";
		else if (cmd == "edgeSource?")       return "TRIgger:A:EDGE:SOUrce?\n";
		else if (cmd == "edgeCoupling?")     return "TRIgger:A:EDGE:COUpling?\n";
		else if (cmd == "edgeSlope?")        return "TRIgger:A:EDGE:SLOpe?\n";

		else if (cmd == "channel?")          return "SELect?\n";

		else if (cmd == "waveformAcquire?")  return "ACQuire:STOPAfter?\n";
		else if (cmd == "acquireState?")     return "ACQuire:STATE?\n";
		else if (cmd == "waveform?")         return "WAVFrm?\n";
		else if (cmd == "waveformInfo?")     return "WFMOutpre?\n";

		else if (cmd == "xOffset?")          return "WFMInpre:XZEro?\n";
		else if (cmd == "xResolution?")      return "WFMInpre:XINcr?\n";
		else if (cmd == "xUnit?")            return "WFMInpre:XUNit?\n";

		else if (cmd == "yOffsetCH1?")       return "CH1:POSition?\n";
		else if (cmd == "yOffsetCH2?")       return "CH2:POSition?\n";
		else if (cmd == "yOffsetCH3?")       return "CH3:POSition?\n";
		else if (cmd == "yOffsetCH4?")       return "CH4:POSition?\n";
		else if (cmd == "yResolution?")      return "WFMInpre:YMUlt?\n";
		else if (cmd == "yUnit?")            return "WFMInpre:YUNit?\n";

		else if (cmd == "dataStop?")         return "DATA:STOP?\n";

		else if (cmd == "meas1StdDev?")              return "MEASUrement:MEAS1:STDdev?\n";
		else if (cmd == "meas2StdDev?")              return "MEASUrement:MEAS2:STDdev?\n";
		else if (cmd == "meas3StdDev?")              return "MEASUrement:MEAS3:STDdev?\n";
		else if (cmd == "meas4StdDev?")              return "MEASUrement:MEAS4:STDdev?\n";
		else{
			std::cerr<<"no such command!"<<std::endl;
			exit(-1);
		}
	}
	else{
		std::cerr<<"no such mode!"<<std::endl;
		exit(-1);
	}
}

std::string TranslateCommand(std::string const& mode, std::string const& cmd, std::string const& parameter){
	if(mode == "-s" || mode == "--send"){
		if(cmd == "triggerType")             return "TRIgger:A:TYPe " + parameter + "\n";
		else if (cmd == "triggerThreshold")  return "TRIgger:A:LEVel " + parameter + "\n";
		else if (cmd == "edgeSource")        return "TRIgger:A:EDGE:SOUrce CH" + parameter + "\n";
		else if (cmd == "edgeCoupling")      return "TRIgger:A:EDGE:COUpling " + parameter + "\n";
		else if (cmd == "edgeSlope")         return "TRIgger:A:EDGE:SLOpe " + parameter + "\n";

		else if (cmd == "channel1")          return "SELect:CH1 " + parameter + "\n";
		else if (cmd == "channel2")          return "SELect:CH2 " + parameter + "\n";
		else if (cmd == "channel3")          return "SELect:CH3 " + parameter + "\n";
		else if (cmd == "channel4")          return "SELect:CH4 " + parameter + "\n";
		else if (cmd == "channelSelect")     return "SELect:CONTROL " + parameter + "\n";  

		else if (cmd == "waveformAcquire")   return "ACQuire:STOPAfter " + parameter + "\n";
		else if (cmd == "acquireState")      return "ACQuire:STATE " + parameter + "\n";

		else if (cmd == "header")            return "HEADer " + parameter + "\n";

		else if (cmd == "xOffset")           return "WFMInpre:XZEro " + parameter + "\n";
		else if (cmd == "xResolution")       return "WFMInpre:XINcr " + parameter + "\n";
		else if (cmd == "xScale")            return "HORizontal:SCAle " + parameter + "\n";

		else if (cmd == "yOffset")           return "WFMInpre:YZEro " + parameter + "\n";
		else if (cmd == "yResolution")       return "WFMInpre:YMUlt " + parameter + "\n";
		else if (cmd == "yScaleCH1")         return "CH1:SCAle " + parameter + "\n";
		else if (cmd == "yScaleCH2")         return "CH2:SCAle " + parameter + "\n";
		else if (cmd == "yScaleCH3")         return "CH3:SCAle " + parameter + "\n";
		else if (cmd == "yScaleCH4")         return "CH4:SCAle " + parameter + "\n";

		else if (cmd == "meas1Source")       return "MEASUrement:MEAS1:SOUrce CH" + parameter + "\n";
		else if (cmd == "meas2Source")       return "MEASUrement:MEAS2:SOUrce CH" + parameter + "\n";
		else if (cmd == "meas3Source")       return "MEASUrement:MEAS3:SOUrce CH" + parameter + "\n";
		else if (cmd == "meas4Source")       return "MEASUrement:MEAS4:SOUrce CH" + parameter + "\n";

		else if (cmd == "sendCommand")       return parameter + "\n";
		else{ std::cerr<<"no such command!"<<std::endl; exit(-1); }
	}
	else{ std::cerr<<"no such mode!"<<std::endl; exit(-1); }

}

void Usage(){
	std::cout<<"./osc_control <mode> <command> <parameter>"<<std::endl;
	std::cout<<"mode <\033[32m-q\033[0m> or <\033[32m--query\033[0m>: get information (no parameter needed)"<<std::endl;
	std::cout<<"notice that for the last question mark you need to put a \\ before it"<<std::endl;
	std::cout<<"     command <\033[32midentify\033[0m> returns machine information"<<std::endl;
	std::cout<<"     command <\033[32mtriggerType?\033[0m> returns current trigger type"<<std::endl;
	std::cout<<"     command <\033[32mtriggerThreshold?\033[0m> returns the threshold of trigger"<<std::endl;
	std::cout<<"     command <\033[32medgeSource?\033[0m> returns the source edge of trigger (which channel)"<<std::endl;
	std::cout<<"     command <\033[32medgeCoupling?\033[0m> returns the coupling method of edge trigger"<<std::endl;
	std::cout<<"     command <\033[32medgeSlope?\033[0m> returns the slope of edge trigger [rise, falling or either]"<<std::endl;
	std::cout<<"     command <\033[32mchannel?\033[0m> returns which channel is turned on and which channel is being selected"<<std::endl;
	std::cout<<"     command <\033[32mwaveformAcquire?\033[0m> returns method of waveform acquisition [runStop, sequence]"<<std::endl;
	std::cout<<"     command <\033[32macquisitionState?\033[0m> returns the state of waveform acquisition [on, off]"<<std::endl;
	std::cout<<"     command <\033[32mwaveform?\033[0m> returns headers and data points (8-bit) of waveform"<<std::endl;
	std::cout<<"     command <\033[32mwaveformInfo?\033[0m> returns headers of waveform"<<std::endl;
	std::cout<<"     command <\033[32mxOffset?\033[0m> returns horizontal offset of waveform"<<std::endl;
	std::cout<<"     command <\033[32mxResolution?\033[0m> returns horizontal resolution of waveform"<<std::endl;
	std::cout<<"     command <\033[32mxUnit?\033[0m> returns horizontal unit of waveform [s, Hz]"<<std::endl;
	std::cout<<"     command <\033[32myOffset?\033[0m> returns vertical offset of waveform"<<std::endl;
	std::cout<<"     command <\033[32myResolution?\033[0m> returns vertical resolution of waveform"<<std::endl;
	std::cout<<"     command <\033[32myUnit?\033[0m> returns vertical unit of waveform [V, mV, etc.]"<<std::endl;
	std::cout<<"     command <\033[32mdataStop?\033[0m> returns number of data points will be transferred"<<std::endl;
	std::cout<<"     command <\033[32mmeas1StdDev?\033[0m> returns standard deviation from measurement block 1"<<std::endl;
	std::cout<<"     command <\033[32mmeas2StdDev?\033[0m> returns standard deviation from measurement block 2"<<std::endl;
	std::cout<<"     command <\033[32mmeas3StdDev?\033[0m> returns standard deviation from measurement block 3"<<std::endl;
	std::cout<<"     command <\033[32mmeas4StdDev?\033[0m> returns standard deviation from measurement block 4"<<std::endl;
	std::cout<<"mode <\033[32m-s\033[0m> or <\033[32m--send\033[0m>: send command to oscilloscope (need parameter after command)"<<std::endl;
	std::cout<<"     command <\033[32mtriggerType\033[0m> specifies the trigger type [edge, logic, pulse, bus, video]"<<std::endl;
	std::cout<<"     command <\033[32mtriggerThreshold\033[0m> specifies the threshold of trigger"<<std::endl;
	std::cout<<"     command <\033[32medgeSource\033[0m> specifies the source edge of trigger (which channel)"<<std::endl;
	std::cout<<"     command <\033[32medgeCoupling\033[0m> specifies the coupling method of edge trigger"<<std::endl;
	std::cout<<"     command <\033[32medgeSlope\033[0m> specifies the slope of edge trigger [rise, falling, either]"<<std::endl;
	std::cout<<"     command <\033[32mchannel1\033[0m> turn [on, off] the channel 1"<<std::endl;
	std::cout<<"     command <\033[32mchannel2\033[0m> turn [on, off] the channel 2"<<std::endl;
	std::cout<<"     command <\033[32mchannel3\033[0m> turn [on, off] the channel 3"<<std::endl;
	std::cout<<"     command <\033[32mchannel4\033[0m> turn [on, off] the channel 4"<<std::endl;
	std::cout<<"     command <\033[32mchannelSelect\033[0m> specifies the channel that been selected to use"<<std::endl;
	std::cout<<"     command <\033[32mwaveformAcquire\033[0m> specifies the waveform acquisition method [runStop, sequence]"<<std::endl;
	std::cout<<"     command <\033[32macquireState\033[0m> turn [on, off] the waveform acquisition"<<std::endl;
	std::cout<<"     command <\033[32mheader\033[0m> turn [on, off] the header information"<<std::endl;
	std::cout<<"     command <\033[32mxOffset\033[0m> specifies the horizontal offset"<<std::endl;
	std::cout<<"     command <\033[32mxScale\033[0m> specifies the horizontal scale [s/div]"<<std::endl;
	std::cout<<"     command <\033[32myOffset\033[0m> specifies the vertical offset"<<std::endl;
	std::cout<<"     command <\033[32myScaleCH1\033[0m> specifies the vertical scale of channel 1 [V/div]"<<std::endl;
	std::cout<<"     command <\033[32myScaleCH2\033[0m> specifies the vertical scale of channel 2 [V/div]"<<std::endl;
	std::cout<<"     command <\033[32myScaleCH3\033[0m> specifies the vertical scale of channel 3 [V/div]"<<std::endl;
	std::cout<<"     command <\033[32myScaleCH4\033[0m> specifies the vertical scale of channel 4 [V/div]"<<std::endl;
	std::cout<<"     command <\033[32mmeas1Source\033[0m> specifies the source of channel [1,2,3,4] to measurement block 1"<<std::endl;
	std::cout<<"     command <\033[32mmeas2Source\033[0m> specifies the source of channel [1,2,3,4] to measurement block 2"<<std::endl;
	std::cout<<"     command <\033[32mmeas3Source\033[0m> specifies the source of channel [1,2,3,4] to measurement block 3"<<std::endl;
	std::cout<<"     command <\033[32mmeas4Source\033[0m> specifies the source of channel [1,2,3,4] to measurement block 4"<<std::endl;
	std::cout<<"     command <\033[32msendCommand\033[0m> directly send command to oscilloscope"<<std::endl;
	std::cout<<"mode <\033[32m-r\033[0m> or <\033[32m--read\033[0m>: read waveform from oscilloscope and save as txt files"<<std::endl;
	std::cout<<"              \033[33m./osc_control <mode> <channels> <number of events> <dirName (optional)>"<<std::endl;
	std::cout<<"         e.g. \033[33m./osc_control -r 1,4 100\033[0m will read 100 events from channel 1 and channel 4 respectively"<<std::endl;
}

int main(int argc, char** argv){
	if(argc <3 || argc > 5){
		Usage();
		exit(-1);
	}
	std::string mode = argv[1];
	std::string cmd;
	int const socketOsc = SocketConnect();

	if(verbose >= 1) std::cout<<"mode = "<<mode << std::endl;
	if(mode == "-q" || mode == "--query"){
		std::cout<<"Query mode"<<std::endl;
		cmd = TranslateCommand(mode, argv[2]);
		SocketQuery(socketOsc, cmd.c_str());
	}
	else if(mode == "-s" || mode == "--send"){
		if(argc != 4){
			std::cerr<<"need parameter for this mode!"<<std::endl;
			exit(-1);
		}
		if(verbose >= 1) std::cout<<"Send command to remote host mode"<<std::endl;
		cmd = TranslateCommand(mode, argv[2], argv[3]);
		SendCommand(socketOsc, cmd.c_str());
	}
	else if(mode == "-r" || mode == "--read"){
		if(argc < 4 || argc > 5){
			std::cerr<<"need parameter for this mode!"<<std::endl;
			exit(-1);
		}
		if(verbose >= 1) std::cout<<"Data acquisition mode"<<std::endl;
		std::string Chs = argv[2];
		int nEvent = atoi(argv[3]);
        std::string dirName = "";
        if(argc == 5){
            dirName = argv[4];
        }
		ReadData(socketOsc, Chs, nEvent, dirName);
	}
	else if(mode == "-h" || mode == "--help"){
		Usage();
		exit(-1);
	}
	else{
		std::cerr<<"no such mode!"<<std::endl;
		std::cerr<<"use -h or --help argument to see help message"<<std::endl;
		exit(-1);
	}
		

	return 0;
}
