void checkWaveform(std::string f_name){
	std::ifstream f_in(f_name);
	std::string x,y;
	TGraph* g = new TGraph();
	while(f_in>>x>>y) g->AddPoint(std::stod(x), std::stod(y));
	g->GetXaxis()->SetTitle("Time [s]");
	g->GetYaxis()->SetTitle("Voltage [V]");
	g->SetMarkerColor(2);
	g->Draw("APL");
}
