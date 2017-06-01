#include "ProbabilityMap.h" 
#include "UCUtil.h"
#include "UCFilters.h" 
#include "PointingResolutionModel.h"
#include "Analyzer.h" 
#include "AnalysisConfig.h" 
#include "AnitaDataset.h" 
#include "SystemResponse.h" 


void testMapWais(int run =342, int max = 0) 
{

  FFTtools::loadWisdom("wisdom.dat"); 
  AnitaDataset d(run); 


  UCorrelator::AnalysisConfig cfg; 
  cfg.nmaxima = 1; 
  cfg.response_option = UCorrelator::AnalysisConfig::ResponseIndividualBRotter; 
  cfg.deconvolution_method = new AnitaResponse::AllPassDeconvolution; 

  UCorrelator::Analyzer  analyzer (&cfg); 
  AnitaEventSummary sum; 
  FilterStrategy strategy; 
  UCorrelator::fillStrategyWithKey(&strategy,"sinsub_5_3_ad_2"); 

  StereographicGrid g(1024,1024); 
  UCorrelator::ConstantPointingResolutionModel m(0.2,0.3);
  UCorrelator::ProbabilityMap map(&g,&m); 
  int ndone = 0; 

  for (int i = 0; i< d.N(); i++)
  {
    d.getEntry(i); 
    printf("----(%d)-----\n",i); 
    UsefulAdu5Pat pat(d.gps()); 
    if (UCorrelator::isWAISHPol(&pat, d.header()))
    {
      printf("Processing event %d (%d)\n",d.header()->eventNumber,ndone); 
      FilteredAnitaEvent ev(d.useful(), &strategy, d.gps(), d.header()); 

      analyzer.analyze(&ev, &sum); 
      map.add(&sum,d.gps(), AnitaPol::kHorizontal, 0); 
      ndone++;
    }


    if (max && ndone >= max) break; 

  }

  map.segmentationScheme()->Draw("colz", map.getProbabilities()); 


  FFTtools::saveWisdom("wisdom.dat"); 
}