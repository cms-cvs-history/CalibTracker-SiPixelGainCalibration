//---------------------------------------------------

// Author : Freya.Blekman@cern.ch
// Name   : PixelROCGainCalibPixel

//---------------------------------------------------

#ifndef PixelROCGainCalibPixel_H
#define PixelROCGainCalibPixel_H

#include "TObject.h"
#include <vector>
#include "TGraph.h"
//#include "CalibTracker/SiPixelGainCalibration/interface/PixelROCGainCalibElement.h"

class PixelROCGainCalibPixel
{
 private :
 
  std::vector<uint32_t> adcvalues;
  std::vector<uint32_t> nentries;
 public :

  PixelROCGainCalibPixel(uint32_t npoints=60); 

  virtual ~PixelROCGainCalibPixel();

 //-- Setter/Getter
  uint32_t npoints(){return adcvalues.size();}
  void addPoint(uint32_t icalpoint, uint32_t adcval);
  TGraph *createGraph(TString thename, TString thetitle, const int ntimes,std::vector<uint32_t> vcalvalues);
  void init(uint32_t nvcal);
  void clearAllPoints();
  double getpoint(uint32_t icalpoint, uint32_t ntimes);
  bool isfilled();

};

#endif

