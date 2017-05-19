
// $Id$

//#include <stdio.h>
#include <iostream>
#include <cmath>
#include "sim-models.h"
#include "normal.h"
#include "autocorrelation.h"
#include <vector>

using namespace std;

int nextEvent(double* events, int n, int* flags);

int SIM_BatchMeans(sim_model* m, int N, void* msrdata, sim_confintl* estlist, int batchNum, int batchSize, float percentage){
  
  int samples = batchNum*batchSize;
  double simClock = 0.0; /* current simulation clock */
  double firedTime = 0.0; /* next simulation clock */
  double deltaTime = 0.0; /* time difference between next and current */
  double t = 0.0; /* time for eventTime/speed */
  int firedEvent; 
  
  /*  event list variables: 
    *  Each event is associated with a clock, speed, and an enabled(1) or not(0) flag.
    */
  int noe = m->NumberOfEvents();
  double* clocks = new double[noe];
  int* enabledFlags = new int[noe]; 
  double* eventTimes = new double[noe];
  double* speeds = new double[noe];
  
  /* statistical array variables for each measure */
  double* stats = new double[N];
  double* curBatchMean = new double[N];
  double* preBatchMean = new double[N];
  double* autoCovariance = new double[N];
  double* avglist = new double[N];
  double* mean = new double[N];
  double* sum = new double[N];
  double error;
  double std;
  double tNormal = idfStdNormal((1+percentage)/2);
  
  //vector<autocorrelation> autocorrelationVector;
  vector<autocorrelation*> autoVector;
  autocorrelation *a;
  
  /* initialize */
  for (int j =0 ; j < noe; j++){
    clocks[j] = 0.0; 
    enabledFlags[j] = 0;
    eventTimes[j] = 0.0;
    speeds[j] = 0.0;
    }
  
  for (int l = 0; l < N; l++){
    mean[l] = 0.0;
    sum[l] = 0.0;
    avglist[l] = 0.0;
    curBatchMean[l] = 0.0;
    preBatchMean[l] = 0.0;
    autoCovariance[l] = 0.0;
    stats[l] = 0.0;
    a = new autocorrelation();
    autoVector.push_back(a);

  }
  
    sim_state* currState = m->CreateState();
  int initialSuccess = m->FillInitialState(currState);
  if (initialSuccess != 0) {
    return initialSuccess;
  }
  
  sim_state* nextState;
  
  bool flag = false;  /* general flag to indicate whether there is event enabled */
  int i = 0;
  double diffMean = 0.0; /* for grand mean */
  int index = 0;
  double preBatchTime = 0.0;
  double batchTime = 0.0; /* time difference between previous batch and current batch */
  
  /* initial scheduling  */
  for (i=0; i<noe; i++){
    if (m->IsEventEnabled(i, currState)){
      enabledFlags[i] = 1; 
      eventTimes[i] = m->GenerateEventTime(i, currState);
      speeds[i] = m->GetEventSpeed(i, currState);
      eventTimes[i]/=speeds[i];
      clocks[i] = eventTimes[i];
    }
  }
  
  for (int g = 1; g <= batchNum; g++){
    while ((++index) <= (batchSize*g)){
      firedEvent = nextEvent(clocks, noe, enabledFlags);
      if (firedEvent >= noe){ // all the events are disabled
        break;
      }
      
      firedTime = clocks[firedEvent];
      enabledFlags[firedEvent] = 0; 
      
      //fire event
      nextState = m->CreateState();
      int nextSuccess = m->FillNextState(firedEvent, currState, nextState);
      if (nextSuccess != 0) {
        return nextSuccess;
      }
      
      /* update statistics */
      int measureSuccess = m->EvaluateMeasures(currState, N, msrdata, avglist);
      if (measureSuccess != 0) {
        return measureSuccess;
      }
      
      deltaTime = firedTime - simClock;
      
      for (int j = 0; j<N; j++){
        stats[j]+=avglist[j]*deltaTime;
      }
      
      /* advance the current clock */
      simClock = firedTime;
      m->DestroyState(currState);
      currState = nextState; 
      
      //update the event list
      for (i=0; i<noe; i++){
        if (m->IsEventEnabled(i, currState)){
          flag = true;
          // disabled previously or enabled previously and fired 
          if ( enabledFlags[i] == 0){
            enabledFlags[i] = 1; 
            eventTimes[i] = m->GenerateEventTime(i, currState);
            speeds[i] = m->GetEventSpeed(i, currState);
            t = eventTimes[i]/speeds[i];
            clocks[i] = simClock + t;
          } 
          else{
            eventTimes[i] = eventTimes[i] - deltaTime*speeds[i];
            speeds[i] = m->GetEventSpeed(i, currState);
            t = eventTimes[i]/speeds[i];
            clocks[i] = simClock + t;
          }
        } //end if
        else{ //event i is not enabled in the current state
          enabledFlags[i] = 0;
        }
      }//end for
      
      if (!flag) { //no single event is enabled
        break;
      }
      flag = true;    
    } //end while
    batchTime = firedTime - preBatchTime;
    index--;
    
    if (g==1){
      for (int l=0; l<N; l++) {
        mean[l] = stats[l]/batchTime;
        a = autoVector.at(l);
        a->add(mean[l]);
        //preBatchMean[l] = mean[l];
        stats[l] = 0.0;
      }
    }
    else{
      for (int l=0; l<N; l++) {
        curBatchMean[l] = stats[l]/batchTime;
        a = autoVector.at(l);
        a->add(curBatchMean[l]);
        
        diffMean =   curBatchMean[l] - mean[l];
        sum[l]  += diffMean * diffMean * (batchTime * preBatchTime)/firedTime;
        mean[l] += (diffMean*batchTime)/firedTime;
        
        //sum[l]  += diffMean * diffMean * (g-1)/g;
        //mean[l] += diffMean/g;
        
        //autoCovariance[l] += preBatchMean[l]*curBatchMean[l];
        //preBatchMean[l] = curBatchMean[l];
        
        stats[l] = 0.0;      
      }
    }
    preBatchTime = firedTime;
  } //end for
  
  /* Compute autocorrelation for each measure */
  //for (int l=0; l<N; l++) {
  //    autoCovariance[l] = autoCovariance[l]/(batchNum-1) - (mean[l] * mean[l]);  
  //    //sum[l] = sum[l]/firedTime;   // sample variance for time averages
  //    sum[l] = sum[l]/batchNum; //sample variance for job averages
  //  }
  
  /* compute confidence interval for each measure */
    for (i=0; i<N; i++) {
    sum[i] = sum[i]/firedTime;   // sample variance for time average
    //sum[i] /= batchNum;
    std = sqrt(sum[i]);
    error = std/sqrt(double(batchNum));
    //error = std/sqrt(firingTime);
    
    /* store data into estlist */
    estlist[i].average = mean[i];
    estlist[i].variance = sum[i];
    estlist[i].samples = samples;
    estlist[i].confidence = percentage;
    estlist[i].half_width = tNormal * error;
    //estlist[i].autocorrelation = autoCovariance[i]/sum[i];
    a = autoVector.at(i);
    estlist[i].autocorrelation = a->getAutocorrelation();
    
  }
  /* release memory */
  delete [] eventTimes;
  delete [] speeds;
  delete [] avglist;
  delete [] clocks;
  delete [] enabledFlags;
  delete [] stats;
  delete [] curBatchMean;
  delete [] preBatchMean;
  delete [] autoCovariance;
  delete [] mean;
  delete [] sum;
  
  return 0;
}

int nextEvent(double* events, int n, int* flags){
  /* ---------------------------------------
  * return the index of the next event type
  * ---------------------------------------
  */
  
  int e;                                      
  int i = 0;
  
  while (flags[i] != 1)       /* find the index of the first 'active' */
    i++;                        /* element in the event list            */ 
  e = i;                        
  while (i < n) {         /* now, check the others to find which  */
    i++;                        /* event type is most imminent          */
    if ((flags[i] == 1) && (events[i] < events[e]))
      e = i;
  }
return (e);
}


