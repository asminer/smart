
#ifndef SPLAY_H
#define SPLAY_H

#include "defines.h"


/**
    Splay tree or list of void pointers.
    Not for super large collections (whatever fits in unsigned,
    typically 4 billion or so).
    For cleverness, we can convert back and forth to a doubly-linked
    list when there are few enough items.

    This exists to share code between
    the various derived template classes, below.

    To use, there must be a method so that
        a->Compare(DATA *b) works similar to strcmp(a, b)
*/
class splay_void {
    public:
        /**
            To traverse the tree,
            derive a class from this one,
            and override the visit() method.
        */
        class tree_traversal {
            public:
                tree_traversal();
                virtual ~tree_traversal();
                /**
                    Visit the given item.
                        @param  item    Pointer to the item
                */
                virtual void visit(void* item) = 0;
        };
    private:
        /// Stack, used for tree traversals.
        static unsigned* stack;
        /// Top of stack.
        static unsigned stack_top;
        /// Size of stack (max depth).
        static unsigned stack_size;

        /// Items
        void** item;
        /// Left pointers.
        unsigned* left;
        /// Right pointers.
        unsigned* right;
        /// Dimension of item, left, right arrays.
        unsigned max_elements;
        /// Last used element of item, left, right arrays.
        unsigned last_element;
        /// Number of elements in tree/list.
        unsigned num_elements;
        /// Pointer to free space
        unsigned free_list;
        /// Pointer to root node
        unsigned root;
        /// Element count that triggers a change from
        /// linked-list to tree, when increasing.
        unsigned list2tree;
        /// Element count that triggers a change from
        /// tree to linked-list, when decreasing.
        unsigned tree2list;
        /// Currently, are we a list?  Otherwise we are a tree.
        bool is_list;
    public:
        /** Constructor.
            Create a new, empty list/tree.
                @param  l2t Size at which we change from a list to a tree,
                            when adding elements.
                            If less than 1, we will start with a tree.

                @param  t2l Size at which we change from a tree to a list,
                            when removing elements.
                            If less than 1, we will never change back.
        */
        splay_void(unsigned l2t, unsigned t2l);

        /// Destructor.
        ~splay_void();

        inline unsigned numElements() const { return num_elements; }

        /** Traverse the elements, in order.
                @param  t   How to visit each item.
        */
        void traverse(tree_traversal &t) const;

        /**
            Remove all elements but don't deallocate memory.
        */
        void clear();

    protected:

        inline unsigned& Left(unsigned ptr)
        {
            DCASSERT(ptr);
            DCASSERT(ptr<=last_element);
            return left[ptr-1];
        }
        inline unsigned Left(unsigned ptr) const
        {
            DCASSERT(ptr);
            DCASSERT(ptr<=last_element);
            return left[ptr-1];
        }
        inline unsigned& Right(unsigned ptr)
        {
            DCASSERT(ptr);
            DCASSERT(ptr<=last_element);
            return right[ptr-1];
        }
        inline unsigned Right(unsigned ptr) const
        {
            DCASSERT(ptr);
            DCASSERT(ptr<=last_element);
            return right[ptr-1];
        }
        inline void*& Item(unsigned ptr)
        {
            DCASSERT(ptr);
            DCASSERT(ptr<=last_element);
            return item[ptr-1];
        }
        inline void* Item(unsigned ptr) const
        {
            DCASSERT(ptr);
            DCASSERT(ptr<=last_element);
            return item[ptr-1];
        }

        void Expand();

        void ConvertToTree();
        void ConvertToList();

        unsigned newNode();
        void recycleNode(unsigned x);

        static void enlargeStack();
        static inline void Push(long x) {
            if (stack_top >= stack_size) {
                enlargeStack();
            }
            stack[stack_top++] = x;
        }
        static inline long Pop() {
            return (stack_top) ? (stack[--stack_top]) : 0;
        }
        static inline void StackClear() {
            stack_top = 0;
        }
        ///
        /// swap parent and child, preserve BST property
        inline void TreeRotate(unsigned C, unsigned P, unsigned GP) {
            if (Left(P) != C) {
                Right(P) = Left(C);
                Left(C) = P;
            } else {
                Left(P) = Right(C);
                Right(C) = P;
            }
            if (GP) {
                if (Left(GP) != P) {
                    Right(GP) = C;
                } else {
                    Left(GP) = C;
                }
            }
        }

        //
        // Second half of splay, that reshapes the tree
        // after searching for the given element.
        // Unrolls the stack; thus no parameters needed.
        //
        void splay_reshape();

        //
        // Insert a new node, after a splay operation.
        //
        void insert_after_splay(void* item, int cmp);

        //
        // Remove the root node as a list
        // Return true if we succeeded.
        //
        bool remove_list_root();
};

// ==================================================================
// ||                                                              ||
// ||                                                              ||
// ||                    SplayOfPointers  class                    ||
// ||                                                              ||
// ||                                                              ||
// ==================================================================

/**  Splay tree template class.
     The tree stores pointers to objects.
     Also, for cleverness, we can convert back and forth to a doubly-linked
     list when there are few enough items.

     To use, there must be a method so that
        a->Compare(DATA *b) works similar to strcmp(a, b)

*/

template <class DATA>
class splayOfPointers : public splay_void {
    protected:
        inline DATA* dataItem(unsigned n) {
            DCASSERT(n<=last_element);
            return static_cast <DATA*> (Item(n));
        };
    public:
        inline DATA* getItem(unsigned n) const {
            return dataItem(n+1);
        };

        /**
            Delete all entries, and clear the tree.
        */
        void DeleteAndClear() {
            for (unsigned i=0; i<last_element; i++) {
                delete getItem(i);
            }
            clear();
        }


        /** Splay.
            Find the closest element to key in the tree/list, and make it
            the root.  Does so in a manner consistent with the underlying
            data structure (i.e., tree or list).
                @param  key  Item to search for
                @return The value of root->Compare(key).
        */
        template <class DATA2>
        int Splay(const DATA2* key) {
            if (!root) return -1;
            int cmp;
            if (is_list) {
                // List splay
                cmp = dataItem(root)->Compare(key);
                if (0==cmp) return 0;  // already at root.
                if (cmp > 0) {
                    // traverse to the left
                    while (Left(root)) {
                        root = Left(root);
                        cmp = dataItem(root)->Compare(key);
                        if (cmp <= 0) return cmp;
                    } // while
                    return cmp;
                }
                // traverse to the right
                while (Right(root)) {
                    root = Right(root);
                    cmp = dataItem(root)->Compare(key);
                    if (cmp >= 0) return cmp;
                } // while
                return cmp;
            }
            // Tree splay
            unsigned child = root;
            StackClear();
            while (child) {
                Push(child);
                cmp = dataItem(child)->Compare(key);
                if (0==cmp)   break;
                if (cmp > 0)  child = Left(child);
                else          child = Right(child);
            } // while child
            splay_reshape();
            return cmp;
        }

  /** Find element.
        @param  key  Item to search for.
        @return NULL, if not found;
                an item equal to key according to Compare(), otherwise.
  */
  template <class DATA2>
  inline DATA* Find(const DATA2* key) {
    int cmp = Splay(key);
    if (0==cmp)   return item[root];
    else          return 0;
  }

  /** Find the "index" of an element.
        @param  key   Item to search for.
        @return -1,   if not found;
                the index (0..size-1) of the key, otherwise.
  */
  template <class DATA2>
  inline long FindIndex(const DATA2* key) {
    int cmp = Splay(key);
    if (0==cmp)   return root;
    else          return -1;
  }

  /** Add element.
        @param  key  Item to add.
        @return key,  if it was successfully added to the tree/list.
                item, with item->Compare(key)==0 if one was already present.
                NULL, if there is no more memory to add an item.
  */
  DATA* Insert(DATA* key);

  /** Find and remove element.
        @param  key  Target to find.
        @return item, with item->Compare(key)==0, where item
                      has just been removed from the tree/list.
                0,    if no such item is in the tree/list.
  */
  DATA* Remove(DATA* key);

  /** Copy the elements, in order, into an array.
        @param  a  An array, of dimension NumElements() or larger.
  */
  void CopyToArray(DATA** a);

#ifdef DEBUG
  /** Print the tree/list (for debugging)
      There must be a method:
        Show(OutputStream& s)
      in class DATA for this to work!
  */
  void Show(OutputStream& s) {
    s << "free list: " << free_list << "\n";
    if (is_list)    s << "stored as a list\n";
    else            s << "stored as a tree\n";
    s << "root: " << root << "\n";
    s << "items:\n";
    for (long i=0; i<last_element; i++) {
      s << "\t" << i << ": ";
      if (item[i])  item[i]->Show(s);
      else          s << "null";
      s << "\n";
    }
    s << "left: ";
    s.PutArray(left, last_element);
    s << "\nright: ";
    s.PutArray(right, last_element);
    s << "\n";
  }
#endif

};

// ==================================================================
// ||                                                              ||
// ||                   SplayOfPointers  methods                   ||
// ||                                                              ||
// ==================================================================


template <class DATA>
void SplayOfPointers<DATA>::DeleteAndClear()
{
}

template <class DATA>
template <class DATA2>
int SplayOfPointers<DATA>::Splay(const DATA2* key)
{
  if (root < 0) return -1;
  int cmp;
  if (is_list) {
    // List splay
    cmp = item[root]->Compare(key);
    if (0==cmp) return 0;  // already at root.
    if (cmp > 0) {
      // traverse to the left
      while (left[root] >= 0) {
        root = left[root];
        cmp = item[root]->Compare(key);
        if (cmp <= 0) return cmp;
      } // while
      return cmp;
    }
    // traverse to the right
    while (right[root] >= 0) {
      root = right[root];
      cmp = item[root]->Compare(key);
      if (cmp >= 0) return cmp;
    } // while
    return cmp;
  }
  // Tree splay
  long child = root;
  StackClear();
  while (child >= 0) {
    Push(child);
    cmp = item[child]->Compare(key);
    if (0==cmp)   break;
    if (cmp > 0)  child = left[child];
    else          child = right[child];
  } // while child
  splay_reshape();
  return cmp;
}

template <class DATA>
DATA* SplayOfPointers<DATA>::Insert(DATA* key)
{
    if (!root) {
        insert_after_splay(key, 0);
        return key;
    }
    int cmp = Splay(key);
    if (cmp) {
        // Not found; add
        insert_after_splay(key, cmp);
        return key;
    } else {
        // Already in tree
        return Item(root);
    }
}


template <class DATA>
DATA* SplayOfPointers<DATA>::Remove(DATA* key)
{
  if (root < 0) return 0;
  int cmp = Splay(key);
  if (cmp)   return 0;

  unsigned oldroot = root;
  if (!remove_list_root()) {
    unsigned oldleft = Left(root);
    unsigned oldright = Right(root);
    if (oldleft) {
      root = oldleft;
      Splay(Item(oldroot));
      Right(root) = Right(oldroot);
    } else {
      root = Right(oldroot);
    }
  }

  DATA* tmp = Item(oldroot);
  RecycleNode(oldroot);
  return tmp;
}

template <class DATA>
void SplayOfPointers<DATA>::CopyToArray(DATA** a)
{
  if (root < 0) return;
  long i;
  long slot = 0;
  if (is_list) {
    for (i=root; left[i] >= 0; i=left[i]) { }
    for (; i>=0; i=right[i]) {
      a[slot] = item[i];
      slot++;
    }
    return;
  }
  // non-recursive, inorder tree traversal
  StackClear();
  i = root;
  while (i>=0) {
    if (left[i] >= 0) {
      Push(i);
      i =left[i];
      continue;
    }
    while (i>=0) {
      a[slot] = item[i];
      slot++;
      if (right[i] >= 0) {
        i = right[i];
        break;
      }
      i = Pop();
    } // inner while
  } // outer while
}



#endif

