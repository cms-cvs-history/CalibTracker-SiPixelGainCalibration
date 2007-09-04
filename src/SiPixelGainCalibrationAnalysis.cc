// -*- C++ -*-
//
// Package:    SiPixelGainCalibrationAnalysis
// Class:      SiPixelGainCalibrationAnalysis
// 
/**\class SiPixelGainCalibrationAnalysis SiPixelGainCalibrationAnalysis.cc CalibTracker/SiPixelGainCalibrationAnalysis/src/SiPixelGainCalibrationAnalysis.cc

 Description: <one line class summary>

 Implementation:
     <Notes on implementation>
*/
//
// Original Author:  Freya Blekman
//         Created:  Mon May  7 14:22:37 CEST 2007
// $Id: SiPixelGainCalibrationAnalysis.cc,v 1.7 2007/08/29 15:52:41 fblekman Exp $
//
//


// system include files
#include <memory>
#include <map>

// user include files
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDAnalyzer.h"
#include "PhysicsTools/UtilAlgos/interface/TFileDirectory.h"
#include "PhysicsTools/UtilAlgos/interface/TFileService.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"
// other classes
#include "DataFormats/SiPixelDigi/interface/PixelDigi.h"
#include "DataFormats/DetId/interface/DetId.h"
#include "DataFormats/SiPixelDigi/interface/PixelDigiCollection.h"
#include "DataFormats/Common/interface/DetSetVector.h"
#include "CalibTracker/SiPixelGainCalibration/interface/SiPixelGainCalibrationAnalysis.h"
#include "DataFormats/FEDRawData/interface/FEDRawData.h"
#include "DataFormats/FEDRawData/interface/FEDRawDataCollection.h"
#include "CondCore/DBOutputService/interface/PoolDBOutputService.h"
#include "DataFormats/SiPixelDetId/interface/PXBDetId.h"
#include "DataFormats/SiPixelDetId/interface/PXFDetId.h"
#include "Geometry/Records/interface/TrackerDigiGeometryRecord.h" 
#include "Geometry/TrackerTopology/interface/RectangularPixelTopology.h"
#include "Geometry/TrackerGeometryBuilder/interface/TrackerGeometry.h"
#include "Geometry/TrackerGeometryBuilder/interface/PixelGeomDetUnit.h"
#include "Geometry/TrackerGeometryBuilder/interface/PixelGeomDetType.h"
#include "DataFormats/SiPixelDetId/interface/PixelEndcapName.h"
#include "DataFormats/SiPixelDetId/interface/PixelBarrelName.h"
// root classes
#include "TF1.h"
#include "TRandom.h"
#include "TLinearFitter.h"
//
// static data member definitions
//

//
// constructors and destructor
//
SiPixelGainCalibrationAnalysis::SiPixelGainCalibrationAnalysis(const edm::ParameterSet& iConfig):
  recordName_(iConfig.getParameter<std::string>("record")),
  conf_(iConfig),
  appendMode_(conf_.getUntrackedParameter<bool>("appendDatabaseMode",false)),
  SiPixelGainCalibration_(0),
  SiPixelGainCalibrationService_(iConfig),
  eventno_counter_(0),
  src_( iConfig.getUntrackedParameter<std::string>("src","source")),
  instance_ (iConfig.getUntrackedParameter<std::string>("InputInstance","")),
  inputconfigfile_( iConfig.getUntrackedParameter<std::string>( "inputFileName","/afs/cern.ch/cms/Tracker/Pixel/forward/ryd/calib_070106d.dat" ) ),
  vcal_fitmin_(256),
  vcal_fitmax_(0),
  vcal_fitmax_fixed_( iConfig.getUntrackedParameter<uint32_t>( "cutoffVCalFit",100)),
  maximum_ped_(iConfig.getUntrackedParameter<double>("maximumPedestal",120.)),
  maximum_gain_(iConfig.getUntrackedParameter<double>("maximumGain",5.)),
  minimum_ped_(iConfig.getUntrackedParameter<double>("minimumPedestal",120.)),
  minimum_gain_(iConfig.getUntrackedParameter<double>("minimumGain",5.)),
  save_histos_(iConfig.getUntrackedParameter<bool>("saveAllHistos",false)),
  test_(iConfig.getUntrackedParameter<bool>("dummyData",false)),
  fitfuncrootformula_(iConfig.getUntrackedParameter<std::string>("rootFunctionForFit","pol1")),
  only_one_detid_(iConfig.getUntrackedParameter<bool>("onlyOneDetID",false)),
  usethisdetid_(0),
  do_scurveinstead_(iConfig.getUntrackedParameter<bool>("doSCurve",false)),
   saveAllGainCurvesForGivenPlaquettes_(iConfig.getUntrackedParameter<bool>("saveAllGainCurvesForGivenPlaquettes", false)),
   saveGainCurvesWithBadChi2_(iConfig.getUntrackedParameter<bool>("saveGainCurvesWithBadChi2", false)),
   maximumChi2overNDF_(iConfig.getUntrackedParameter<double>("maximumChi2", 4)),
   dropLowVcalOutliersForCurvesWithBadChi2_(iConfig.getUntrackedParameter<bool>("dropLowVcalOutliersForCurvesWithBadChi2", false)),
   useFirstNonZeroBinForFitMin_(iConfig.getUntrackedParameter<bool>("useFirstNonZeroBinForFitMin", false))

{
   //now do what ever initialization is needed
  if(test_)
    save_histos_=true;
  
  // std::auto_ptr <PixelCalib> bla( new PixelCalib(inputconfigfile_));
  PixelCalib tempcalib(inputconfigfile_);
  calib_ =tempcalib;

  //intiatialize list of plaquettes to save (default empty)
  std::vector<std::string> defaultEmptyVString;
  plaquettesToSave_ = iConfig.getUntrackedParameter< std::vector<std::string> >("plaquettesToSave", defaultEmptyVString );
  // intialize error dir
  if (saveGainCurvesWithBadChi2_)
     errordir_ = new TFileDirectory(therootfileservice_->mkdir("Errors", "Errors"));


  // database vars
  ::putenv("CORAL_AUTH_USER=me");
  ::putenv("CORAL_AUTH_PASSWORD=test");
  if(test_)
    edm::LogInfo("INFO") << "Using test configuration, dummy data being entered into histograms..." << std::endl;
}


SiPixelGainCalibrationAnalysis::~SiPixelGainCalibrationAnalysis()
{
 
   // do anything here that needs to be done at desctruction time
   // (e.g. close files, deallocate resources etc.)

}


//
// member functions
//

bool SiPixelGainCalibrationAnalysis::doFits(){
  TH1F gr("temporary","temporary",calib_.nVcal(),calib_.vcal_first(),calib_.vcal_last());
  for(std::map<uint32_t,uint32_t>::iterator imap = detIDmap_.begin(); imap!=detIDmap_.end(); ++imap){
    uint32_t detid= imap->first;
    TString detidname = detnames_[detid];
    //    std::cout << detidname << " " << calibPixels_[detid].size()<< std::endl;
    TFileDirectory thisdir = therootfileservice_->mkdir(detidname.Data(),detidname.Data());
    for(uint32_t ipixel=0; ipixel<calibPixels_[detid].size(); ipixel++){
      //std::cout << "event : " << eventno_counter_ << " summary for pixel : " << ipixel << "(="<< detid << ","<< colrowpairs_[detid][ipixel].first << "," << colrowpairs_[detid][ipixel].second<<")"<<  std::endl;
	
      TString histname = detidname;
      histname += ", row " ;
      histname+=colrowpairs_[detid][ipixel].first;
      histname += ", col " ;
      histname+=colrowpairs_[detid][ipixel].second;
      //    std::cout << histname << std::endl;
      float slope_last3points=200;
      float plateaustart = -1;
      int plateauStartBin = -1;
      int firstNonZeroBin = -1;
      for(uint32_t ipoint=0; ipoint<calibPixels_[detid][ipixel].npoints(); ipoint++){

	float response = calibPixels_[detid][ipixel].getpoint(ipoint,0);
	float error = calibPixels_[detid][ipixel].geterror(ipoint,1);
	if(do_scurveinstead_){
	  float m = calibPixels_[detid][ipixel].getentries(ipoint);
	  float n = calib_.nTriggers();
	  response = m/n;
	  float d = n *n *(1+2.*m)-4.*n*(1.+n)*m*m;
	  //only fill the largest of the two (asymmetric) binomial errors
	  float temperror = ( n*(1.0+2.0*m) + sqrt(d) )/(2.0*n*(1.0+n));
	  error = fabs(temperror-response);
	  temperror = ( n*(1.0+2.0*m) - sqrt(d) )/(2.0*n*(1.0+n));
	  if(fabs(temperror-response)>error)
	    error = fabs(temperror-response);
	}
	gr.SetBinContent(ipoint,response);
	gr.SetBinError(ipoint,error);
	if (response > 0 && firstNonZeroBin < 0)
	   firstNonZeroBin = ipoint;
	///	std::cout << "filled hist: " << gr.GetBinCenter(ipoint) << " " << gr.GetBinContent(ipoint) << " " << gr.GetBinError(ipoint) << std::endl;
	
	if (useFirstNonZeroBinForFitMin_)
	   vcal_fitmin_ = gr.GetBinLowEdge(firstNonZeroBin);
	//only start looking once we've found the first non zero bin
	if(firstNonZeroBin > 0 && ipoint-firstNonZeroBin > 3 && plateaustart<0){
	  float npoints=0;
	  slope_last3points = 0;
	  for(int ii=ipoint; ii>0 && npoints<3; ii--){
	    if(gr.GetBinContent(ii)>0 && gr.GetBinContent(ii-1)>0){
	      slope_last3points += (gr.GetBinContent(ii)-gr.GetBinContent(ii-1))/gr.GetBinWidth(ii);
	      npoints++;
	    }
	  }
	  slope_last3points /= npoints;
	  if(fabs(slope_last3points)<0.5 || gr.GetBinLowEdge(ipoint + 1) > 50) {
	     if (!useFirstNonZeroBinForFitMin_ || (useFirstNonZeroBinForFitMin_ && ipoint - firstNonZeroBin > 5))
	     {
		plateaustart = gr.GetBinLowEdge(ipoint);
		plateauStartBin = ipoint;
	     }
	  }
	  //	std::cout << "slope is : " << slope_last3points << " for point " << gr.GetBinCenter(ipoint) << std::endl;
	}
      }

      if(plateaustart>0)
	func->SetRange(vcal_fitmin_,0.8*plateaustart);
      else
	continue;
      // copy to a new object that is saved in the file service:


      // The following is an algorithm to drop outlying points in the lowVcal range
      // Do not use unless you are sure you know why the points are bad in the first place!
      bool redoFit = false;
      gr.Fit(func, "RQ");

      if (func->GetChisquare()/func->GetNDF() > maximumChi2overNDF_ && dropLowVcalOutliersForCurvesWithBadChi2_ ) { 
	 //std::cout << "Found bad fit in " << detidname << "[" << colrowpairs_[detid][ipixel].first << "],[" << colrowpairs_[detid][ipixel].second << "] with a chi-square of " << func->GetChisquare() << std::endl;
	 int testPoint = firstNonZeroBin+1;
	 bool doneWithOutlierCheck = false;
	 float lastDiff = 0;
	 while (!doneWithOutlierCheck)
	 {
	    int nEntriesLeft = 0;
	    int nEntriesRight = 0;
	    float sumLeft = 0;
	    float sumRight = 0;
	    //compute the average difference between successive bins on the left and right side of testPoint
	    for (int iBin = firstNonZeroBin; iBin < plateauStartBin; iBin++){
	       float diff = gr.GetBinContent(iBin + 1) - gr.GetBinContent(iBin);
	       if (iBin < testPoint) {
		  sumLeft += diff;
		  nEntriesLeft++;
	       } else {
		  sumRight += diff;
		  nEntriesRight++;
	       }
	    }
	    if (nEntriesLeft > 0 && nEntriesRight > 0) {
	       sumLeft /= nEntriesLeft;
	       sumRight /= nEntriesRight;
	    }
	    //std::cout << "Test point: " << testPoint << " AvgL: " << sumLeft << " AvgR: " << sumRight << endl << "Diff: " << sumRight - sumLeft << endl;
	    // maximize the difference between left and right slows
	    float diffLR = fabs(sumRight - sumLeft);
	    if (diffLR < lastDiff && diffLR / sumLeft > 0.05) {
	       doneWithOutlierCheck = true;
	       redoFit = true;
	       func->SetRange(gr.GetBinLowEdge(testPoint - 1), plateaustart);
	       //std::cout << "Cutting off at low edge of bin " << testPoint - 1 << endl;
	       //mark the cut off bins with a large error (only for validating algorithm)
	       //gr.SetBinError(testPoint - 2, gr.GetBinContent(testPoint - 2));

	    }
	    lastDiff = fabs(sumRight - sumLeft);
	    testPoint++;
	    if (testPoint >= plateauStartBin)
	       doneWithOutlierCheck = true;
	 }
      }
      
      float ped = 255;
      float gain = 255;
      float chi2 = -1;
      float plat = -1;

      //check if this is a plaquette that we want to save all gain curves for
      bool thisDetIdinSaveList = false;
      for (std::vector<std::string>::const_iterator plaquetteToSave = plaquettesToSave_.begin(); plaquetteToSave != plaquettesToSave_.end(); ++plaquetteToSave)
      {
	 if ( (*plaquetteToSave) == detidname )
	 {
	    thisDetIdinSaveList = true;
	    break;
	 }
      }

      //save the histogrames using a perverse application of De Morgan's law
      if(!save_histos_  && !(saveAllGainCurvesForGivenPlaquettes_ && thisDetIdinSaveList) ){
	 //only refit if we dropped points - this should only run if dropLowVcalOutliersForCurvesWithBadChi2_ is set
	 if (redoFit)
	    gr.Fit(func,"RQ");
      }
      else{
	 TH1F *gr2 = thisdir.make<TH1F>(histname.Data(),histname.Data(),calib_.nVcal(),calib_.vcal_first(),calib_.vcal_last());
	 gr2->Sumw2();
	 gr2->SetMarkerStyle(22);
	 gr2->SetMarkerSize(0.5*gr2->GetMarkerSize());
	 for(int ibin=-1; ibin<=gr.GetNbinsX(); ibin++){
	    gr2->SetBinContent(ibin,gr.GetBinContent(ibin));
	    gr2->SetBinError(ibin,gr.GetBinError(ibin));
	 }
	 gr2->Fit(func,"QR");
      }

      //save the gain curves that had to be refit to a seperate directory
      if (redoFit || (saveGainCurvesWithBadChi2_ && func->GetChisquare()/func->GetNDF() > maximumChi2overNDF_)) {
	 assert(errordir_);
	 TH1F *gr3 = errordir_->make<TH1F>(histname.Data(),histname.Data(),calib_.nVcal(),calib_.vcal_first(),calib_.vcal_last());
	 gr3->Sumw2();
	 gr3->SetMarkerStyle(22);
	 gr3->SetMarkerSize(0.5*gr3->GetMarkerSize());
	 for(int ibin=-1; ibin<=gr.GetNbinsX(); ibin++){
	    gr3->SetBinContent(ibin,gr.GetBinContent(ibin));
	    gr3->SetBinError(ibin,gr.GetBinError(ibin));
	 }
	 //is there a way to draw the fit without recomputing it?
	 gr3->Fit(func,"QR");
      }

      ped =func->GetParameter(0);
      gain = func->GetParameter(1);
      /*
      if (gain == 0) {
	 std::cout << "Gain of zero for " << histname.Data() << " with Nentries " << gr.GetNbinsX() << " and fit chi2 " << func->GetChisquare() << " and NDF " << func->GetNDF() << std::endl;
      }
      //DEBUG
      if (thisDetIdinSaveList && colrowpairs_[detid][ipixel].first == 0 && colrowpairs_[detid][ipixel].second == 201) {
	 for(uint32_t ipoint=0; ipoint<calibPixels_[detid][ipixel].npoints(); ipoint++){
	    std::cout << "getPoint: " << calibPixels_[detid][ipixel].getpoint(ipoint, 0) << " Nentries: " 
	       << calibPixels_[detid][ipixel].getentries(ipoint) << "   " << calibPixels_[detid][ipixel].geterror(ipoint, 1) << std::endl;
	 }
      }*/

      summaries1D_pedestal_[detid]->Fill(ped);
      summaries1D_gain_[detid]->Fill(gain);
      summaries1D_plat_[detid]->Fill(plat);
      summaries_pedestal_[detid]->Fill(colrowpairs_[detid][ipixel].second,colrowpairs_[detid][ipixel].first,ped);
      summaries_gain_[detid]->Fill(colrowpairs_[detid][ipixel].second,colrowpairs_[detid][ipixel].first,gain);
      summaries_plat_[detid]->Fill(colrowpairs_[detid][ipixel].second,colrowpairs_[detid][ipixel].first,plat);
      summaries_chi2_[detid]->Fill(colrowpairs_[detid][ipixel].second,colrowpairs_[detid][ipixel].first,chi2);
      calibPixels_[detid][ipixel].clearAllPoints();
    }
  }
  return true;
  

}


void
SiPixelGainCalibrationAnalysis::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup)
{
  
  
   uint32_t state = eventno_counter_ / calib_.nTriggers();  
   uint32_t ivcal = state % calib_.nVcal();
   uint32_t vcal = calib_.vcal(state);
   
   uint32_t nextswitch = (eventno_counter_)/(calib_.nTriggers()*calib_.nVcal());
   nextswitch ++;
   nextswitch *= calib_.nTriggers()*calib_.nVcal();
   //   std::cout << state << " " << eventno_counter_ << " " << ivcal << " " << nextswitch << std::endl;
   if(eventno_counter_%(calib_.nTriggers()*calib_.nVcal())==0){
     new_configuration_=true;
     if(eventno_counter_!=0){
       doFits();    
       // and reset everything...
       //       std::cout << "and resetting:" << std::endl;
       for( std::map < uint32_t, uint32_t >::iterator detiter=detIDmap_.begin(); detiter!=detIDmap_.end(); ++detiter){
	 
	 while(colrowpairs_[detiter->first].size()!=0)
	   colrowpairs_[detiter->first].erase(colrowpairs_[detiter->first].end());
	 //     std::cout << "size of colrowpairs now : " << colrowpairs_.size() <<std::endl;
       }
     }
   }
   else
     new_configuration_=false;
   if(new_configuration_)
     edm::LogInfo("INFO") << "now using config: VCAL="<< vcal << " VCALSTATE=" << calib_.vcal(state) << " NTRIGGERS=" << calib_.nTriggers() << " NVCAL=" << calib_.nVcal() << " VCALSTEP=" << calib_.vcal_step() << " VCALFIRST=" << calib_.vcal_first() << " NTRIGGERSTOTAL=" << calib_.nTriggersTotal() << std::endl;

   // do loop over 
   iSetup.get<TrackerDigiGeometryRecord>().get( geom_ );
   const TrackerGeometry& theTracker(*geom_);
   // Get event setup (to get global transformation)  
   edm::Handle< edm::DetSetVector<PixelDigi> > pixelDigis;
   iEvent.getByLabel( src_, pixelDigis );
   edm::DetSetVector<PixelDigi>::const_iterator digiIter;
   //   edm::LogInfo("SiPixelGainCalibrationAnalysis") << "SiPixelGainCalibrationAnalysis:found " << pixelDigis->size() << " digi hits..." << std::endl;
   for (digiIter = pixelDigis->begin() ; digiIter != pixelDigis->end(); digiIter++){// loop over detector units...
     uint32_t detid = digiIter->id;   // this is the raw idetector ID...
     DetId detId(detid);
     uint32_t detType = detId.det();
     uint32_t detsubId = detId.subdetId();
     if(detsubId>2 || detsubId<1){
       edm::LogError("SiPixelGainCalibrationAnalysis")<<" Beware: expected forward pixel detector and looking at detector with subdetector type = " << detsubId << "(1=barrel, 2=forward, else=unknown). Det Type = " << detType <<  std::endl;
       //       continue;
     }

     if(usethisdetid_==0){
       usethisdetid_=detid;
       edm::LogInfo("INFO") << "only looking at detector ID: " << detid << std::endl;
     }
     if(usethisdetid_!=detid && only_one_detid_)
       continue;
     int maxcol = 0;
     int maxrow = 0;
     TString detstring = "unknown";
     if(detsubId==2){// FPIX
       PXFDetId pdetId = PXFDetId(detid);
       const PixelGeomDetUnit *theGeomDet = dynamic_cast<const PixelGeomDetUnit*> ( theTracker.idToDet(detId) );   
       maxcol = theGeomDet->specificTopology().ncolumns();
       maxrow = theGeomDet->specificTopology().nrows();
       PixelEndcapName nameworker(detid);
       detstring = nameworker.name().c_str();
       
     }
     else if(detsubId==1){//BPIX
       PXBDetId pdetId = PXBDetId(detid);
       const PixelGeomDetUnit *theGeomDet = dynamic_cast<const PixelGeomDetUnit*> ( theTracker.idToDet(detId) );   
       maxcol = theGeomDet->specificTopology().ncolumns();
       maxrow = theGeomDet->specificTopology().nrows();
       PixelBarrelName nameworker(detid);
       detstring = nameworker.name().c_str();
     }
     else
       continue;
     //std::cout << detstring << std::endl;
     bool new_det=false;
     if(detIDmap_.size()==0)
       new_det=true;
     else{
       std::map<uint32_t, uint32_t>::iterator imap = detIDmap_.find(detid);
       if(imap->first != detid)
	 new_det=true;
     }
     if(new_det){ 
       std::vector<std::pair<uint32_t, uint32_t> > colrowpairs(0);
       std::vector<PixelROCGainCalibPixel> pixels(0,PixelROCGainCalibPixel(calib_.nVcal()));
       edm::LogInfo("SiPixelGainCalibrationAnalysis") << " adding new Det ID " << detid << " at position " << detIDmap_.size() << std::endl;
       detIDmap_[detid]=detIDmap_.size();
       detnames_[detid]=detstring;
       colrowpairs_[detid]=colrowpairs;
       calibPixels_[detid]=pixels;
       TString titleped1d = detstring;
       titleped1d+=" pedestals in all pixels";
       TString titlegain1d = detstring;
       titlegain1d+=" gain in all pixels";
       TString titleplat1d = detstring;
       titleplat1d+=" plateau in all pixels";
       TString titlechi21d = detstring;
       titlechi21d+=" #Chi^{2} / NDOF in all pixels";
       TString titleped2d = detstring;
       titleped2d+=" pedestals";
       TString titlegain2d = detstring;
       titlegain2d+=" gains";
       TString titlechi22d = detstring;
       titlechi22d+=" #Chi^{2} / NDOF";
       TString titleplat2d = detstring;
       titleplat2d+=" plateau";
       summaries1D_pedestal_[detid]=therootfileservice_->make<TH1F>(titleped1d.Data(),titleped1d.Data(),256,0,256);
       summaries1D_gain_[detid]=therootfileservice_->make<TH1F>(titlegain1d.Data(),titlegain1d.Data(),100,0,10);
       summaries1D_chi2_[detid]=therootfileservice_->make<TH1F>(titlechi21d.Data(),titlechi21d.Data(),100,0,20);
       summaries1D_plat_[detid]=therootfileservice_->make<TH1F>(titleplat1d.Data(),titleplat1d.Data(),256,0,256);
       summaries_pedestal_[detid]=therootfileservice_->make<TH2F>(titleped2d.Data(),titleped2d.Data(),maxcol,0,maxcol,maxrow,0,maxrow);
       summaries_gain_[detid]=therootfileservice_->make<TH2F>(titlegain2d.Data(),titlegain2d.Data(),maxcol,0,maxcol,maxrow,0,maxrow);
       summaries_chi2_[detid]=therootfileservice_->make<TH2F>(titlechi22d.Data(),titlechi22d.Data(),maxcol,0,maxcol,maxrow,0,maxrow);
       summaries_plat_[detid]=therootfileservice_->make<TH2F>(titleplat2d.Data(),titleplat2d.Data(),maxcol,0,maxcol,maxrow,0,maxrow);
       //       std::cout << detIDmap_.size() << " " << detnames_.size() << " " << colrowpairs_.size() << " " << calibPixels_.size() << std::endl;
     }
     
     edm::DetSet<PixelDigi>::const_iterator ipix;   
     for(ipix = digiIter->data.begin(); ipix!=digiIter->end(); ipix++){
	 
       bool foundpair=false;
       uint32_t pairindex=0;
       
       std::pair <uint32_t,uint32_t > pixloc(ipix->row(),ipix->column()); 
       //       std::cout << "next pixel " << detid << "," << pixloc.first << "," << pixloc.second << std::endl;
       for(uint32_t ipair=0; ipair<colrowpairs_[detid].size() && !foundpair; ipair++){
	 //       for(std::map< std::vector< std::pair <uint32_t, uint32_t > > >::iterator findthepair = colrowpairs_[detid].begin(); findthepair != colrowpairs_[detid].end() && !foundpair; ++findthepair){
	 //	 if(pixloc.first == findthepair->first && pixloc.second == findthepair->second){
	 if(pixloc.first == colrowpairs_[detid][ipair].first && pixloc.second == colrowpairs_[detid][ipair].second){
	   pairindex=ipair;
	   foundpair=true;
	 }
	 else
	   pairindex++;
       }
       
       if(!foundpair){
	 //	 std::cout << "NEW pixel " << detid << "," << pixloc.first << "," << pixloc.second << std::endl;
	 colrowpairs_[detid].push_back(pixloc);
	 pairindex==colrowpairs_[detid].size()-1;
	 foundpair=true;
	 while(colrowpairs_[detid].size()>calibPixels_[detid].size()){// colrowpairs_ gets reset, only expaind calibPixels_ when really necessary
	   //	   PixelROCGainCalibPixel bla(calib_.nVcal());
	   calibPixels_[detid].push_back(PixelROCGainCalibPixel(calib_.nVcal()));
	 }
	 if(!new_configuration_){
	   TString listofpairs="for this det id the following ";
	   listofpairs += colrowpairs_[detid].size() ;
	   listofpairs+= " pixels are listed: " ;
	   for(std::vector<std::pair<uint32_t, uint32_t> >::iterator ipair = colrowpairs_[detid].begin(); ipair!=colrowpairs_[detid].end(); ++ipair){
	     listofpairs+=ipair->first; 
	     listofpairs+=",";
	     listofpairs+=ipair->second;
	     listofpairs+=" ";
	   }
	   edm::LogInfo("ERROR")<< "WARNING, found unexpected pixel pair detid,row,col:" << detid << "," << colrowpairs_[detid][pairindex].first <<","<< colrowpairs_[detid][pairindex].second <<" in event " << eventno_counter_<< " (next change in pattern expected at event " << nextswitch << ") "<< listofpairs <<  std::endl;
	 }
       }

       if(foundpair ){ 
	 //	 std::cout << "filling pixel detid " << detid << " pair " << pairindex << " " << colrowpairs_[detid][pairindex].first <<","<< colrowpairs_[detid][pairindex].second << ", vcal point " << ivcal << std::endl; 
	 if(calibPixels_[detid][pairindex].npoints()<ivcal){
	   edm::LogInfo("ERROR")<< "WARNING, number of VCal points is " <<calibPixels_[detid][pairindex].npoints() << ", expected " << calib_.nVcal() << ", trying to fill point " << ivcal << std::endl;
	   continue;
	 }
	 //	 std::cout << "before fill: " << calibPixels_[detid][pairindex].getpoint(ivcal,1)<< " " << calibPixels_[detid][pairindex].npoints() << std::endl;
	 calibPixels_[detid][pairindex].addPoint(ivcal,ipix->adc());
	 //	 std::cout << "after fill: " << calibPixels_[detid][pairindex].getpoint(ivcal,1)<< " " << calibPixels_[detid][pairindex].npoints() << std::endl;
       }
     }
   }
   //   std::cout << "now ending event " << eventno_counter_ << std::endl;
   eventno_counter_++;
  
}


void SiPixelGainCalibrationAnalysis::beginJob(const edm::EventSetup&)
{
  //  edm::LogInfo("SiPixelGainCalibrationAnalysis") <<"beginjob: " << nrowsmax_ << " " << ncolsmax_ << " " << nrocsmax_ << " " << nchannelsmax_ << std::endl;
  vcal_fitmin_ =  calib_.vcal_first();
  vcal_fitmax_ =  calib_.vcal_last();
  if(vcal_fitmax_>vcal_fitmax_fixed_)
    vcal_fitmax_=vcal_fitmax_fixed_;
  if(vcal_fitmin_>vcal_fitmax_)
    vcal_fitmin_=0;
  func = new TF1("func",fitfuncrootformula_.c_str(),vcal_fitmin_,vcal_fitmax_);

}

// ------------ method called once each job just after ending the event loop  ------------
void 
SiPixelGainCalibrationAnalysis::endJob(){
  // do the final fits
  if(doFits())
    edm::LogInfo("now starting Database filling...");
  // loop over histograms and save 
  SiPixelGainCalibration_ = new SiPixelGainCalibration();

  for(std::map<uint32_t, uint32_t>::iterator imap = detIDmap_.begin(); imap!=detIDmap_.end(); imap++){
    uint32_t detid = imap->first;
    std::vector<char> theSiPixelGainCalibration;
    uint32_t ncols = summaries_gain_[detid]->GetNbinsY();
    uint32_t nrows = summaries_gain_[detid]->GetNbinsX();
    for(uint32_t icol=0; icol<ncols; icol++){// at the moment: the order of rows... 
      for(uint32_t irow=0; irow<nrows; irow++){
	float ped=summaries_pedestal_[detid]->GetBinContent(irow+1,icol+1);//root histos run from 1 through Nbins
	float gain=summaries_gain_[detid]->GetBinContent(irow+1,icol+1);   //root histos run from 1 through Nbins
	if(ped==0 || gain==0){
	  ped=255;
	  gain=255;
	}
	else if(ped<minimum_ped_||ped>maximum_ped_||gain<minimum_gain_||gain>maximum_gain_){
	  ped=255;
	  gain=255;
	}
	//	std::cout << "Detid,row,col: "<< detid << ","<<irow<< "," << icol << " ped,gain:" << ped << "," << gain <<  std::endl;

	float theEncodedGain  = SiPixelGainCalibrationService_.encodeGain(gain);
	float theEncodedPed   = SiPixelGainCalibrationService_.encodePed (ped);	
	//	edm::LogInfo("SiPixelGainCalibrationAnalysis") << "gains: " << theEncodedGain << " " << gain << std::endl;
	//	edm::LogInfo("SiPixelGainCalibrationAnalysis") << "peds: " << theEncodedPed << " " << ped << std::endl;
	SiPixelGainCalibration_->setData( theEncodedPed, theEncodedGain, theSiPixelGainCalibration);
      }// loop over rows...
    } // loop over columns ...  
    std::cout << "now starting to fill database..." << std::endl;
    SiPixelGainCalibration::Range range(theSiPixelGainCalibration.begin(),theSiPixelGainCalibration.end());
    if( !SiPixelGainCalibration_->put(detid,range,ncols) )
      edm::LogError("SiPixelGainCalibrationAnalysis")<<"[SiPixelGainCalibration:endJob] detid already exists"<<std::endl;
  }// loop over det IDs

    // code copied more-or-less directly from CondTools/SiPixel/test/SiPixelCondObjBuilder.cc
  std::cout << " NOW filling database, this can take a while..." << std::endl;
  edm::LogInfo(" --- writing to DB!");
  edm::Service<cond::service::PoolDBOutputService> mydbservice;
  if(!mydbservice.isAvailable() ){
    edm::LogError("db service unavailable");
    return;
  } 
  else { 
    edm::LogInfo("DB service OK");
  }
  try{
    if( mydbservice->isNewTagRequest(recordName_) ){
      mydbservice->createNewIOV<SiPixelGainCalibration>(SiPixelGainCalibration_, mydbservice->endOfTime(),recordName_);
    }
    else {
      mydbservice->appendSinceTime<SiPixelGainCalibration>(SiPixelGainCalibration_, mydbservice->currentTime(),recordName_);
    }
    edm::LogInfo(" --- all OK");
  } 
  catch(const cond::Exception& er){
    edm::LogError("SiPixelCondObjBuilder")<<er.what()<<std::endl;
  } 
  catch(const std::exception& er){
    edm::LogError("SiPixelCondObjBuilder")<<"caught std::exception "<<er.what()<<std::endl;
  }
  catch(...){
    edm::LogError("SiPixelCondObjBuilder")<<"Funny error"<<std::endl;
  }
}

// additional methods...

