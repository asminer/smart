
// $Id$

#include "api.h"

#include "../Base/options.h"

#include "flatss.h"


void InitStates()
{
  const char* hash_skip_doc = "The hash function used on states looks at the first 4 bytes of the internal, compressed representation; then, it looks at the remaining bytes.  This value specifies how many bytes to skip between observed bytes in the remaining part of the state.  If states are extremely large, then a small value here can lead to an expensive hash function; a larger value may lead to more collisions in the hash table.  The default value 0 means that the entire state is examined.";

  Hash_Skip = MakeIntOption("HashSkip", hash_skip_doc, 0, 0, 1000000000);
  AddOption(Hash_Skip);
}
