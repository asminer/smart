
/*
 *  autocorrelation.cpp
 *  
 *  Created by Yaping Jing on 12/23/07.
 *
 */

#include <iostream>
#include <string.h>
#include <math.h>
#include <vector>
#include "autocorrelation.h"

using namespace std;

autocorrelation::autocorrelation(){
  int count = 0;
  double sum = 0.0;
  double mean = 0.0;
  double autocorre = 0.0;
  double variance = 0.0;
  double preData = 0.0;
  double covariance = 0.0;  
}


void autocorrelation::add(double data){
  count++;
  double diff = data - mean;
  if(count > 1){
      covariance += preData * data;
      variance += diff * diff * (count-1)/count;  
  }
  preData = data; //remember the previous data
  sum += data;
  mean += diff/count;
}


double autocorrelation::getAutocorrelation(){
  double sampleVariance = getVariance();
  if (sampleVariance != 0){
    autocorre = (covariance/(count-1) - mean*mean)/sampleVariance;
  }
  return autocorre;
}

double autocorrelation::getSum(){
  return sum;
}

double autocorrelation::getSampleSize(){
  return count;
}

double autocorrelation::getVariance(){
  if (count > 1 )
    return variance/count;
  else return 0;
}

double autocorrelation::getMean(){
  return mean;
}

autocorrelation::~autocorrelation(){
  int count = 0;
  double sum = 0.0;
  double mean = 0.0;
  double autocorrelation = 0.0;
  double variance = 0.0;
  double preData = 0.0;
  double covariance = 0.0;  
}

//int main(){
//  vector<autocorrelation*> v;
//  autocorrelation *a;
//  for (int i=0; i<5; i++){
//    a = new autocorrelation();
//    v.push_back(a);
//  }
//  for (int i=0; i<5; i++){
//    a = v.at(i);
//    a->add(2);
//    a->add(8);
//    cout << a->getMean() << "; " << a->getVariance() <<  "; " << a->getAutocorrelation()<< endl;
//  }
//  return 0;
//}
