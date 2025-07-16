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

void threshold_scan_graph(const TString& SNo, const TString& FGvalue, int StartCH) {
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

        // ファイル名例：
        // 0_7_FG10mV_DAC3450mV.root
        // 8_15_FG10mV_DAC3450mV.root

        // FG電圧一致チェック
        if (!fname.Contains(Form("FG%smV", FGvalue.Data()))) continue;

        // StartCH一致チェック
        if (!fname.BeginsWith(Form("%d_", StartCH))) continue;

        // DAC値を抽出
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

    for (int ch=0; ch<8; ++ch) {
        int absCh = StartCH + ch;
        int mappedCh = mapChannel(absCh);

        std::vector<double> x(dac_sorted.begin(), dac_sorted.end());
        std::vector<double> y(hit_sorted[ch].begin(), hit_sorted[ch].end());

        c1->cd(ch+1);
        TGraph *gr = new TGraph(x.size(), &x[0], &y[0]);
        gr->SetTitle(Form("CH%d;DAC [mV];Hit Ratio", mappedCh));
        gr->SetMarkerStyle(20);
        gr->SetMarkerColor(kBlue);
        gr->Draw("AP");

        // Fit はコメントアウトしたまま
    }

    TString outDir = Form("../ROOT/%s/thresholdscan/fig/", SNo.Data());
    ensureDirectoryExists(outDir);
    TString outname = outDir + Form("%d_%d_FG%smV_thresholdscan.pdf", StartCH, lastCH, FGvalue.Data());
    c1->SaveAs(outname);
}

void usage() {
    std::cout << "Usage: ./threshold_scan_graph <SNo> <FGvalue> <StartCH>" << std::endl;
}

int main(int argc, char** argv) {
    if (argc != 4) {
        usage();
        return 1;
    }

    TString SNo = argv[1];
    TString FGvalue = argv[2];
    int StartCH = TString(argv[3]).Atoi();

    TApplication app("app", &argc, argv);
    threshold_scan_graph(SNo, FGvalue, StartCH);
    return 0;
}