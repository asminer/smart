
#include "init_data.h"
#include "../Options/options.h"
#include "../Options/optman.h"
#include "../Utils/strings.h"
#include "../Utils/initializer.h"

#include "exprman.h"
#include "type.h"
#include "casting.h"
#include "sets.h"
#include "intervals.h"

#include "ops_bool.h"
#include "ops_int.h"
#include "ops_real.h"
#include "ops_set.h"
#include "ops_misc.h"

#include <errno.h>

//#define REQUIRES_PRECOMPUTING

// #define DEBUG_REAL_TYPE

// ******************************************************************
// *                                                                *
// *                                                                *
// *                     Promotions and casting                     *
// *                                                                *
// *                                                                *
// ******************************************************************


// ******************************************************************
// *                                                                *
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// *                                                                *
// ******************************************************************

void InitTypes(exprman* em)
{
  if (0==em)  return;

  // Operators
  InitBooleanOps(em);
  InitIntegerOps(em);
  InitRealOps(em);
  InitSetOps(em);
  InitMiscOps(em);
}

