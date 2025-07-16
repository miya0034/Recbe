#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <TString.h>
#include <TH1D.h>
#include <TCanvas.h>
#include <TApplication.h>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <cmath>

void ensureDirectoryExists(const TString &dirPath) {
    struct stat st;
    std::string dirStr(dirPath.Data());
    if (stat(dirStr.c_str(), &st) != 0) {
        mkdir(dirStr.c_str(), 0777);
    }
}

void draw_hist_tdcAmp(const TString& SNo) {
    TString filepath = Form("../ROOT/%s/thresholdscan/fitting/tdcAmp/tdcAmp_fitparam.txt", SNo.Data());

    std::ifstream infile(filepath.Data());
    if (!infile.is_open()) {
        std::cerr << "Cannot open file: " << filepath << std::endl;
        return;
    }

    std::vector<double> intercepts;
    std::vector<double> slopes;

    std::string line;
    while (std::getline(infile, line)) {
        if (line.empty()) continue;
        if (line[0] == '#') continue;

        std::stringstream ss(line);
        std::string SNo_str, CH_str;
        double Intercept, InterceptErr, Slope, SlopeErr;
        ss >> SNo_str >> CH_str >> Intercept >> InterceptErr >> Slope >> SlopeErr;

        intercepts.push_back(Intercept);
        slopes.push_back(Slope);
    }
    infile.close();

    if (slopes.empty() || intercepts.empty()) {
        std::cerr << "No data to draw histograms." << std::endl;
        return;
    }

    // 固定範囲
    double slope_min = -7.5;
    double slope_max = -6.0;
    double intercept_min = 3820.0;
    double intercept_max = 3840.0;

    double slope_binWidth = 0.1;
    double intercept_binWidth = 1.0;

    int nBins_slope = static_cast<int>(std::round((slope_max - slope_min) / slope_binWidth));
    int nBins_intercept = static_cast<int>(std::round((intercept_max - intercept_min) / intercept_binWidth));

    TH1D *hSlope = new TH1D("hSlope", "Slope Distribution;Slope;Entries", 
                            nBins_slope, slope_min, slope_max);
    TH1D *hIntercept = new TH1D("hIntercept", "Intercept Distribution;Intercept [mV];Entries",
                                nBins_intercept, intercept_min, intercept_max);

    for (size_t i=0; i<slopes.size(); ++i) {
        if (slopes[i] >= slope_min && slopes[i] < slope_max) {
            hSlope->Fill(slopes[i]);
        }
        if (intercepts[i] >= intercept_min && intercepts[i] < intercept_max) {
            hIntercept->Fill(intercepts[i]);
        }
    }

    TCanvas *c1 = new TCanvas("c1", "TDC Amp Histograms", 1200, 600);
    c1->Divide(2,1);

    c1->cd(1);
    hSlope->GetXaxis()->SetRangeUser(slope_min, slope_max);
    hSlope->Draw();

    c1->cd(2);
    hIntercept->GetXaxis()->SetRangeUser(intercept_min, intercept_max);
    hIntercept->Draw();

    TString outdir = Form("../ROOT/%s/thresholdscan/fitting/tdcAmp/", SNo.Data());
    ensureDirectoryExists(outdir);
    TString pdfOut = outdir + Form("%s_tdcAmp_Hist.pdf", SNo.Data());
    c1->SaveAs(pdfOut);

    std::cout << "Histograms saved to: " << pdfOut << std::endl;
}

void usage() {
    std::cout << "Usage: ./draw_hist_tdcAmp <SNo>" << std::endl;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        usage();
        return 1;
    }

    TString SNo = argv[1];

    TApplication app("app", &argc, argv);
    draw_hist_tdcAmp(SNo);
    return 0;
}