#include "TH2.h" 
#include "UCImageTools.h" 
#include "TString.h" 
#include <algorithm>
#include <set>
#include <math.h>


double UCorrelator::getZRMS(const TH2* hist)
{
  int numBinsX = hist->GetNbinsX();
  int numBinsY = hist->GetNbinsY();

  double sum2 = 0;

  for (int binX=0; binX<numBinsX; binX++) {
    for (int binY=0; binY<numBinsY; binY++) {
      double binValue = double(hist->GetBinContent(binX,binY));
      sum2 += pow(binValue,2);
    }
  }

  return sqrt(sum2/(numBinsX*numBinsY));
}


static TH1* makeTH1(char type, const char * name, const char * title, int nbins, double xmin, double xmax) 
{

  switch(type)
  {
    case 'C': 
      return new TH1C(name,title,nbins,xmin,xmax); 
    case 'S': 
      return new TH1S(name,title,nbins,xmin,xmax); 
    case 'I': 
      return new TH1I(name,title,nbins,xmin,xmax); 
    case 'F': 
      return new TH1F(name,title,nbins,xmin,xmax); 
    case 'D': 
    default:
      return new TH1D(name,title,nbins,xmin,xmax); 
  }
}


static const TAxis * getAxis(const TH2* H, int axis) 
{
  switch(axis) 
  {

    case 1: 
      return H->GetXaxis(); 
    case 2: 
      return H->GetYaxis(); 
    default:
      fprintf(stderr,"UCorrelator::image: axis must be 1 (x) or 2 (y)\n"); 
      return 0; 
  }
}


TH1* UCorrelator::image::getPctileProjection(const TH2 * H, int axis, double pct, bool ignoreEmpty) 
{
  TString name; 
  TString title; 
  name.Form("%s_%g_proj_%d", H->GetName(), pct, axis); 
  title.Form("%s %g%% projection of axis %d", H->GetTitle(), 100*pct, axis); 
  
  TH1 * h = makeTH1(H->ClassName()[3], name.Data(), title.Data(), getAxis(H,axis)->GetNbins(), getAxis(H,axis)->GetXmin(), getAxis(H,axis)->GetXmax()); 

  int nOrth = getAxis(H,3-axis)->GetNbins(); 

  std::set<int> ignore; 

  if (ignoreEmpty) 
  {
    for (int j = 1; j <= nOrth; j++) 
    {
      bool empty = true; 

      for (int i =1; i <= h->GetNbinsX(); i++) 
      {
        if( H->GetBinContent( axis == 1 ? i : j, axis == 1 ? j : i))
        {
          empty = false; 
          break; 
        }
      }

      if (empty) 
        ignore.insert(j); 
    }

  }

  for (int i = 1; i <= h->GetNbinsX(); i++) 
  {
    std::vector<double> v;
    v.reserve(nOrth); 
    for (int j = 1; j <= nOrth; j++)
    {
      if (ignore.count(j)) 
        continue; 

      v.push_back( H->GetBinContent( axis == 1 ? i : j, axis == 1 ? j : i)); 
    }
    std::nth_element(v.begin(), v.begin() + (v.size()-1) * pct, v.end()); 
    h->SetBinContent(i, v[(v.size()-1)*pct]); 
  }

  return h; 
} 


static double bicubicFunction(double x, double * params)
{
  double * a = params+1;  //hack to use same notation as in wikipedia :)
  return 0.5 * (2*a[0] + x*(-a[-1] + a[1]) + x*x*(2*a[-1]-5*a[0]+4*a[1] -a[2]) + x*x*x*(-a[-1] + 3*a[0] - 3*a[1] + a[2])); 
}


double UCorrelator::image::interpolate(const TH2 *h, double x, double y, InterpolationType type, InterpolationEdgeBehavior flags_x, InterpolationEdgeBehavior flags_y, bool centers)
{

  /*
  if (type == NEAREST) 
  {
    int xbin = h->GetXaxis()->FindFixBin(x); 
    int ybin = h->GetXaxis()->FindFixBin(x); 

    if (centers)  //this is easy 
    {
      return h->GetBinContent(xbin,ybin); 
    }

    //otherwise, we want to find the closest corner. 




  }


  if (type == BILINEAR)  
  {

    //find the bin
    
    int xbin = h->GetXaxis()->FindFixBin(x); 
    int ybin = h->GetXaxis()->FindFixBin(x); 

    if (centers && x < h->GetXaxis()->GetBinCenter(xbin) )
      xbin = (xbin == 1 && edge_x == PERIODIC) ? h->GetNbinsX() : xbin-1; 

    if (centers && y < h->GetYaxis()->GetBinCenter(ybin) )
      ybin = (ybin == 1 && edge_y == PERIODIC) ? h->GetNbinsY() : ybin-1; 




    //bins used to get the value 
    int xbin2 = xbin1 < h->GetNbinsX() ? xbin1 + 1 : 
                flags_x == PERIODIC   ? 1        :
                flags_x == EXTEND     ? xbin1     :
                xbin+1; //this will be the overflow bin which I suppose will usually be zero 

    int ybin2 = ybin < h->GetNbinsY() ? ybin1 + 1 : 
                flags_y == PERIODIC   ? 1        :
                flags_y == EXTEND     ? ybin1     :
                ybin+1; //this will be the overflow bin which I suppose will usually be zero 

    //get the values 
    double q11  = h->GetBinContent(xbin,ybin); 
    double q21  = h->GetBinContent(xbin2,ybin); 
    double q12  = h->GetBinContent(xbin,ybin2); 
    double q21  = h->GetBinContent(xbin2,ybin2); 


    // TODO: special case the uniform bin size case
    
    double xrel = x - h->GetXaxis()->GetBinLowEdge(xbin); 
    double yrel = y - h->GetYaxis()->GetBinLowEdge(ybin); 
    double xrel2 = xwidth - xrel; 
    double yrel2 = ywidth - yrel; 

    return 1./ (xwidth * ywidth)  * ( q11 * xrel2 * yrel2 + q21 *xrel * yrel2 + q12 * xrel2 * yrel  + q22 * xrel * yrel); 
  }

  if (type  == BICUBIC)
  {

    if (h->GetXaxis()->GetBins() || h->GetYaxis()->GetBins())
    {
      fprintf(stderr,"WARNING, bicubic interpolation does not work for non-uniform binning. Reverting to bilinear.\n"); 
      return interpolate(h,x,y,BILINEAR,flags_x,flags_y); 
    }

    
    double xmin = h->GetXaxis()->GetXmin(); 
    double xmax = h->GetXaxis()->GetXmax(); 
    double ymin = h->GetYaxis()->GetXmin(); 
    double ymax = h->GetYaxis()->GetXmax(); 
    double xwidth = (xmax-xmin) / (h->GetNbinsX()); 
    double ywidth = (ymax-ymin) / (h->GetNbinsY()); 

  
    int binx0 = (int) ((x - xmin) / xwidth + 0.5); 
    int biny0 = (int) ((y - ymin) / ywidth + 0.5); 


    double b[4]; 
    for (int iy = -1; iy <3; iy++)
    {
      double a[4]; 
      int biny = biny0 + iy; 

      if (biny < 1) 
      { 
        biny = flag_y == PERIODIC ? biny 

      }

      for (int ix = -1; ix <3; ix++)
      {
        a[ix+1] = in->GetBinContent(binx, biny); 
      }
      b[iy+1] = bicubicFunction( (x - in->GetXaxis()->GetBinCenter(binx0)) / xwidth,a); 
    }
    
    return bicubicFunction((y - in->GetYaxis()->GetBinCenter(biny0)) / ywidth,b);  


  }

*/

  return 0; 

}

