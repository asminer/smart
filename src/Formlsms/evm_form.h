
// $Id$

#ifndef EVM_FORM_H
#define EVM_FORM_H

#include "../include/list.h"

class exprman;
class msr_func;

/// Initialize Event & variable formalisms.
void InitializeEVMs(exprman* em, List <msr_func> *);

#endif
