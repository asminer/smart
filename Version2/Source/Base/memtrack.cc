
// $Id$

#include "../defines.h"
#include "memtrack.h"
#include "streams.h"

#ifdef MEM_TRACE_ON

memlog::memlog()
{
  head = NULL;
  peaksize = 0;
  currsize = 0;
  active = false;
}

memlog::~memlog()
{
  while (head) {
    logitem* next = head->next;
    delete head;
    head = next; 
  }
}

void memlog::Stop(OutputStream& report)
{
  if (!active) return;
  active = false;
  report << "Peak memory size: " << peaksize << " bytes\n";
  report << "Current memory size: " << currsize << " bytes\n";
  logitem *i;
  for (i=head; i; i=i->next) 
    if ((i->count) || (i->size)) {
      report << "\t" << i->count << " instances of " << i->item;
      report << " totaling " << i->size << " bytes\n";
    }
}

void memlog::Alloc(const char* item, int size)
{
  if (active) {
    MoveToFront(item);
    head->count++;
    head->size += size;
    currsize += size;
    peaksize = MAX(peaksize, currsize);
  }
}

void memlog::Free(const char* item, int size)
{
  if (active) {
    MoveToFront(item);
    head->count--;
    head->size -= size;
    currsize -= size;
  }
}

void memlog::MoveToFront(const char* item)
{
  logitem *prev = NULL;
  logitem *curr = head;
  while (curr) {
    if (0==strcmp(item, curr->item)) {
      if (NULL==prev) return;  // already front
      prev->next = curr->next;
      curr->next = head;
      head = curr;
      return;
    }
    prev = curr;
    curr = curr->next;
  }
  // not found, add new list item
  curr = new logitem;
  curr->item = item;
  curr->count = 0;
  curr->size = 0;
  curr->next = head;
  head = curr;
}

memlog Memory_Log;

#endif
