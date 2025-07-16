#include <TFile.h>
#include <TTree.h>
#include <TCanvas.h>
#include <TGraphErrors.h>
#include <TLegend.h>
#include <TString.h>
#include <TPad.h>
#include <TF1.h>
#include <TStyle.h>
#include <TPaveStats.h>
#include <TH1F.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <dirent.h>
#include <TAxis.h>
#include <fstream>
#include <filesystem>

void chargeVSadc(TString dirName, int chStart)
{
    TString outputDir = "../ROOT/" + dirName + "/chargeVSadc";
    const int nCh = 8;
    const int nSample = 32;

    std::vector<double> chargeVec;
    std::vector<double> maxAdcVec[nCh];

    double zeroAdcSum[nCh] = {0};
    int zeroAdcCount = 0;

    DIR *dp;
    struct dirent *entry;

    TString fullDirName = "../ROOT/" + dirName;
    if ((dp = opendir(fullDirName.Data())) == nullptr) {
        std::cerr << "Cannot open directory: " << fullDirName << std::endl;
        return;
    }

    while ((entry = readdir(dp)) != nullptr) {
        TString fileName = entry->d_name;
        if (!fileName.EndsWith(".root")) continue;

        Ssiz_t pos0 = fileName.Index("_");
        if (pos0 == kNPOS) continue;

        TString chStartStr = fileName(0, pos0);
        int fileChStart = chStartStr.Atoi();
        if (fileChStart != chStart) continue;

        Ssiz_t pos2 = fileName.Index("mV");
        if (pos2 == kNPOS) continue;

        std::string fileNameStr(fileName.Data());
        size_t pos1 = fileNameStr.rfind('_', pos2);
        if (pos1 == std::string::npos) continue;

        TString chargeStr = fileName(pos1+1, pos2-pos1-1);
        double charge = chargeStr.Atof();

        TString fullPath = fullDirName + "/" + fileName;
        TFile *file = TFile::Open(fullPath, "READ");
        if (!file || file->IsZombie()) {
            std::cerr << "Cannot open file: " << fullPath << std::endl;
            continue;
        }

        TTree *tree = (TTree*)file->Get("tree");
        if (!tree) {
            std::cerr << "No tree in file: " << fullPath << std::endl;
            file->Close();
            continue;
        }

        Int_t adc[48][32];
        tree->SetBranchAddress("adc", adc);
        Long64_t nEntries = tree->GetEntries();
        double maxAdc[nCh] = {0};

        for (Long64_t i = 0; i < nEntries; ++i) {
            tree->GetEntry(i);
            for (int ch = 0; ch < nCh; ++ch) {
                int absCh = chStart + ch;
                int mappedCh = (absCh == 0) ? 24 : (absCh == 24) ? 0 : absCh;
                for (int j = 0; j < nSample; ++j) {
                    if (adc[mappedCh][j] > maxAdc[ch]) {
                        maxAdc[ch] = adc[mappedCh][j];
                    }
                }
            }
        }

        if (charge == 0) {
            for (Long64_t i = 0; i < nEntries; ++i) {
                tree->GetEntry(i);
                for (int ch = 0; ch < nCh; ++ch) {
                    int absCh = chStart + ch;
                    int mappedCh = (absCh == 0) ? 24 : (absCh == 24) ? 0 : absCh;
                    for (int j = 0; j < nSample; ++j) {
                        zeroAdcSum[ch] += adc[mappedCh][j];
                    }
                }
            }
            zeroAdcCount += nEntries * nSample;
        } else {
            chargeVec.push_back(charge);
            for (int ch = 0; ch < nCh; ++ch)
                maxAdcVec[ch].push_back(maxAdc[ch]);
        }
        file->Close();
    }
    closedir(dp);

    double zeroAdcMean[nCh] = {0};
    if (zeroAdcCount > 0) {
        for (int ch = 0; ch < nCh; ++ch)
            zeroAdcMean[ch] = zeroAdcSum[ch] / zeroAdcCount;
    }

    for (int ch = 0; ch < nCh; ++ch) {
        for (size_t i = 0; i < maxAdcVec[ch].size(); ++i)
            maxAdcVec[ch][i] = (maxAdcVec[ch][i] - zeroAdcMean[ch]) * 1.95;
    }

    TCanvas *c1 = new TCanvas("c1", "Charge vs ADC", 1800, 1600);
    c1->Divide(2,4);

    gStyle->SetOptFit(1111);

    TString outTxtName = "../ROOT/" + dirName + "/chargeVSadc/fitting/chargeVSadc.txt";

    try {
        std::filesystem::path txtFilePath(outTxtName.Data());
        std::filesystem::create_directories(txtFilePath.parent_path());
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error creating directory: " << e.what() << std::endl;
        return;
    }

    std::ofstream ofs(outTxtName.Data());
    if (!ofs) {
        std::cerr << "Cannot open file: " << outTxtName << std::endl;
        return;
    }
    ofs << "# ch slope slope_err intercept intercept_err chi2/ndf ndf chi2\n";

    for (int ch = 0; ch < nCh; ++ch) {
        c1->cd(ch + 1);
        int absCh = chStart + ch;
        int mappedCh = (absCh == 0) ? 24 : (absCh == 24) ? 0 : absCh;

        double fitChi2NdfThreshold = 8.0;
        double maxCharge = *std::max_element(chargeVec.begin(), chargeVec.end());
        double bestCutCharge = 0;
        int nFitSteps = 20;

        for (int step = 1; step <= nFitSteps; ++step) {
            double cutCharge = maxCharge * step / nFitSteps;
            std::vector<double> xFit, yFit;
            for (size_t i = 0; i < chargeVec.size(); ++i) {
                if (chargeVec[i] <= cutCharge) {
                    xFit.push_back(chargeVec[i]);
                    yFit.push_back(maxAdcVec[ch][i]);
                }
            }
            if (xFit.size() < 2) break;

            TGraph grFit(xFit.size(), &xFit[0], &yFit[0]);
            TF1 fTest("fTest", "[0]*x+[1]");
            grFit.Fit(&fTest, "Q0");
            double chi2ndf = fTest.GetChisquare() / fTest.GetNDF();
            if (chi2ndf <= fitChi2NdfThreshold) {
                bestCutCharge = cutCharge;
            } else {
                break;
            }
        }

        std::vector<double> ex(chargeVec.size(), 0.0);
        std::vector<double> ey(chargeVec.size(), 0.0);
        TGraphErrors *gr = new TGraphErrors(chargeVec.size(), &chargeVec[0], &maxAdcVec[ch][0], &ex[0], &ey[0]);
        gr->SetTitle(Form("Charge vs ADC ch%d", mappedCh));
        gr->GetXaxis()->SetTitle("Charge [pC]");
        gr->GetYaxis()->SetTitle("Amplitude [mV]");
        gr->SetMarkerStyle(20);
        gr->SetMarkerSize(0.8);
        gr->SetMarkerColor(kBlue);
        gr->Draw("AP");

        TF1 *fLin = new TF1("fLin", "[0]*x+[1]", 0, bestCutCharge);
        gr->Fit(fLin, "Q", "", 0, bestCutCharge);
        gPad->Update();

        TPaveStats* st = (TPaveStats*)gr->GetListOfFunctions()->FindObject("stats");
        if (st) {
            st->SetX1NDC(0.65);
            st->SetX2NDC(0.95);
            st->SetY1NDC(0.15);
            st->SetY2NDC(0.35);
            gPad->Modified();
            gPad->Update();
        }

        double slope = fLin->GetParameter(0);
        double slope_err = fLin->GetParError(0);
        double intercept = fLin->GetParameter(1);
        double intercept_err = fLin->GetParError(1);
        double chi2 = fLin->GetChisquare();
        double ndf  = fLin->GetNDF();

        ofs << mappedCh << " " << slope << " " << slope_err << " "
            << intercept << " " << intercept_err << " "
            << chi2/ndf << " " << ndf << " " << chi2 << "\n";
    }

    c1->Update();

    TString outName = outputDir + "/" + dirName;
    outName.ReplaceAll("../ROOT/" + dirName + "/chargeVSadc", "");
    outName += Form("_%d.pdf", chStart);
    c1->SaveAs(outputDir + "/" + outName);

    ofs.close();
    std::cout << "Saved: " << outputDir << "/" << outName << std::endl;
}

int main(int argc, char** argv)
{
    if (argc != 3) {
        std::cerr << "Usage: root -l -q 'chargeVSadc.C(\"dirName\", chStart)'" << std::endl;
        return 1;
    }
    TString dirName = argv[1];
    int chStart = TString(argv[2]).Atoi();
    chargeVSadc(dirName, chStart);
    return 0;
}