
#ifndef TESTER_H
#define TESTER_H

#include "sim-models.h"

int HelpScreen(int argc, const char** argv);

int TestSim(int argc, const char** argv, sim_model* mdl, int N, const double* theory);

#endif
