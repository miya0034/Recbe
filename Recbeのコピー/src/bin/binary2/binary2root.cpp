#include <arpa/inet.h>
#include <errno.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <cstdio>
#include <fstream>
#include <iostream>

#include <TString.h>
#include <TROOT.h>
#include <TFile.h>
#include <TTree.h>

int debug=0;

TFile *file;
TTree *tree;
unsigned char* buf;

#define WINDOW_SIZE  32
#define NUMBER_OF_CH 48
#define MAX_TDCNHIT  32
#define NUMBER_OF_BOARD 1

int HEADER_SIZE;
int MY_BUFSIZE;
int POW_2_16; // pow(2,16)

int num_events=0;
int isample=0;
int triggerNumber;
int triggerTime ;
int adc[NUMBER_OF_CH][WINDOW_SIZE];
int tdc[NUMBER_OF_CH][WINDOW_SIZE];
int buftdc[NUMBER_OF_CH][WINDOW_SIZE];
int tdcHitFlag[NUMBER_OF_CH][WINDOW_SIZE];
int q[NUMBER_OF_CH];
int tdcNhit[NUMBER_OF_CH];
int driftTime[NUMBER_OF_CH][MAX_TDCNHIT];
int clockNumberDriftTime[NUMBER_OF_CH][MAX_TDCNHIT];

int usage(void)
{
	std::string message = "Usage: ./binary2rootC <datafilename> <rootfilename>";
	std::cerr << message << std::endl;
	return 0;
}

int decode()
{
	triggerTime = buf[4]*256 + buf[5];
	unsigned int *trig = (unsigned int *)&buf[8];
	triggerNumber = ntohl(*trig);

	if (debug) {
		printf("num_events  %d  ", num_events);
		printf("triggerNumber  %d  ", triggerNumber);
		printf("\n");
	}

	unsigned short *adc_data;
	unsigned short *tdc_data;

	for(int clk=0;clk<WINDOW_SIZE;clk++){
		for(int ch=0;ch<NUMBER_OF_CH;ch++){
			tdcHitFlag[ch][clk] = -1;
			adc_data = (unsigned short *)&buf[ch*2+clk*NUMBER_OF_CH*4+HEADER_SIZE];
			adc[ch][clk] = ntohs(*adc_data);
			tdc_data = (unsigned short *)&buf[(ch+NUMBER_OF_CH)*2+clk*NUMBER_OF_CH*4+HEADER_SIZE];
			buftdc[ch][clk] = ntohs(*tdc_data);
			tdcHitFlag[ch][clk] = (buftdc[ch][clk]&(0x8000))>>15;
			tdc[ch][clk] = (buftdc[ch][clk]&(0x7fff));
		}
	}

	for(int ch=0;ch<NUMBER_OF_CH;ch++){
		q[ch] =0;
		tdcNhit[ch] = 0;
		for (int iHit=0;iHit<MAX_TDCNHIT;iHit++) {
			driftTime[ch][iHit] = -9999;
			clockNumberDriftTime[ch][iHit] = -9999;
		}
		for(int clk=0;clk<WINDOW_SIZE;clk++){
			q[ch] += adc[ch][clk];
			if (tdcHitFlag[ch][clk]==1) {
				driftTime[ch][tdcNhit[ch]] = tdc[ch][clk] - triggerTime;
				if(driftTime[ch][tdcNhit[ch]]>0) driftTime[ch][tdcNhit[ch]] -= (1<<15) ;
				clockNumberDriftTime[ch][tdcNhit[ch]] = clk;
				// printf("driftTime[%d]=%d @ %d\n",ch, driftTime[ch][tdcNhit[ch]],clk );
				tdcNhit[ch]++;
			}
		}
		// printf("tdcNhit[%d]=%d\n",ch, tdcNhit[ch] );
	}

	return 0;
}

int main(int argc, char *argv[])
{
	if (argc != 3) {
		usage();
		exit(1);
	}

	printf("%s %s\n",argv[0],argv[1]);

	POW_2_16 = pow(2, 16);
	if (getenv("DEBUG")) {
		debug=atoi(getenv("DEBUG"));
	}

	HEADER_SIZE = 1 + 3 + 4 + 4;
	MY_BUFSIZE  = HEADER_SIZE + NUMBER_OF_CH*2*2*(WINDOW_SIZE);
	buf = (unsigned char*)malloc(NUMBER_OF_BOARD*MY_BUFSIZE*sizeof(unsigned char));
	if (buf==NULL) {
		fprintf(stderr,"failed to malloc my_buf\n");
		exit(1);
	}

	//   file = new TFile(Form("/data3/cdcTohoku/root/run_%06d_built.root",runNo),"recreate");
	file = new TFile(argv[2],"recreate");

	tree = new TTree("tree","recbe");
	tree->Branch("triggerNumber",&triggerNumber,"triggerNumber/I");
	tree->Branch("triggerTime",&triggerTime,"triggerTime/I");
	tree->Branch("adc",adc,Form("adc[%d][%d]/I",NUMBER_OF_CH,WINDOW_SIZE));
	tree->Branch("tdc",tdc,Form("tdc[%d][%d]/I",NUMBER_OF_CH,WINDOW_SIZE));
	tree->Branch("q",q,Form("q[%d]/I",NUMBER_OF_CH));
	tree->Branch("tdcNhit",tdcNhit,Form("tdcNhit[%d]/I",NUMBER_OF_CH));
	tree->Branch("driftTime",driftTime,Form("driftTime[%d][%d]/I",NUMBER_OF_CH,MAX_TDCNHIT));
	tree->Branch("clockNumberDriftTime",clockNumberDriftTime,Form("clockNumberDriftTime[%d][%d]/I",NUMBER_OF_CH,MAX_TDCNHIT));

	//   FILE* fp = fopen(Form("/data3/cdcTohoku/data-built/run_%06d_built.dat",runNo), "r");
	FILE* fp = fopen(Form("%s",argv[1]), "r");

	if (fp == NULL) {
		err(1,"fopen");
	}

	num_events = 0;
	while (1) {
		int nbyte = fread(buf, 1 , MY_BUFSIZE*NUMBER_OF_BOARD, fp); // try to read (1*MY_BUFSIZE) bytes
		if (debug>=2) printf("num_events %d nbyte %d\n", num_events, nbyte);
		if (nbyte == 0) {
			if (feof(fp)) {          // End of File
				tree->Write();
				file->Close();
				break;
			} else if (ferror(fp)) { // cannot read data
				tree->Write();
				file->Close();
				printf("\ntotal event = %d\nfinish to make root file\n",num_events-1);
				err(1, "fread");
			} else {                 // etc error
				tree->Write();
				file->Close();
				printf("\ntotal event = %d\nfinish to make root file\n",num_events-1);
				errx(1, "unknown error");
			}
		} else if (nbyte != MY_BUFSIZE*NUMBER_OF_BOARD) {
			errx(1, "short read: try to read %d but returns %d bytes", 1*MY_BUFSIZE*NUMBER_OF_BOARD, nbyte);
			break;
		}

		decode();

		tree->Fill();
		num_events++;
	}
	printf("\ntotal event = %d\nfinish to make root file\n",num_events);
	return 0;
}
