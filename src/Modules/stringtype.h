
// $Id$

/** \file stringtype.h

    Module for strings.
    Defines a string type, and appropriate operators on strings.
*/

#ifndef STRINGTYPE_H
#define STRINGTYPE_H

class exprman;

/** Initialize string module.
    Nice, minimalist front-end.
      @param  em  The expression manager to use.
*/
void InitStringType(exprman* em);

#endif
