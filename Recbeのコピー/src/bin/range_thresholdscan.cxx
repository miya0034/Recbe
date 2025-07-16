#include <TFile.h>
#include <TTree.h>
#include <TString.h>
#include <TSystemDirectory.h>
#include <TList.h>
#include <TSystemFile.h>
#include <TApplication.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <sys/stat.h>

int mapChannel(int absCh) {
    if (absCh == 0) return 24;
    if (absCh == 24) return 0;
    return absCh;
}

void threshold_scan_range(const TString& SNo, const TString& FGvalue, int StartCH, int nEvents) {
    TString dirPath = Form("../ROOT/%s/thresholdscan/", SNo.Data());
    TSystemDirectory dir("dir", dirPath);
    TList *files = dir.GetListOfFiles();
    if (!files) {
        std::cerr << "ディレクトリが存在しないか空．" << std::endl;
        return;
    }

    std::vector<TString> rootFiles;
    TIter next(files);
    TSystemFile *file;
    while ((file = (TSystemFile*)next())) {
        TString fname = file->GetName();
        if (fname.EndsWith(".root")
            && fname.Contains(Form("CH%d_", StartCH))
            && fname.Contains("FG" + FGvalue + "mV")) {
            rootFiles.push_back(dirPath + fname);
        }
    }

    if (rootFiles.empty()) {
        std::cerr << "指定FG電圧 " << FGvalue << " mV のROOTファイルが存在しない．" << std::endl;
        return;
    }

    std::sort(rootFiles.begin(), rootFiles.end());

    double maxDAC_0hit = -1e9;
    double minDAC_8000hit = 1e9;

    for (const auto& fname : rootFiles) {
        TFile *f = TFile::Open(fname);
        if (!f || f->IsZombie()) {
            std::cerr << "ファイルを開けない: " << fname << std::endl;
            continue;
        }

        TTree *tree = (TTree*)f->Get("tree");
        if (!tree) {
            std::cerr << "TTreeが存在しない: " << fname << std::endl;
            f->Close();
            continue;
        }

        int tdcNhit[48];
        tree->SetBranchAddress("tdcNhit", tdcNhit);

        int hitCount = 0;
        Long64_t nentries = tree->GetEntries();

        for (Long64_t i = 0; i < nentries && i < nEvents; ++i) {
            tree->GetEntry(i);
            for (int ch = 0; ch < 8; ++ch) {
                int absCH = StartCH + ch;
                int mappedCH = mapChannel(absCH);
                if (mappedCH > 47) continue;
                if (tdcNhit[mappedCH] > 0) hitCount++;
            }
        }

        double hit_ratio = static_cast<double>(hitCount) / (nEvents * 8);

        TString dacStr = fname;
        dacStr.ReplaceAll(dirPath, "");
        dacStr.ReplaceAll("mV.root", "");
        int idx = dacStr.Index("DAC");
        if (idx < 0) {
            std::cerr << "DAC値の抽出失敗: " << fname << std::endl;
            f->Close();
            continue;
        }
        dacStr.Remove(0, idx + 3);
        double dac_value = dacStr.Atof();

        std::cout << "DAC = " << dac_value << " mV, hit_ratio = " << hit_ratio << std::endl;

        if (hit_ratio < 0.05) {
            if (dac_value > maxDAC_0hit) maxDAC_0hit = dac_value;
        }
        if (hit_ratio > 0.80) {
            if (dac_value < minDAC_8000hit) minDAC_8000hit = dac_value;
        }

        f->Close();
    }

    std::cout << "===================" << std::endl;
    std::cout << "FG" << FGvalue << "mV, StartCH " << StartCH << "〜" << StartCH + 7 << std::endl;

    if (maxDAC_0hit > -1e9)
        std::cout << "tdcNhit ≈ 0 が確認された最大DAC値: " << maxDAC_0hit << " mV" << std::endl;
    else
        std::cout << "tdcNhit ≈ 0 が確認されなかった．" << std::endl;

    if (minDAC_8000hit < 1e9)
        std::cout << "tdcNhit ≈ 全ヒット が確認された最小DAC値: " << minDAC_8000hit << " mV" << std::endl;
    else
        std::cout << "tdcNhit ≈ 全ヒット が確認されなかった．" << std::endl;

    std::cout << "===================" << std::endl;
    if (maxDAC_0hit > -1e9 && minDAC_8000hit < 1e9) {
        std::cout << "min=" << maxDAC_0hit << " max=" << minDAC_8000hit << std::endl;
    } else {
        std::cout << "min=-1 max=-1" << std::endl;
    }
}

void usage() {
    std::cout << "Usage: ./threshold_scan_range <SNo> <FGvalue> <StartCH> <nEvents>" << std::endl;
}

int main(int argc, char** argv) {
    if (argc != 5) {
        usage();
        return 1;
    }

    TString SNo = argv[1];
    TString FGvalue = argv[2];
    int StartCH = TString(argv[3]).Atoi();
    int nEvents = TString(argv[4]).Atoi();

    TApplication app("app", &argc, argv);
    threshold_scan_range(SNo, FGvalue, StartCH, nEvents);
    return 0;
}