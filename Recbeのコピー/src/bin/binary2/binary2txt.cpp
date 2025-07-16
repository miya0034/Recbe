/* written by Hisataka Yoshida for raw and suppress mode */

#include <arpa/inet.h>      
#include <err.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
using namespace std;

unsigned char* HeaderBuf;
unsigned char* buf;

#define WINDOW_SIZE  32
#define NUMBER_OF_CH 48
#define RAW_EVENT_SIZE 12+4*WINDOW_SIZE*NUMBER_OF_CH

FILE* fp;
int num_events=0;
int num_noteve=0;

int sendNumber;
int triggerNumber;
int triggerTime;
int adc[NUMBER_OF_CH][WINDOW_SIZE] ;
int tdc[NUMBER_OF_CH][WINDOW_SIZE] ;
int buftdc[NUMBER_OF_CH][WINDOW_SIZE];
int adcCOT[NUMBER_OF_CH]; // count over threshold
int q[NUMBER_OF_CH];
int driftTime[NUMBER_OF_CH][WINDOW_SIZE];   
int clockNumberDriftTime[NUMBER_OF_CH][WINDOW_SIZE];
int tdcNhit[NUMBER_OF_CH];

int packet_id; // 32 -> rawmode, 34 -> suppress mode
int board_id;
int send_number;
int trigger_time;
int data_length;
int trigger_counter;

int decode(){
  // decode Header
  int HeaderSize = 1 + 1 + 2 + 2 + 2 + 4; // 12 byte
  HeaderBuf = (unsigned char*)malloc(HeaderSize*sizeof(unsigned char));
  if (HeaderBuf==NULL) {
    fprintf(stderr,"failed to malloc my_buf\n");
    exit(1);
  }
  int HeaderNbyte = fread(HeaderBuf, 1 , HeaderSize, fp); // try to read (1*length) bytes
  if(HeaderNbyte == 0) {
    if(feof(fp)){          // End of File
      return -1;
    }
    else if(ferror(fp)) { // cannot read data
      printf("\ntotal event = %d\nfinish to make root file\n",num_events-1);
      err(1, "fread");
    }
    else{                 // etc error
      printf("\ntotal event = %d\nfinish to make root file\n",num_events-1);
      errx(1, "unknown error");
    }
  }
  else if(HeaderNbyte != HeaderSize) {
    errx(1, "short read: try to read %d but returns %d bytes", 1*HeaderSize, HeaderNbyte);
    return -1;
  }  

  packet_id       = HeaderBuf[0];
  board_id        = HeaderBuf[1];
  send_number     = HeaderBuf[2]*256 + HeaderBuf[3];
  //  trigger_time    = HeaderBuf[4]*256 + HeaderBuf[5];
  trigger_time    = ((HeaderBuf[4]*256 + HeaderBuf[5])&(0x7fff)); //16th bit is not trigger time in raw mode but valid of TDC
  triggerTime = trigger_time ;
  data_length          = HeaderBuf[6]*256 + HeaderBuf[7];

  unsigned int *trig = (unsigned int *)&HeaderBuf[8];
  trigger_counter = ntohl(*trig);
  triggerNumber = trigger_counter;
  //std::cout << triggerNumber << std::endl;
  
  if(data_length==0){
    num_noteve++;
    std::cout << "No Channel Data is included in the event with triggerNumber : " << triggerNumber << std::endl;
    return 0;
  }

  // decode data
  
  buf = (unsigned char*)malloc(data_length*sizeof(unsigned char));
  if (buf==NULL) {
    fprintf(stderr,"failed to malloc my_buf\n");
    exit(1);
  }
  int nbyte = fread(buf, 1 , data_length, fp); // try to read (1*length) bytes
  if (nbyte == 0) {
    if (feof(fp)) {          // End of File
      return -1;
    } else if (ferror(fp)) { // cannot read data
      printf("\ntotal event = %d\nfinish to make root file\n",num_events-1);
      err(1, "fread");
    } else {                 // etc error
      printf("\ntotal event = %d\nfinish to make root file\n",num_events-1);
      errx(1, "unknown error");
      }
  } else if (nbyte != data_length) {
      errx(1, "short read: try to read %d but returns %d bytes", 1*data_length, nbyte);
      return -1;
  }

  if(packet_id==34){ // Raw Mode    
    unsigned short *adc_data;
    unsigned short *tdc_data;
    
    for(int clk=0;clk<WINDOW_SIZE;clk++){
      for(int ch=0;ch<NUMBER_OF_CH;ch++){
	adc_data = (unsigned short *)&buf[ch*2+clk*NUMBER_OF_CH*2*2];
	adc[ch][clk] = ntohs(*adc_data);
	tdc_data = (unsigned short *)&buf[ch*2+clk*NUMBER_OF_CH*2*2+NUMBER_OF_CH*2];
	buftdc[ch][clk] = ntohs(*tdc_data); // indluding 16th bit valid
	if((buftdc[ch][clk]&(0x8000))>>15){
	  tdc[ch][clk] = (buftdc[ch][clk]&(0x7fff));
	  driftTime[ch][tdcNhit[ch]] = tdc[ch][clk] - triggerTime ;
	  if(driftTime[ch][tdcNhit[ch]]>0) driftTime[ch][tdcNhit[ch]] -= (1<<15) ;
	  clockNumberDriftTime[ch][tdcNhit[ch]] = clk;
	  tdcNhit[ch]++ ;
	}
      }
    }

  // text output 
    cout << "== Event Header"
         << ",PacketID:"    << packet_id 
         << ",BoardID:"     << board_id 
         << ",SendNumber:"  << send_number 
         << ",TriggerTime:" << trigger_time
         << ",Length:"      << data_length
         << ",TriggerNumber:" << trigger_counter
         << " ==" << endl;
    
    int nCh = 0 ;
    unsigned long long int hitMap = 0 ;
    for(int ch=0;ch<NUMBER_OF_CH;ch++){
      cout << "= Channel Header,Ch:" << ch << " =" << endl;
      for(int clk=0;clk<WINDOW_SIZE;clk++){
        q[ch] += adc[ch][clk] ;
        if(clk==0) cout  << adc[ch][clk] ;
        else cout << " " << adc[ch][clk] ;
      }
      cout << endl;

      for(int clk=0;clk<WINDOW_SIZE;clk++){
        if(clk==0) cout << tdc[ch][clk] ;
        else cout << " " << tdc[ch][clk] ;
      }
      cout << endl;
      cout << "= Channel Footer" 
           << ",tdcNhit:" << tdcNhit[ch]
           << ",Q:" << q[ch]
           << " =" << endl;
    }
    cout << "== Event Footer"
      //         << ",NHitChannel:" << nCh
      //         << ",hitMap:" << hitMap 
         << " ==" << endl;
    
    return 0; 
  }
  if(packet_id==32){ // Suppress Mode

    int pointer = 0;
    
    unsigned short *adc_COT;
    unsigned short *adc_data;
    unsigned short *tdc_data;
    
    int ch, tdc_num;
    
    while(pointer+2<data_length){
      ch = buf[pointer];
      pointer += 1;
      
      tdc_num = buf[pointer]/2 - 3;
      tdcNhit[ch] = tdc_num;
      pointer += 1;
      
      adc_COT = (unsigned short *)&buf[pointer];
      adcCOT[ch] = ntohs(*adc_COT);
      pointer += 2;
      
      adc_data = (unsigned short *)&buf[pointer];
      q[ch] = ntohs(*adc_data);
      pointer += 2;
    
      for(int clk=0; clk<tdc_num; clk++){
	tdc_data = (unsigned short *)&buf[pointer];
	int tdc_tmp = ntohs(*tdc_data);
	int tdc_diff = 0 - tdc_tmp; // tdc for suppress mode is subtracted with trigger_time
	driftTime[ch][clk] = tdc_diff;
	pointer += 2;
      }
    }
    // text output 
    cout << "== Event Header"
	 << ",PacketID:"    << packet_id 
	 << ",BoardID:"     << board_id 
	 << ",SendNumber:"  << send_number 
	 << ",TriggerTime:" << trigger_time
	 << ",Length:"      << data_length
	 << ",TriggerNumber:" << trigger_counter
	 << " ==" << endl;
    
    int nCh = 0 ;
    unsigned long long int hitMap = 0 ;
    for(ch=0;ch<48;ch++){
      if(tdcNhit[ch]>0){
	cout << "= Ch:" << ch
	     << ",tdcNhit:" << tdcNhit[ch]
	     << ",Q:" << q[ch] ;
	for(int clk=0;clk<tdcNhit[ch];clk++){
	  cout << ",driftTime[" << clk << "]:" << driftTime[ch][clk] ;
	}
	cout << " =" << endl;
	hitMap = hitMap | ((unsigned long long int)1<<ch) ;
	nCh++ ;
      }
    }
    cout << "== Event Footer"
	 << ",NHitChannel:" << nCh
	 << ",hitMap:" << hitMap 
	 << " ==" << endl;
  
    return 0;
  }
  else if(packet_id!=32 && packet_id!=34){
    cerr << "#ERROR : Paket ID is invalid (" << packet_id << ")" <<endl;
    return -1 ;
  }
  return -1 ;
}

int main(int argc, char *argv[]){
  if(argc!=2){
    cerr << "Usage: " << argv[0] << " binary_file" << endl;
    exit(1);
  }
  
  fp = fopen(argv[1], "r");
  
  if(fp == NULL) err(1,"fopen");
  
  int status = 0 ;
  num_events = 0;
  while (1) {
    // initialize for next event
    sendNumber    = -1;
    triggerTime   = -1;
    triggerNumber = -1;
    for(int ch=0; ch<48; ch++){
      adcCOT[ch]       = 0;
      q[ch]            = 0;
      tdcNhit[ch]      = 0;
      for(int clk=0;clk<WINDOW_SIZE;clk++){
	adc[ch][clk] = -1 ;
	tdc[ch][clk] = -1 ;
	driftTime[ch][clk] = -1;
	clockNumberDriftTime[ch][clk] = -1;
      }
    }

    status = decode();
    if(status==-1){
      break;
    }
    num_events++;
  }
  printf("\nerror event = %d\ntotal event = %d\n",num_noteve,num_events);
  return 0;
}
