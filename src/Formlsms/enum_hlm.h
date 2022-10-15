
#ifndef ENUM_HLM_H
#define ENUM_HLM_H

class hldsm;
class lldsm;

/// Make a high-level wrapper around a low-level (enumerated) model.
hldsm* MakeEnumeratedModel(lldsm* mdl);

#endif

