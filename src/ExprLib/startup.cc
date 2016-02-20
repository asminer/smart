
// $Id$

#include "startup.h"
#include <string.h>

// ******************************************************************

initializer::resource* initializer::resource_list = 0;
initializer* initializer::waiting_list = 0;
initializer* initializer::completed_list = 0;
initializer* initializer::failed_list = 0;

exprman* initializer::em = 0;
symbol_table* initializer::st = 0;
const char** initializer::env = 0;
const char* initializer::version = 0;
List <msr_func>* initializer::CML = 0;

// ******************************************************************
// *                                                                *
// *                  initializer::resource  class                  *
// *                                                                *
// ******************************************************************

class initializer::resource {
    const char* name;
    List <initializer> builders;
    bool ready;
  public:
    resource* next;
  public: 
    resource(const char* n);
    bool isReady();
    void addBuilder(initializer* b);

    friend class initializer;
};

// ******************************************************************
// *                 initializer::resource  methods                 *
// ******************************************************************

initializer::resource::resource(const char* n)
{
  name = n;
  next = 0;
  ready = false;
}

bool initializer::resource::isReady()
{
  if (ready) return true;
  for (int i=0; i<builders.Length(); i++) {
    if (builders.ReadItem(i)->hasExecuted()) continue;
    return false;
  }
  ready = true;
  return true;
}

void initializer::resource::addBuilder(initializer *b)
{
  builders.Append(b);
}

// ******************************************************************
// *                                                                *
// *                      initializer  methods                      *
// *                                                                *
// ******************************************************************

initializer::initializer(const char* n)
{
  name = n;
  next = waiting_list;
  waiting_list = this;
  executed = false;
}

initializer::~initializer()
{
}

bool initializer::isReady()
{
  for (int i=0; i<resources_used.Length(); i++) {
    if (resources_used.Item(i)->isReady()) continue;
    return false;
  }
  return true;
}

bool initializer::executeAll()
{
  while (waiting_list) {
    int ran = executeWaiting();
    if (0==ran) return false; // STUCK!
  }
  return true;
}

int initializer::executeWaiting()
{
  int count = 0;
  initializer* run_list = waiting_list;
  waiting_list = 0;

  while (run_list) {
    initializer* next = run_list->next;
    if (run_list->isReady()) {
      count++;
      bool ok = run_list->execute();
      run_list->executed = true;
      if (ok) {
        run_list->next = completed_list;
        completed_list = run_list;
      } else {
        run_list->next = failed_list;
        failed_list = run_list;
      }
    } else {
      run_list->next = waiting_list;
      waiting_list = run_list;
    }
    run_list = next;
  }
  return count;
}

initializer::resource* initializer::findResource(const char* name)
{
  // traverse list, find item with name, and move it to front
  // otherwise, if not present, create a new entry in front

  resource* prev = 0;
  resource* curr = resource_list;

  while (curr) {

    if (0==strcmp(name, curr->name)) {  // found!
      if (prev) {
        // Not at front; move it there
        prev->next = curr->next;
        curr->next = resource_list;
        resource_list = curr;
      }
      return resource_list;
    }
    prev = curr;
    curr = curr->next;
  }

  // still here?  Wasn't found

  curr = new resource(name);
  curr->next = resource_list;
  resource_list = curr;
  return resource_list;
}

// ******************************************************************

void initializer::buildsResource(const char* name)
{
  resource* r = findResource(name);
  DCASSERT(r);
  r->addBuilder(this);
}

void initializer::usesResource(const char* name)
{
  resources_used.Append(findResource(name));
}

