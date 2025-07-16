#include <TFitResult.h>
#include <TFile.h>
#include <TTree.h>
#include <TGraph.h>
#include <TCanvas.h>
#include <TF1.h>
#include <TString.h>
#include <TApplication.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <TPaveText.h>
#include <sys/stat.h>
#include <unistd.h>
#include <TAxis.h>
#include <TSystemDirectory.h>
#include <TList.h>
#include <TSystemFile.h>
#include <TPaveStats.h>
#include <fstream>
#include <sstream>

int mapChannel(int absCh) {
    if (absCh == 0) return 24;
    if (absCh == 24) return 0;
    return absCh;
}

void ensureDirectoryExists(const TString &dirPath) {
    struct stat st;
    std::string dirStr(dirPath.Data());
    if (stat(dirStr.c_str(), &st) != 0) {
        mkdir(dirStr.c_str(), 0777);
    }
}

void writeOrUpdateFitParam(const TString& filepath, const TString& FGvalue,
                           double A, double Aerr,
                           double Mu, double Muerr,
                           double Sigma, double Sigmaerr)
{
    std::vector<std::string> lines;
    bool header_exists = false;
    bool found = false;

    std::ifstream infile(filepath.Data());
    if (infile.is_open()) {
        std::string line;
        while (getline(infile, line)) {
            if (line.find("#") == 0) {
                header_exists = true;
                lines.push_back(line);
                continue;
            }

            std::stringstream ss(line);
            std::string fg;
            ss >> fg;

            std::string fgTarget = "FG" + std::string(FGvalue.Data()) + "mV";
            if (fg == fgTarget) {
                std::ostringstream newline;
                newline << fgTarget << "\t"
                        << A << "\t" << Aerr << "\t"
                        << Mu << "\t" << Muerr << "\t"
                        << Sigma << "\t" << Sigmaerr;
                lines.push_back(newline.str());
                found = true;
            } else {
                lines.push_back(line);
            }
        }
        infile.close();
    }

    if (!header_exists) {
        lines.insert(lines.begin(),
            "# FG[mV]\tA\tAerr\tMu\tMuerr\tSigma\tSigmaerr");
    }

    if (!found) {
        std::string fgTarget = "FG" + std::string(FGvalue.Data()) + "mV";
        std::ostringstream newline;
        newline << fgTarget << "\t"
                << A << "\t" << Aerr << "\t"
                << Mu << "\t" << Muerr << "\t"
                << Sigma << "\t" << Sigmaerr;
        lines.push_back(newline.str());
    }

    std::ofstream outfile(filepath.Data(), std::ios::trunc);
    for (const auto& l : lines) {
        outfile << l << std::endl;
    }
    outfile.close();
}

void threshold_scan_graph(const TString& SNo, const TString& FGvalue, int StartCH,
                          double fitMin, double fitMax) {
    int lastCH = StartCH + 7;

    TString dirPath = Form("../ROOT/%s/thresholdscan/", SNo.Data());

    TSystemDirectory dir("dir", dirPath);
    TList *files = dir.GetListOfFiles();
    if (!files) {
        std::cerr << "ディレクトリが存在しないか空．" << std::endl;
        return;
    }

    std::vector<int> dacVals;
    std::vector<std::vector<double>> hitRatios(8);

    TIter next(files);
    TSystemFile *file;
    while ((file = (TSystemFile*)next())) {
        TString fname = file->GetName();
        if (!fname.EndsWith(".root")) continue;

        if (!fname.Contains(Form("FG%smV", FGvalue.Data()))) continue;

        if (!fname.BeginsWith(Form("%d_", StartCH))) continue;

        int idx_dac = fname.Index("DAC");
        if (idx_dac < 0) continue;
        TString dacStr = fname;
        dacStr.Remove(0, idx_dac + 3);
        dacStr.ReplaceAll("mV.root", "");
        int dac = dacStr.Atoi();

        TString fullPath = dirPath + fname;
        TFile *f = TFile::Open(fullPath, "READ");
        if (!f || f->IsZombie()) {
            continue;
        }
        TTree *tree = (TTree*)f->Get("tree");
        if (!tree) {
            f->Close();
            continue;
        }

        int tdcNhit[48];
        tree->SetBranchAddress("tdcNhit", tdcNhit);
        Long64_t nentries = tree->GetEntries();

        std::vector<double> counts(8, 0.0);

        for (Long64_t i=0; i<nentries; ++i) {
            tree->GetEntry(i);
            for (int ch=0; ch<8; ++ch) {
                int absCh = StartCH + ch;
                int mappedCh = mapChannel(absCh);
                if (mappedCh > 47) continue;
                if (tdcNhit[mappedCh] > 0) {
                    counts[ch] += 1.0;
                }
            }
        }
        f->Close();

        dacVals.push_back(dac);
        for (int ch=0; ch<8; ++ch) {
            hitRatios[ch].push_back(counts[ch] / nentries);
        }
    }

    if (dacVals.empty()) {
        std::cerr << "該当するROOTファイルが存在しない．" << std::endl;
        return;
    }

    // ソート
    std::vector<int> index(dacVals.size());
    for (int i=0; i<dacVals.size(); ++i) index[i] = i;
    std::sort(index.begin(), index.end(), [&](int a, int b) {
        return dacVals[a] < dacVals[b];
    });

    std::vector<double> dac_sorted(dacVals.size());
    std::vector<std::vector<double>> hit_sorted(8);
    for (int ch=0; ch<8; ++ch) hit_sorted[ch].resize(dacVals.size());

    for (int i=0; i<dacVals.size(); ++i) {
        dac_sorted[i] = dacVals[index[i]];
        for (int ch=0; ch<8; ++ch) {
            hit_sorted[ch][i] = hitRatios[ch][index[i]];
        }
    }

    TCanvas *c1 = new TCanvas("c1", "Threshold Scan", 1200, 800);
    c1->Divide(4,2);

    TString fittingDir = Form("../ROOT/%s/thresholdscan/fitting/", SNo.Data());
    ensureDirectoryExists(fittingDir);

    for (int ch=0; ch<8; ++ch) {
        int absCh = StartCH + ch;
        int mappedCh = mapChannel(absCh);

        std::vector<double> x;
        std::vector<double> y;
        for (size_t i = 0; i < dac_sorted.size(); ++i) {
            if (dac_sorted[i] >= fitMin && dac_sorted[i] <= fitMax) {
                x.push_back(dac_sorted[i]);
                y.push_back(hit_sorted[ch][i]);
            }
        }

        if (x.empty()) {
            std::cerr << "CH" << mappedCh << ": データ点が指定範囲内に存在しない．" << std::endl;
            continue;
        }

        c1->cd(ch+1);
        TGraph *gr = new TGraph(x.size(), &x[0], &y[0]);
        gr->SetTitle(Form("CH%d;DAC [mV];Hit Ratio", mappedCh));
        gr->SetMarkerStyle(20);
        gr->SetMarkerColor(kBlue);
        gr->Draw("AP");

        TF1 *f_erf = new TF1("f_erf",
            "[0]*0.5*(1+TMath::Erf((x-[1])/(sqrt(2)*[2])))",
            fitMin, fitMax);
        f_erf->SetParameters(1.0, (fitMin+fitMax)/2, 10.0);

        TFitResultPtr fitResult = gr->Fit(f_erf, "S");

        TPaveStats *stats = (TPaveStats*)gr->GetListOfFunctions()->FindObject("stats");
        if (stats) {
            stats->SetX1NDC(0.55);
            stats->SetX2NDC(0.98);
            stats->SetY1NDC(0.5);
            stats->SetY2NDC(0.85);
        }

        TPaveText *pavetext = new TPaveText(0.15, 0.7, 0.5, 0.85, "NDC");
        pavetext->AddText(Form("CH%d", mappedCh));
        pavetext->AddText(Form("A = %.3f ± %.3f",
                               f_erf->GetParameter(0), f_erf->GetParError(0)));
        pavetext->AddText(Form("Mu = %.3f ± %.3f",
                               f_erf->GetParameter(1), f_erf->GetParError(1)));
        pavetext->AddText(Form("Sigma = %.3f ± %.3f",
                               f_erf->GetParameter(2), f_erf->GetParError(2)));
        pavetext->SetFillColor(0);
        pavetext->SetBorderSize(1);
        pavetext->SetTextFont(42);
        pavetext->Draw("same");

        TString paramFile = fittingDir + Form("%d_fitparam.txt", mappedCh);

        writeOrUpdateFitParam(paramFile, FGvalue,
            f_erf->GetParameter(0), f_erf->GetParError(0),
            f_erf->GetParameter(1), f_erf->GetParError(1),
            f_erf->GetParameter(2), f_erf->GetParError(2));
    }

    TString outDir = Form("../ROOT/%s/thresholdscan/fitting/", SNo.Data());
    ensureDirectoryExists(outDir);
    TString outname = outDir + Form("%d_%d_FG%smV_thresholdscan_fit.pdf", StartCH, lastCH, FGvalue.Data());
    c1->SaveAs(outname);
}

void usage() {
    std::cout << "Usage: ./threshold_scan_graph <SNo> <FGvalue> <StartCH> <FitMin> <FitMax>" << std::endl;
}

int main(int argc, char** argv) {
    if (argc != 6) {
        usage();
        return 1;
    }

    TString SNo = argv[1];
    TString FGvalue = argv[2];
    int StartCH = TString(argv[3]).Atoi();
    double fitMin = std::stod(argv[4]);
    double fitMax = std::stod(argv[5]);

    TApplication app("app", &argc, argv);
    threshold_scan_graph(SNo, FGvalue, StartCH, fitMin, fitMax);
    return 0;
}