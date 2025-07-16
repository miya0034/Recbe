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
#include <TGraph.h>
#include <TApplication.h>
#include <TCanvas.h>
#include <TAxis.h>
#include <TStyle.h>
int debug=0;

TFile *file;
TTree *tree;
unsigned char* buf;

#define WINDOW_SIZE  32
#define NUMBER_OF_CH 48
#define MAX_TDCNHIT  32
#define NUMBER_OF_BOARD 1

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
	std::string message = "Usage: ./wf-viewer <view ch-num> <rootfilename> <SNo> <InputVoltage>";
	std::cerr << message << std::endl;
	return 0;
}

int main(int argc, char *argv[]){
	if (argc != 5) {
		usage();
		exit(1);
	}
	int nCh = atoi(argv[1]);
	TString filePath = TString("../ROOT/") + TString(argv[3]) + "/" + TString(argv[2]);
	file = new TFile(filePath, "read");
    TString outputDir = TString("../ROOT/") + TString(argv[3]) + "/waveforms";
    TString outName = Form("%s_%d_%d_%s_waveform.pdf",TString(argv[4]).Data(), nCh,nCh+7,TString(argv[3]).Data());
	if (!file || file->IsZombie()) {
    	std::cerr << "Error opening file" << std::endl;
    	return 0;
	}
	file->GetObject("tree", tree);
	if (!tree) {
    	std::cerr << "Error: TTree not found" << std::endl;
    	return 0;
	}
	

//	tree = new TTree("tree","recbe");
	tree->SetBranchAddress("triggerNumber",&triggerNumber);
	tree->SetBranchAddress("triggerTime",&triggerTime);
	tree->SetBranchAddress("adc",adc);
	tree->SetBranchAddress("tdc",tdc);
	tree->SetBranchAddress("q",q);
	tree->SetBranchAddress("tdcNhit",tdcNhit);
	tree->SetBranchAddress("driftTime",driftTime);
	tree->SetBranchAddress("clockNumberDriftTime",clockNumberDriftTime);
	
	TApplication app("app", &argc, argv);
    TCanvas* canvas = new TCanvas("adcaa", "", 1800, 1600); 
 
	int T[32];
	for(int i = 0; i<32; ++i){
		T[i] = i;
	}


    TGraph* grwf[8];
	for(int i = 0; i<8; ++i){
		grwf[i] = new TGraph();
	}

	for(int i = 0; i<8; ++i){
		for(int entry = 0; entry<tree->GetEntries(); ++entry){
			tree->GetEntry(entry);
			for(int j = 0; j<32; ++j){
				if(nCh==0){
					int nnCh = nCh + 24;
					int tdchit = clockNumberDriftTime[nnCh+i][0];
					int deltai = j-0*tdchit;
					double t = -driftTime[nnCh+i][0] + (deltai*32) - triggerTime%(32);
						//double t = -driftTime[nCh+i][0] + (deltai*32) - tdc[nCh+i][tdchit]%(32);
					grwf[i]->AddPoint(t/0.96, adc[nnCh+i][j]);
				}else if(nCh==24){
					int nnCh = nCh - 24;
					int tdchit = clockNumberDriftTime[nnCh+i][0];
					int deltai = j-0*tdchit;
					double t = -driftTime[nnCh+i][0] + (deltai*32) - triggerTime%(32);
						//double t = -driftTime[nCh+i][0] + (deltai*32) - tdc[nCh+i][tdchit]%(32);
					grwf[i]->AddPoint(t/0.96, adc[nnCh+i][j]);
				}else{
					int tdchit = clockNumberDriftTime[nCh+i][0];
					int deltai = j-0*tdchit;
					double t = -driftTime[nCh+i][0] + (deltai*32) - triggerTime%(32);
						//double t = -driftTime[nCh+i][0] + (deltai*32) - tdc[nCh+i][tdchit]%(32);
					grwf[i]->AddPoint(t/0.96, adc[nCh+i][j]);
				}
			}

		}
	}

	canvas->Divide(2,4);
	for(int i = 0; i<8; ++i){
		canvas->cd(i+1);
    gStyle->SetLabelSize(0.05, "X");
    gStyle->SetLabelSize(0.05, "Y");
    gStyle->SetLabelSize(0.05, "Z");
		if(nCh==0){
			grwf[i]->SetTitle(Form("Waveform ch%d;time(ns);ADC",nCh+i+24));		
		}else if(nCh==24){
			grwf[i]->SetTitle(Form("Waveform ch%d;time(ns);ADC",nCh+i-24));		
		}else{
			grwf[i]->SetTitle(Form("Waveform ch%d;time(ns);ADC",nCh+i));		
		}
		

		    // 自動範囲設定
    double xmin, ymin, xmax, ymax;
    grwf[i]->ComputeRange(xmin, ymin, xmax, ymax);

    // 少しマージンをとるなら以下のように調整しても良い
    double ymargin = (ymax - ymin) * 0.05;
    grwf[i]->GetYaxis()->SetRangeUser(ymin - ymargin, ymax + ymargin);

		//Draw the graph on the canvas
		grwf[i]->SetMarkerStyle(20);
		grwf[i]->SetMarkerColor(kBlue);
		grwf[i]->SetMarkerSize(0.1);
		grwf[i]->Draw("ap");
	}
	//canvas->Draw();//Print("test.pdf");
    canvas->SaveAs(outputDir + "/" + outName);
    std::cout << "Saved: " << outputDir << "/" << outName << std::endl;

    file->Close();
    delete file;

}	
