#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <TString.h>
#include <TGraph.h>
#include <TF1.h>
#include <TCanvas.h>
#include <TApplication.h>
#include <TPaveStats.h>
#include <TStyle.h>
#include <sys/stat.h>
#include <unistd.h>

void ensureDirectoryExists(const TString &dirPath) {
    struct stat st;
    std::string dirStr(dirPath.Data());
    if (stat(dirStr.c_str(), &st) != 0) {
        mkdir(dirStr.c_str(), 0777);
    }
}

void analyze_mu_vs_fg(const TString& SNo, int Ch) {
    TString filepath = Form("../ROOT/%s/thresholdscan/fitting/%d_fitparam.txt", SNo.Data(), Ch);

    std::ifstream infile(filepath.Data());
    if (!infile.is_open()) {
        std::cerr << "Cannot open file: " << filepath << std::endl;
        return;
    }

    std::vector<double> FGvalues;
    std::vector<double> MuValues;

    std::string line;
    while (std::getline(infile, line)) {
        if (line.empty()) continue;
        if (line[0] == '#') continue;

        std::stringstream ss(line);
        std::string FGstr;
        double A, Aerr, Mu, Muerr, Sigma, Sigmaerr;
        ss >> FGstr >> A >> Aerr >> Mu >> Muerr >> Sigma >> Sigmaerr;

        if (FGstr.substr(0, 2) == "FG") {
            std::string fgValStr = FGstr.substr(2, FGstr.find("mV") - 2);
            double fgVal = std::stod(fgValStr);
            FGvalues.push_back(fgVal);
            MuValues.push_back(Mu);
        }
    }
    infile.close();

    if (FGvalues.empty()) {
        std::cerr << "No data found in file: " << filepath << std::endl;
        return;
    }

    gStyle->SetOptFit(1);

    TCanvas *c1 = new TCanvas("c1", "Mu vs FG", 800, 600);
    TGraph *gr = new TGraph(FGvalues.size(), &FGvalues[0], &MuValues[0]);
    gr->SetTitle(Form("SNo %s CH%d Mu vs FG;FG [mV];Mu [mV]", SNo.Data(), Ch));
    gr->SetMarkerStyle(20);
    gr->SetMarkerColor(kBlue);
    gr->Draw("AP");

    TF1 *f1 = new TF1("f1", "[0] + [1]*x", 0, 100);
    f1->SetParameters(0, 1);
    gr->Fit(f1, "S");

    c1->Update();
    TPaveStats *stats = (TPaveStats*)gr->GetListOfFunctions()->FindObject("stats");
    if (stats) {
        stats->SetX1NDC(0.55);
        stats->SetX2NDC(0.98);
        stats->SetY1NDC(0.6);
        stats->SetY2NDC(0.85);
        stats->Draw("same");
    }

    TString tdcAmpDir = Form("../ROOT/%s/thresholdscan/fitting/tdcAmp/", SNo.Data());
    ensureDirectoryExists(tdcAmpDir);

    TString pdfOut = tdcAmpDir + Form("%s_CH%d_tdc_Amplifer_fit.pdf", SNo.Data(), Ch);
    c1->SaveAs(pdfOut);

    TString fitParamFile = tdcAmpDir + "tdcAmp_fitparam.txt";

    std::vector<std::string> lines;
    std::ifstream fin(fitParamFile.Data());
    bool headerExists = false;
    if (fin.is_open()) {
        std::string l;
        while (getline(fin, l)) {
            if (l.find("#") == 0) headerExists = true;
            lines.push_back(l);
        }
        fin.close();
    }

    std::ostringstream newline;
    newline << "SNo" << SNo.Data()
            << "\tCH" << Ch << "\t"
            << f1->GetParameter(0) << "\t" << f1->GetParError(0) << "\t"
            << f1->GetParameter(1) << "\t" << f1->GetParError(1);

    bool found = false;
    for (size_t i=0; i<lines.size(); ++i) {
        if (lines[i].find("SNo" + std::string(SNo.Data()) + "\tCH" + std::to_string(Ch)) == 0) {
            lines[i] = newline.str();
            found = true;
            break;
        }
    }
    if (!found) {
        if (!headerExists) {
            lines.insert(lines.begin(),
                "# SNo\tCH\tIntercept\tInterceptErr\tSlope\tSlopeErr");
        }
        lines.push_back(newline.str());
    }

    std::ofstream fout(fitParamFile.Data(), std::ios::trunc);
    for (const auto& l : lines) {
        fout << l << std::endl;
    }
    fout.close();

    std::cout << "Done. Plot saved to: " << pdfOut << std::endl;
}

void usage() {
    std::cout << "Usage: ./analyze_mu_vs_fg <SNo> <Ch>" << std::endl;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        usage();
        return 1;
    }

    TString SNo = argv[1];
    int Ch = std::stoi(argv[2]);

    TApplication app("app", &argc, argv);
    analyze_mu_vs_fg(SNo, Ch);
    return 0;
}