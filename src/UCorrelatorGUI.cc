#include "UCorrelatorGUI.h" 
#include "TCanvas.h" 
#include "AnitaEventSummary.h" 
#include "FFTtools.h" 
#include "AnalysisWaveform.h" 
#include "WaveformCombiner.h" 
#include "TVirtualHistPainter.h"
#include "FilteredAnitaEvent.h" 


UCorrelator::gui::Map::Map(const TH2D & hist, const FilteredAnitaEvent * ev, WaveformCombiner * c, WaveformCombiner *cf, AnitaPol::AnitaPol_t pol, const AnitaEventSummary* sum) 
  : TH2D(hist), wfpad(0), f(ev), c(c), cf(cf), clicked(0), use_filtered(false),pol(pol)  
{
  SetStats(0);  
  SetDirectory(0); 

  /** figure out sun position */ 


  double phi_sun = sum->sun.phi;
  double theta_sun = -sum->sun.theta; 
  phi_sun = FFTtools::wrap(phi_sun,360); 
  specials.push_back(TMarker(phi_sun,theta_sun,4)); 
  specials.push_back(TMarker(phi_sun,theta_sun,7)); 


  if (sum->mc.phi >-999)
  {
    TMarker mc(sum->mc.phi, -sum->mc.theta,29); 
    mc.SetMarkerColor(2); 
    specials.push_back(mc); 
  }
  else
  {
    /** figure out if WAIS is (roughly) above horizon*/ 
    if (sum->wais.theta < 8)
    {
      TMarker wais(sum->wais.phi, -sum->wais.theta,29); 
      wais.SetMarkerColor(2); 
      specials.push_back(wais); 
    }

    /** figure out if LDB is (roughly) above horizon */
    if (sum->ldb.theta < 8)
    {
      TMarker ldb(sum->ldb.phi, -sum->ldb.theta,29); 
      ldb.SetMarkerColor(2); 
      specials.push_back(ldb); 
    }
  }

  coherent = 0; 
  deconvolved = 0; 
}


void UCorrelator::gui::Map::addRough(const std::vector<std::pair<double,double> > & rough)
{
  for (unsigned i = 0; i < rough.size(); i++)
  {
    addRough( rough[i].first, rough[i].second); 
  }
}

void UCorrelator::gui::Map::addRough(double x, double y) 
{
  TMarker m(x, y,3); 
  m.SetMarkerStyle(2); 
  rough_m.push_back(m); 
}

void UCorrelator::gui::Map::addFine(const AnitaEventSummary::PointingHypothesis & p) 
{
  TMarker m(p.phi, -p.theta,2); 
  m.SetMarkerStyle(2); 
  m.SetMarkerColor(3); 
  fine_m.push_back(m); 

  double angle = 90. / TMath::Pi()* atan2(2*p.rho * p.sigma_theta * p.sigma_phi, p.sigma_phi * p.sigma_phi - p.sigma_theta * p.sigma_theta);
  TEllipse ell(p.phi, -p.theta, p.sigma_phi, p.sigma_theta, 0, 360, angle); 
  ell.SetFillStyle(0); 
  ell.SetLineColor(3); 
  fine_e.push_back(ell); 
}

UCorrelator::gui::Map::~Map()
{
  if (wfpad) delete wfpad; 
  if (clicked) delete clicked; 
  if (coherent) delete coherent; 
  if (deconvolved) delete deconvolved; 
}


void UCorrelator::gui::Map::Paint(Option_t * opt) 
{
  //we ignore most of the options 
  GetPainter(opt);
  fPainter->Paint(opt); 

  /* turn off peaks with np*/ 
  if (!strcasestr(opt,"np"))
  {
    for (size_t i = 0; i < rough_m.size(); i++) 
    {
      rough_m[i].SetMarkerSize(1+rough_m.size()-i); 
      rough_m[i].Draw(); 
    }

    for (size_t i = 0; i < fine_m.size(); i++) 
    {
      fine_m[i].Draw(); 
    }
  }

  /* turn off peaks with ne*/ 
  if (!strcasestr(opt,"ne"))
  {
    for (size_t i = 0; i < fine_e.size(); i++) 
    {
      fine_e[i].Draw(); 
    }
  }

  /** turn off specials with ns */ 

  if (!strcasestr(opt,"ns"))
  {
    for (size_t i = 0; i < specials.size(); i++) 
    {
      specials[i].Draw(); 
    }
  }

  if (clicked) clicked->Draw(); 
}

void UCorrelator::gui::Map::SetUseUnfiltered()
{
  use_filtered = false; 
  if (wfpad) drawWf(last_phi, last_theta); 
}

void UCorrelator::gui::Map::SetUseFiltered()
{
  use_filtered = true; 
  if (wfpad) drawWf(last_phi, last_theta); 

}

void UCorrelator::gui::Map::clear()
{
  rough_m.clear(); 
  fine_m.clear(); 
  fine_e.clear(); 
  specials.clear(); 
}

void UCorrelator::gui::Map::closeCanvas()
{
  TMarker * deleteme = clicked; 
  clicked = 0;
  wfpad = 0; 
  delete deleteme; 
}

void UCorrelator::gui::Map::ExecuteEvent(int event, int px, int  py)
{
  if (event != kButton1Down && fPainter)
  {
//    fPainter->ExecuteEvent(event,px,py); //??? 
    return;
  }

  Double_t x  = gPad->PadtoX(gPad->AbsPixeltoX(px));
  Double_t y  = gPad->PadtoY(gPad->AbsPixeltoY(py));

  TVirtualPad * ours = gPad; 
  drawWf(x,-y); 

  if (clicked) delete clicked; 
  clicked = new TMarker(x,y,5); 
  ours->Modified(); 
  ours->Update(); 

}

void UCorrelator::gui::Map::drawWf(double phi, double theta) 
{
  last_theta=theta;
  last_phi = phi; 
  if (!wfpad || !wfpad->GetCanvasImp()) 
  {
    if (!wfpad) delete wfpad; 
    wfpad = new TCanvas(TString::Format("%s_zoom",GetName()),TString::Format("%s (clicked)", GetTitle()), 800,800); 
    wfpad->Connect("Closed()", "UCorrelator::gui::Map",this,"closeCanvas()"); 
  }

  wfpad->Clear(); 
  WaveformCombiner * wf = use_filtered ? cf : c; 
  wf->combine(phi, theta, f, (AnitaPol::AnitaPol_t) pol, 0); 


  if (coherent) delete coherent; 
  if (deconvolved) delete deconvolved; 
  coherent = new AnalysisWaveform(*wf->getCoherent()); 
  deconvolved = wf->getDeconvolved() ? new AnalysisWaveform(*wf->getDeconvolved()) : 0; 

  coherent->updateEven()->SetTitle(TString::Format("Coherent (#phi=%g, #theta=%g) (%s)",phi,theta, use_filtered ? "filtered" :"unfiltered")); 

  wfpad->Divide(2,deconvolved?2:1); 
  wfpad->cd(1); 
  coherent->drawEven(); 
  wfpad->cd(2); 
  coherent->drawPowerdB(); 

  if (deconvolved)
  {
    deconvolved->updateEven()->SetTitle(TString::Format("Deconvolved (#phi=%g, #theta=%g) (%s)",phi,theta, use_filtered ? "filtered" :"unfiltered")); 
    wfpad->cd(3); 
    deconvolved->drawEven(); 
    wfpad->cd(4); 
    deconvolved->drawPowerdB(); 
  }


  wfpad->Update(); 
}
