
// $Id$

#ifndef TAM_FORM_H
#define TAM_FORM_H

#include "../include/list.h"

class exprman;
class msr_func;

/// Initialize Tile Assembly Model formalisms.
void InitializeTAMs(exprman* em, List <msr_func> *);

#endif
