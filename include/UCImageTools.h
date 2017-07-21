#ifndef UCORRELATOR_IMAGE_TOOLS_H
#define UCORRELATOR_IMAGE_TOOLS_H

/** Image tools for working with TH2*'s
 *
 * Cosmin Deaconu <cozzyd@kicp.uchicago.edu> 
 *
 *  Also other things!  Ben Rotter <BenJRotter@gmail.com>
 **/


class TH1; 
class TH2; 
class TProfile2D;

namespace UCorrelator
{

  /*=====
    TH1::GetRMS(3) stopped working so here is a function to just calculate it by hand
   */
  double getZRMS(const TH2*);

  /*====
    Need a way to easily rotate the maps to account for heading.  Makes a new map since the input is probably const
    Always returns something that goes from 0->360 with 0/360 being north
  */
  TH2* rotateHistogram(const TH2* inHist,double rotate);

  /*===
    Tools for averaging together interferometric maps, or any histograms really:
    TH2toTProfile2D - "upgrades" a TH2 to a TProfile2D, which can do more stuff. Doesn't copy any of the data, just the frame.
   */
  TProfile2D *TH2toTProfile2D(TH2* inTH2);

  /*===
    Tools for averaging together interferometric maps, or any histograms really:
    fillTProfile2DWithTH2 - "overlays" a TH2 onto a TProfile2D, which will average them together
   */
  void fillTProfile2DWithTH2(TProfile2D *prof, TH2* hist);


  namespace image
  {

    TH1 * getPctileProjection(const TH2 * h, int axis = 1, double pct = 0.5, bool ignoreEmpty = true); 

  }; 
}; 

#endif
