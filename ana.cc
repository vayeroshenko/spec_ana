{
	Int_t nGraph = 4000;
	Int_t noiseLevel = 300;
	std::vector<TGraph> graphs(nGraph,TGraph());

	TGraph grMax = TGraph();

	std::ifstream file;
	file.open("1.txt");

	TH1D* spectrum = new TH1D("spectrum", "spectrum", 1000,0,2000);


	Bool_t isPeak = false;
	Int_t iGraph = 0;

	while (true) {
		if (iGraph == nGraph) break;
		Int_t value;
		file >> value;
		
		// if (iGraph > 221 && iGraph < 224) std::cout << value << "\n";
		
		if (isPeak) {
			if (value > noiseLevel){
				graphs[iGraph].SetPoint(graphs[iGraph].GetN(),graphs[iGraph].GetN(),value);
				continue;
			} else {
				isPeak = false;
				iGraph ++;
				continue;
			}
		}
		else if (value > noiseLevel){
			// std::cout << "wow\n";
			graphs[iGraph].SetPoint(graphs[iGraph].GetN(),graphs[iGraph].GetN(),value);
			isPeak = true;
			continue;
		}
		
	}




	// graphs[1].Draw("AP");


	TF1* func = new TF1("f1", "gaus", 7.5, 14.5);


	for (int i = 1; i < nGraph; ++i){
		graphs[i].SetMarkerStyle(kFullSquare);
		
		graphs[i].SetMarkerColor(i);



		func->SetParameters(1600., 10., 6);
		graphs[i].Fit(func,"", "", 7.5, 14.5);

		Int_t Xmax = 0;
		Int_t Ymax = 0;
		for (Int_t j = 0; j < graphs[i].GetN(); ++j){
			Double_t x, y;
			graphs[i].GetPoint(j, x, y);
			if (y > Ymax){
				Ymax = y;
				Xmax = x;
			}
		}

		Ymax = func->GetParameter(0);

		grMax.SetPoint(grMax.GetN(), Xmax, Ymax);



		// if(Xmax > 20){
			// graphs[i].Draw("SAME P");
		// 	std::cout << i << "\n";
		// }
		
		spectrum->Fill( Ymax );
	}	

	grMax.SetMarkerStyle(kFullSquare);
	grMax.SetMarkerColor(kRed);

	Int_t k = 1;
	// Int_t l = 1000;

	graphs[k].Draw("AP");
	// graphs[l].SetMarkerColor(kRed);

	// graphs[l].Draw("SAME P");

	// grMax.Draw("SAME P");
	spectrum->Draw();



}



// fit 7 - 14