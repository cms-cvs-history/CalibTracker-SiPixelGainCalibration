//
// This class is a first attempt at writting a configuration
// object that will perform a calibration loop.
//
//
//

#include "CalibTracker/SiPixelGainCalibration/interface/PixelCalib.h"
#include <fstream>
#include <iostream>
#include <ios>
#include <assert.h>
#include <stdlib.h>

char* itoa(int i){
    assert(0);
}

PixelCalib::PixelCalib(std::string filename):
  PixelConfigBase("","",""){


    old_irows=-1;
    old_icols=-1;
    if (filename =="")
	return;

    std::ifstream in(filename.c_str());

    if (!in.good()){
	std::cout << "Could not open:"<<filename<<std::endl;
	assert(0);
    }
    else {
	std::cout << "Opened:"<<filename<<std::endl;
    }

    std::string tmp;

    in >> tmp;

    assert(tmp=="Rows:");

    in >> tmp;

    std::vector <unsigned int> rows;
    while (tmp!="Cols:"){
	if (tmp=="|") {
	    rows_.push_back(rows);
            rows.clear();
	}
	else{
	  int arow=(atoi(tmp.c_str()));
	  assert(arow>=0);
	  assert(arow<=79);
	  rows.push_back(arow);
	}
	in >> tmp;
    }
    rows_.push_back(rows);
    rows.clear();
    

    in >> tmp;

    std::vector <unsigned int> cols;
    while (tmp!="Vcal:"){
	if (tmp=="|") {
	    cols_.push_back(cols);
            cols.clear();
	}
	else{
	  int acol=(atoi(tmp.c_str()));
	  assert(acol>=0);
	  assert(acol<=79);
	  cols.push_back(acol);
	}
	in >> tmp;
    }
    cols_.push_back(cols);
    cols.clear();

    in >> vcal_first_ >> vcal_last_ >> vcal_step_;

    /*
    std::cout << "pixelCalib: vcal first, last, step "
	      << vcal_first_ <<" "
	      << vcal_last_ <<" "
	      << vcal_step_ << std::endl;
    */
    in >> tmp;

    assert(tmp=="Repeat:");

    in >> ntrigger_;


    in >> tmp;

    if (in.eof()){
	roclistfromconfig_=false;
	in.close();
	return;
    }

    assert(tmp=="Rocs:");
  
    in >> tmp;

    while (!in.eof()){
	rocs_.push_back(tmp);
	in >> tmp;
    }

    in.close();

    return;

}

unsigned int PixelCalib::vcal(unsigned int state) const{

    assert(state<nConfigurations());

    unsigned int i_vcal=state%nVcal();
    unsigned int vcal=vcal_first_+i_vcal*vcal_step_;

    return vcal;

}

std::ostream& operator<<(std::ostream& s, const PixelCalib& calib){

    s<< "Rows:"<<std::endl;
    for (unsigned int i=0;i<calib.rows_.size();i++){
	for (unsigned int j=0;j<calib.rows_[i].size();j++){
	    s<<calib.rows_[i][j]<<" "<<std::endl;
	}
	s<< "|"<<std::endl;
    }

    s<< "Cols:"<<std::endl;
    for (unsigned int i=0;i<calib.cols_.size();i++){
	for (unsigned int j=0;j<calib.cols_[i].size();j++){
	    s<<calib.cols_[i][j]<<" "<<std::endl;
	}
	s<< "|"<<std::endl;
    }

    s << "Vcal:"<<std::endl;

    s << calib.vcal_first_ << " " << calib.vcal_last_ 
      << " "<< calib.vcal_step_<<std::endl;

    s << "Repeat:"<<std::endl;
    
    s << calib.ntrigger_<<std::endl;

    return s;

}


void PixelCalib::getRowsAndCols(unsigned int state,
				std::vector<unsigned int>& rows,
				std::vector<unsigned int>& cols) const{


    assert(state<nConfigurations());

    unsigned int i_vcal=state%nVcal();
    unsigned int vcal=vcal_first_+i_vcal*vcal_step_;

    unsigned int i_row=(state/nVcal())/cols_.size();

    unsigned int i_col=(state/nVcal())%cols_.size();

    assert(i_row<rows_.size());
    assert(i_col<cols_.size());

    rows=rows_[i_row];
    cols=cols_[i_col];

}
