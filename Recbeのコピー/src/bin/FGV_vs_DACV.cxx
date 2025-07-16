#include <TFile.h>
#include <TGraphErrors.h>
#include <TF1.h>
#include <TCanvas.h>
#include <TString.h>
#include <TLegend.h>
#include <TApplication.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <TAxis.h>
#include <TPaveStats.h>
#include <TFitResult.h>
#include <TStyle.h>
#include <sys/stat.h>
#include <unistd.h>

int mapChannel(int absCh) {
    if (absCh == 0) return 24;
    if (absCh == 24) return 0;
    return absCh;
}

void save_fit_params(const TString& SNo, int mappedCh,
                     double p0, double p0_err,
                     double p1, double p1_err,
                     double chi2, int ndf, double prob)
{
    TString outFile = Form("../ROOT/%s/thresholdscan/fitting/%s_FG_vs_Mu_fitparams.txt",
                           SNo.Data(), SNo.Data());

    // 読み込み
    std::vector<std::string> lines;
    bool header_found = false;
    bool replaced = false;

    std::ifstream ifs(outFile.Data());
    if (ifs.is_open()) {
        std::string line;
        while (getline(ifs, line)) {
            if (line.empty()) continue;
            if (line[0] == '#') {
                header_found = true;
                lines.push_back(line);
                continue;
            }
            std::istringstream iss(line);
            std::string sNo_str;
            int ch;
            iss >> sNo_str >> ch;
            if (sNo_str == SNo.Data() && ch == mappedCh) {
                std::ostringstream oss;
                oss << SNo << " " << mappedCh << " "
                    << p0 << " " << p0_err << " "
                    << p1 << " " << p1_err << " "
                    << chi2 << " " << ndf << " " << prob;
                lines.push_back(oss.str());
                replaced = true;
            } else {
                lines.push_back(line);
            }
        }
        ifs.close();
    }

    if (!header_found) {
        lines.insert(lines.begin(),
            "# SNo Ch p0 p0_err p1 p1_err Chi2 NDF Prob");
    }

    if (!replaced) {
        std::ostringstream oss;
        oss << SNo << " " << mappedCh << " "
            << p0 << " " << p0_err << " "
            << p1 << " " << p1_err << " "
            << chi2 << " " << ndf << " " << prob;
        lines.push_back(oss.str());
    }

    std::ofstream ofs(outFile.Data(), std::ios::trunc);
    if (!ofs) {
        std::cerr << "Cannot open file: " << outFile << std::endl;
        return;
    }
    for (const auto& l : lines) {
        ofs << l << std::endl;
    }
    ofs.close();

    std::cout << "Saved fit parameters to: " << outFile << std::endl;
}

void plot_FG_vs_Mu(const TString& SNo, int Ch) {
    int mappedCh = mapChannel(Ch);

    TString fitFileName = Form("../ROOT/%s/thresholdscan/fitting/%d_%s/%d_%s_fitparams.txt",
                               SNo.Data(), mappedCh, SNo.Data(), mappedCh, SNo.Data());

    std::ifstream ifs(fitFileName.Data());
    if (!ifs.is_open()) {
        std::cerr << "Cannot open file: " << fitFileName << std::endl;
        return;
    }

    std::vector<double> FG;
    std::vector<double> Mu;
    std::vector<double> eMu;

    std::string line;
    while (std::getline(ifs, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        double fg, a, mu, sigma, chi2;
        int ndf;
        double prob;
        iss >> fg >> a >> mu >> sigma >> chi2 >> ndf >> prob;
        FG.push_back(fg);
        Mu.push_back(mu);
        eMu.push_back(0.0);
    }
    ifs.close();

    int N = FG.size();
    if (N == 0) {
        std::cerr << "ファイルに有効なデータがない．" << std::endl;
        return;
    }

    gStyle->SetOptFit(1111);

    TCanvas *c = new TCanvas("c", "FG vs Mu", 800, 600);
    c->SetLeftMargin(0.18);

    TGraphErrors *gr = new TGraphErrors(N, &FG[0], &Mu[0], nullptr, &eMu[0]);

    gr->SetTitle(Form("CH%d;FG Voltage [mV];V^{(50%%)}_{#it{DAC}} [mV]", mappedCh));
    gr->GetYaxis()->SetTitleSize(0.045);
    gr->GetYaxis()->SetTitleOffset(1.5);

    gr->SetMarkerStyle(20);
    gr->SetMarkerSize(1.2);
    gr->SetMarkerColor(kBlue);
    gr->Draw("AP");

    TF1 *f1 = new TF1("f1", "[0]+[1]*x", FG.front(), FG.back());
    f1->SetLineColor(kRed);
    auto fitResult = gr->Fit(f1, "RS");

    f1->Draw("same");

    TPaveStats* st = (TPaveStats*)gr->GetListOfFunctions()->FindObject("stats");
    if (st) {
        st->SetX1NDC(0.65);
        st->SetX2NDC(0.95);
        st->SetY1NDC(0.15);
        st->SetY2NDC(0.35);
        gPad->Modified();
        gPad->Update();
    }

    double p0 = f1->GetParameter(0);
    double p0_err = f1->GetParError(0);
    double p1 = f1->GetParameter(1);
    double p1_err = f1->GetParError(1);
    double chi2 = fitResult->Chi2();
    int ndf = fitResult->Ndf();
    double prob = fitResult->Prob();

    std::cout << "Fit result for CH" << mappedCh << ":" << std::endl;
    std::cout << "Mu = " << p0 << " + " << p1 << " * FG" << std::endl;
    std::cout << "Chi2/NDF = " << chi2 << " / " << ndf << std::endl;
    std::cout << "Prob = " << prob << std::endl;

    save_fit_params(SNo, mappedCh, p0, p0_err, p1, p1_err, chi2, ndf, prob);

    TString outName = Form("../ROOT/%s/thresholdscan/fitting/%d_%s/%d_%s_FG_vs_Mu.pdf",
                           SNo.Data(), mappedCh, SNo.Data(), mappedCh, SNo.Data());
    c->SaveAs(outName);
    std::cout << "Saved PDF: " << outName << std::endl;
}

void usage() {
    std::cout << "Usage: ./plot_FG_vs_Mu <SNo> <Ch>" << std::endl;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        usage();
        return 1;
    }

    TString SNo = argv[1];
    int Ch = TString(argv[2]).Atoi();

    TApplication app("app", &argc, argv);
    plot_FG_vs_Mu(SNo, Ch);
    return 0;
}