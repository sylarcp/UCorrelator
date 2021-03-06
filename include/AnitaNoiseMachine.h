#ifndef ANITA_NOISE_MACHINE
#define ANITA_NOISE_MACHINE

#include "AnitaConventions.h"
#include "AnitaNoiseSummary.h"
#include "AnitaEventSummary.h"
#include "FilteredAnitaEvent.h"
#include "Analyzer.h"
#include "UCorrelatorGUI.h"
#include "UCImageTools.h"

/*=====================
  A class to process and save information about the thermal environment*/
class AnitaNoiseMachine
{
 public:

  /* Constructor */
  AnitaNoiseMachine(const int length = 60);

  const int fifoLength; //one minute of noise averaging

  bool quiet;// = true; //whether to complain about things that I randomly decided upon

  //do you want to save the interferometric maps?  They are very large.  Also multiple ways to save them
  bool fillMap ;    //save the min bias maps as TH2D (~18kB per event)
  bool fillAvgMap; //save it as the average of fifoLength min bias maps (Won't work if fillMap==true)
  bool fillArray;  //save it as a double array (25% smaller, ~14kB per event)
  
  /* Reset */
  void zeroInternals();

  /* makes an averaged TProfile2D out of the histograms in the mapFifo */
  TProfile2D *getAvgMapNoiseProfile(AnitaPol::AnitaPol_t pol);


  /* updates all the fifos with the current event */
  void updateMachine(UCorrelator::Analyzer *analyzer,FilteredAnitaEvent *filtered);


  /* Moves things into the summary */
  void fillNoiseSummary(AnitaNoiseSummary *noiseSummary); //crabcrabcrab

  /* Fill the mapHistoryVal value in AnitaEventSummary (eventSummary should be mostly filled already) */
  void fillEventSummary(AnitaEventSummary *eventSummary);
  //crab


  /* check if it is still all zeros or what */
  bool isJustInitialized() { return fJustInitialized; };


 private:

  //Makes sure the fifos start at zero
  bool fJustInitialized;

  /** Saves the last updated heading value */
  double lastHeading;
  
  //induvidual update functions which need to all be called at once so the fifos incriment properly
  /* for building up an enormous memory block of histograms from a bunch of events, then making an average 
              also does the double array   */
  void updateAvgMapFifo(UCorrelator::Analyzer *analyzer, FilteredAnitaEvent *filtered);
  /* for calculating rms of waveform from a minute average before event capture */
  void updateAvgRMSFifo(FilteredAnitaEvent *filtered);

  /* fills up the history for a single source, called by fillEventSummary */
  void setSourceMapHistoryVal(AnitaEventSummary::SourceHypothesis& source);

  //internals for time domain waveform rms fifo
  double *rmsFifo; //where the info is saved
  int rmsFifoPos; //where in the fifo the most recent write was
  bool rmsFifoFillFlag ; //whether you've completely filled the fifo once
  int rmsFifoIndex(int pol,int ringi,int poli,int pos); //so I know where to write in the 1d array
  double rmsAvg[NUM_PHI][NUM_ANTENNA_RINGS][NUM_POLS]; //a running count of the average per channel


  //internals for interferometric map fifo (probably enormous in memory so maybe make a flag)
  TH2 **mapFifo[NUM_POLS]; //where the info is saved
  int mapFifoPos;  //where in the fifo the most recent write was
  bool mapFifoFillFlag ; //whether you've completely filled the fifo once.
  int mapFifoIndex(); 


  //maybe will be more compressed
  static const int nPhi = 180; //default in UCorrelator::AnalysisConfig, this is hard to make dynamic
  static const int nTheta = 100; //default in UCorrelator::AnalysisConfig, this is hard to make dynamic
  //where the quickly calculated double array is stored.  should be maxDirections*nPhi*nTheta*NUM_POLS long
  double *rollingMapAvg;
  int rollingMapIndex(int poli,int iPhi,int iTheta); 

  ClassDefNV(AnitaNoiseMachine, 5); 

};
/*------------------*/


#endif
