
// $Id$

/*  
    Implementation of circular list functions.
*/

#include "../defines.h"
#include "circlist.h"

circ_node *AddElement(circ_node *tail, circ_node *element)
{
  if (NULL==element) return tail;
  DCASSERT((element->next == element) || (element->next == NULL));
  // empty list case... easy
  if (NULL==tail) {
    element->next = element;
    return element;
  }
  // new element is new tail... easy
  if (tail->index < element->index) {
    element->next = tail->next;  
    tail->next = element;
    return element;
  }
  // new element is NOT the new tail, find out where it goes
  circ_node *front = tail->next;
  circ_node *prev = tail;
  while (1) {
    if (element->index == front->index) {
      // elements must be combined somehow, beyond our ability.
      return NULL;
    }
    if (element->index < front->index) {
      prev->next = element;
      element->next = front;
      return tail;
    }
    prev = front;
    front = front->next;
    DCASSERT(prev!=tail); // we've done a whole circuit! impossible!
  }
  // can we get here?
  return NULL;
}

circ_node *FindIndex(circ_node *tail, int key)
{
  if (NULL==tail) return NULL;  // empty list, not found
  if (tail->index < key) return NULL; 
  if (tail->index == key) return tail;
  circ_node *front = tail->next;
  while (1) {
    if (key == front->index) {
      return front;
    }
    if (key < front->index) {
      return NULL;
    }
    DCASSERT(front != tail);  // should be impossible
    front = front->next;
  }
  // can't get here
  return NULL;
}

int ListLength(circ_node *tail)
{
  if (NULL==tail) return 0;
  circ_node *front = tail->next;
  int answer = 0;
  while (1) {
    answer++;
    if (front==tail) return answer;
    front = front->next;
  }
  // keep compiler happy
  return -1;
}
