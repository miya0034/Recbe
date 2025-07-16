#include <TFile.h>
#include <TTree.h>
#include <TGraphErrors.h>
#include <TCanvas.h>
#include <TF1.h>
#include <TString.h>
#include <TSystemDirectory.h>
#include <TList.h>
#include <TSystemFile.h>
#include <TApplication.h>
#include <TMath.h>
#include <TPaveStats.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <sys/stat.h>  // mkdir用
#include <TAxis.h>
#include <TH1.h>
#include <TFitResult.h>
#include <TStyle.h>
void threshold_scan_graph(const TString& SNo, const TString& FGvalue, int StartCH) {
    TString dirPath = "../ROOT/" + SNo + "/thresholdscan/";
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
        if (fname.EndsWith(".root") && fname.Contains("FG" + FGvalue + "mV_")) {
            rootFiles.push_back(dirPath + fname);
        }
    }

    if (rootFiles.empty()) {
        std::cerr << "指定FG電圧 " << FGvalue << " mV のROOTファイルが存在しない．" << std::endl;
        return;
    }

    std::sort(rootFiles.begin(), rootFiles.end());

    const int nCH = 8;
    std::vector<double> dac_values;
    std::vector<double> hit_ratios[nCH];
    std::vector<double> hit_errors[nCH];

    Long64_t nentries_per_file = 0;

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
        Long64_t nentries = tree->GetEntries();

        nentries_per_file = nentries;

        double count[nCH] = {0.0};

        for (Long64_t i = 0; i < nentries; ++i) {
            tree->GetEntry(i);
            for (int ch=0; ch<nCH; ++ch) {
                int absCH = StartCH + ch;
                if (absCH > 47) continue;
                if (tdcNhit[absCH] > 0) count[ch] += 1.0;
            }
        }

        TString dacStr = fname;
        dacStr.ReplaceAll(dirPath,"");
        dacStr.ReplaceAll("mV.root","");
        int idx = dacStr.Index("DAC");
        if (idx < 0) {
            std::cerr << "DAC値の抽出失敗: " << fname << std::endl;
            f->Close();
            continue;
        }
        dacStr.Remove(0, idx+3);
        double dac_value = dacStr.Atof();
        dac_values.push_back(dac_value);

        for (int ch=0; ch<nCH; ++ch) {
            double hit_ratio = count[ch] / static_cast<double>(nentries);
            hit_ratios[ch].push_back(hit_ratio);
            double err = 0.0;
            if (nentries > 0) {
                err = sqrt(hit_ratio * (1.0 - hit_ratio) / nentries);
            }
            hit_errors[ch].push_back(err);
        }

        f->Close();
    }

    int N = dac_values.size();
    if (N == 0) {
        std::cerr << "有効なデータが存在しない．" << std::endl;
        return;
    }

    std::vector<int> index(N);
    for (int i=0; i<N; ++i) index[i] = i;
    std::sort(index.begin(), index.end(), [&](int a, int b) {
        return dac_values[a] < dac_values[b];
    });

    std::vector<double> dac_sorted(N);
    std::vector<double> hit_sorted[nCH];
    std::vector<double> err_sorted[nCH];
    for (int ch=0; ch<nCH; ++ch) {
        hit_sorted[ch].resize(N);
        err_sorted[ch].resize(N);
    }
    for (int i=0; i<N; ++i) {
        dac_sorted[i] = dac_values[index[i]];
        for (int ch=0; ch<nCH; ++ch) {
            hit_sorted[ch][i] = hit_ratios[ch][index[i]];
            err_sorted[ch][i] = hit_errors[ch][index[i]];
        }
    }

    gStyle->SetOptFit(1111);

    TCanvas *c1 = new TCanvas("c1","Threshold Scan",1200,800);
    c1->Divide(4,2);

    for (int ch=0; ch<nCH; ++ch) {
        c1->cd(ch+1);

        std::vector<double> ex(N, 0.0);

        TGraphErrors *gr = new TGraphErrors(N,
                                            &dac_sorted[0],
                                            &hit_sorted[ch][0],
                                            &ex[0],
                                            &err_sorted[ch][0]);

        TString title = TString::Format("CH%d;DAC [mV];Hit Ratio", StartCH+ch);
        gr->SetTitle(title);
        gr->SetMarkerStyle(20);
        gr->SetMarkerColor(kBlue);
        gr->Draw("AP");

        double medianDAC = dac_sorted[N/2];
        TF1 *fitFunc = new TF1("fitFunc",
            "[0]*0.5*(1+TMath::Erf((x-[1])/(sqrt(2)*[2])))",
            dac_sorted.front(), dac_sorted.back());
        fitFunc->SetParameters(1.0, medianDAC, 10.0);

        auto fitResult = gr->Fit(fitFunc, "RS");

        fitFunc->SetLineColor(kRed);
        fitFunc->Draw("same");

        double mu = fitFunc->GetParameter(1);
        double xmin = mu - 100.0;
        double xmax = mu + 100.0;

        gr->GetXaxis()->SetLimits(xmin, xmax);
        gr->GetHistogram()->GetXaxis()->SetRangeUser(xmin, xmax);

        c1->Update();

        // 統計ボックスを右下に移動
        TPaveStats *st = (TPaveStats*)gr->GetListOfFunctions()->FindObject("stats");
        if (st) {
            st->SetX1NDC(0.7);
            st->SetX2NDC(0.9);
            st->SetY1NDC(0.1);
            st->SetY2NDC(0.3);
            st->Draw();
        }

        double A = fitFunc->GetParameter(0);
        double Mu = fitFunc->GetParameter(1);
        double Sigma = fitFunc->GetParameter(2);
        double chi2 = fitResult->Chi2();
        int ndf = fitResult->Ndf();
        double prob = fitResult->Prob();

        std::cout << "CH" << StartCH+ch << ": "
                  << "A = " << A
                  << ", Mu = " << Mu
                  << ", Sigma = " << Sigma
                  << ", Chi2 = " << chi2
                  << ", NDF = " << ndf
                  << ", Prob = " << prob
                  << std::endl;
    }

    TString outDir = dirPath + "fig/";
    struct stat st;
    if (stat(outDir.Data(), &st) != 0) {
        int ret = mkdir(outDir.Data(), 0777);
        if (ret != 0) {
            std::cerr << "出力ディレクトリ作成失敗: " << outDir << std::endl;
            return;
        }
    }

    TString outname = TString::Format("%s%d_%d_FG%smV_thresholdscan.pdf",
                                      outDir.Data(), StartCH, StartCH+7, FGvalue.Data());
    c1->SaveAs(outname);
}

void usage() {
    std::cout << "Usage: ./threshold_scan_graph <SNo> <FGvalue> <StartCH>" << std::endl;
    std::cout << "  SNo: 測定ディレクトリ名 (例 20240626)" << std::endl;
    std::cout << "  FGvalue: FG電圧値 (例 20 → FG20mV_の20)" << std::endl;
    std::cout << "  StartCH: 開始チャンネル番号 (0〜40)" << std::endl;
}

int main(int argc, char** argv) {
    if (argc != 4) {
        usage();
        return 1;
    }

    TString SNo = argv[1];
    TString FGvalue = argv[2];
    int StartCH = TString(argv[3]).Atoi();

    if (StartCH < 0 || StartCH > 40) {
        std::cerr << "StartCHは0〜40の範囲で指定せよ．" << std::endl;
        return 1;
    }

    TApplication app("app", &argc, argv);
    threshold_scan_graph(SNo, FGvalue, StartCH);
    return 0;
}