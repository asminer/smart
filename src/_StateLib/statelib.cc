
#include <stdio.h>

#include "statelib.h"

#include "coll.h"
#include "splaydb.h"
#include "rb_db.h"
#include "hash_db.h"

const int MAJOR_VERSION = 2;  // interface changes
const int MINOR_VERSION = 0;  // (significant) backend algorithm changes

// ******************************************************************
// *                                                                *
// *                        error  methods                          *
// *                                                                *
// ******************************************************************

const char* StateLib::error::getName() const 
{
  switch (errcode) {
    case Success:         return "Success";
    case NoMemory:        return "Out of memory";
    case Static:          return "Database is static";
    case StackOver:       return "Stack overflow";
    case BadHandle:       return "Bad handle";
    case SizeMismatch:    return "Size mismatch";
    case NotImplemented:  return "Not implemented yet!";
    default:              return "Unknown error";
  }
}

// ******************************************************************
// *                                                                *
// *                      state_coll methods                        *
// *                                                                *
// ******************************************************************

StateLib::state_coll::state_coll()
{
  numstates = 0;
}
 
StateLib::state_coll::~state_coll()
{
}


// ******************************************************************
// *                                                                *
// *                       state_db methods                         *
// *                                                                *
// ******************************************************************

StateLib::state_db::state_db()
{
  is_static = false;
  num_states = 0;
  index2handle = 0;
}
 
StateLib::state_db::~state_db()
{
  free(index2handle);
}

long* StateLib::state_db::TakeIndexMap()
{
  long* answer = index2handle;
  index2handle = 0;
  return answer;
}

// ******************************************************************
// *                                                                *
// *                       frontend functions                       *
// *                                                                *
// ******************************************************************

const char* StateLib::LibraryVersion()
{
  static char buffer[100];
  snprintf(buffer, sizeof(buffer), "State library version %d.%d", 
    MAJOR_VERSION, MINOR_VERSION);
  return buffer;

  // TBD - revision number?
}

StateLib::state_coll*  
StateLib::CreateCollection(bool useindices, bool storesize)
{
  return new main_coll(useindices, storesize);
}

StateLib::state_db*  
StateLib::CreateStateDB(state_db_type which, bool useindices, bool storesize)
{
  switch (which) {
    case SDBT_Splay:
        if (useindices)   return new splay_index_db(storesize);
        else              return new splay_handle_db(storesize);

    case SDBT_RedBlack:
        if (useindices)   return new redblack_index_db(storesize);
        else              return new redblack_handle_db(storesize);

    case SDBT_Hash:
        if (useindices)   return new hash_index_db(storesize);
        return NULL;

  }
  return NULL;
}



