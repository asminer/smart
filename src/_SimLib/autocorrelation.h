/*
 *  autocorrelation.h
 *  
 *
 *  Created by Yaping Jing on 12/23/07.
 *  Copyright 2007 All rights reserved.
 *
 */

#ifndef AUTOCORRELATION_H
#define AUTOCORRELATION_H

class autocorrelation{

private: 
  int count;
  double sum;
  double mean;
  double autocorre;
  double variance;
  double preData;
  double covariance;
  
public:
  autocorrelation();
  ~autocorrelation();
  void add(double data);
  double getSum();
  double getMean();
  double getVariance();
  double getAutocorrelation();
  double getSampleSize();
  
};



#endif


