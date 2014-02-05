
// $Id$

#ifndef DCP_FORM_H
#define DCP_FORM_H

#include "../include/list.h"

class exprman;
class msr_func;

/// Initialize "discrete constraint program" formalisms.
void InitializeDCPs(exprman* em, List <msr_func> *);

#endif
