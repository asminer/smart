
// $Id$

/*  
    Proper circular linked lists, sorted by "index".
    To deal with more data, derive a class from it (e.g., circ_node_data)

    REMEMBER: circular lists, so next of tail element is the front of the list
*/

#ifndef CIRCLIST_H
#define CIRCLIST_H


struct circ_node {
  /// value to sort by
  int index;  
  /// next in the chain
  circ_node* next;
};

template <class DATA>
struct circ_node_data : public circ_node {
  DATA value;
};

circ_node *AddElement(circ_node *tail, circ_node *element);
circ_node *FindIndex(circ_node *tail, int key);
int ListLength(circ_node *tail);

#endif
