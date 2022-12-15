#ifndef SPLAY_H
#define SPLAY_H

#include "../include/defines.h"
#include "../include/shared.h"

/**
    Splay tree or list of shared objects.
    Not for super large collections (whatever fits in unsigned,
    typically 4 billion or so).
    For cleverness, we can convert back and forth to a doubly-linked
    list when there are few enough items.
*/
class splayOfShared {
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
                virtual void visit(shared_object* item) = 0;
        };
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
        splayOfShared(unsigned l2t, unsigned t2l);

        /// Destructor.
        ~splayOfShared();

        /**
            Delete all entries, and clear the tree.
        */
        void deleteAndClear();
            for (unsigned i=0; i<last_element; i++) {
                delete getItem(i);
            }
            clear();
        }

        inline unsigned numElements() const { return num_elements; }

        /** Traverse the elements, in order.
                @param  t   How to visit each item.
        */
        void traverse(tree_traversal &t) const;

        /** Find element.
                @param  key  Item to search for.
                @return null pointer, if not found;
                        an item equal to key according to Compare(), otherwise.
        */
        inline shared_object* find(const shared_object* key) {
            if (0==Splay(key))  return Item(root);
            else                return nullptr;
        }

        /** Find the "index" of an element.
                @param  key     Item to search for.
                @return -1,     if not found;
                                the index (0..size-1) of the key, otherwise.
        */
        inline long findIndex(const shared_object* key) {
            if (0==Splay(key))  return long(root)-1;
            else                return -1;
        }

        /** Add element.
                @param  key     Item to add.
                @return key,    if it was successfully added to the tree/list.
                        item,   with item->Compare(key)==0 if one
                                was already present.
                        null,   if there is no more memory to add an item.
        */
        shared_object* insert(shared_object* key);

        /** Find and remove element.
                @param  key     Target to find.
                @return item,   with item->Compare(key)==0, where item
                                has just been removed from the tree/list.
                        null,   if no such item is in the tree/list.
        */
        shared_object* remove(shared_object* key);

        /// Print the tree/list (for debugging)
        void show(std::ostream& s) const;

    private:

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
        inline shared_object*& Item(unsigned ptr)
        {
            DCASSERT(ptr);
            DCASSERT(ptr<=last_element);
            return item[ptr-1];
        }
        inline shared_object* Item(unsigned ptr) const
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

        /** Splay operation.
            Find the closest element to key in the tree/list, and make it
            the root.  Does so in a manner consistent with the underlying
            data structure (i.e., tree or list).
                @param  key  Item to search for
                @return The value of root->Compare(key).
        */
        int splay(const shared_object* key);

    private:
        /// Stack, used for tree traversals.
        static unsigned* stack;
        /// Top of stack.
        static unsigned stack_top;
        /// Size of stack (max depth).
        static unsigned stack_size;

        /// Items
        shared_object** item;
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
};

#endif
