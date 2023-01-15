
#include "splay.h"

unsigned* splayOfShared::stack = nullptr;
unsigned  splayOfShared::stack_top = 0;
unsigned  splayOfShared::stack_size = 0;

splayOfShared::splayOfShared(unsigned l2t, unsigned t2l)
{
    list2tree = l2t;
    tree2list = t2l;
    item = nullptr;
    left = right = nullptr;
    max_elements = num_elements = last_element = 0;
    free_list = root = 0;
    is_list = (list2tree > 0);
}

splayOfShared::~splayOfShared()
{
    free(item);
    free(left);
    free(right);
}

void splayOfShared::deleteAndClear()
{
    for (unsigned i=1; i<=last_element; i++) {
        Delete(Item(i));
        Item(i) = nullptr;
    }
    last_element = num_elements = 0;
    free_list = root = 0;
    is_list = (list2tree > 0);
}

void splayOfShared::traverse(shared_visitor &t) const
{
    if (!root) return;
    unsigned i = root;
    if (is_list) {
        for (; Left(i); i=Left(i)) { }
        for (; i; i=Right(i)) {
            t.visit(Item(i));
        }
        return;
    }
    // non-recursive, inorder tree traversal
    StackClear();
    while (i) {
        if (Left(i)) {
            Push(i);
            i = Left(i);
            continue;
        }
        while (i) {
            t.visit(Item(i));
            if (Right(i)) {
                i = Right(i);
                break;
            }
            i = Pop();
        } // inner while
    } // outer while
}

shared_object* splayOfShared::insert(shared_object* key)
{
    if (!root) {
        //
        // Empty list/tree.
        // Building the first node is the same, regardless :)
        //
        root = newNode();
        if (!root) return nullptr;
        Item(root) = key;
        Left(root) = 0;
        Right(root) = 0;
        return key;
    }

    //
    // Put closest element at the root, and check for a match
    int cmp = splay(key);
    if (0==cmp) return Item(root);

    //
    // Need to add an element.
    // First, see if it's time to convert to a tree
    if (is_list && (num_elements > list2tree)) {
        ConvertToTree();
    }

    //
    // Build new root node
    //
    unsigned newroot = newNode();
    if (!newroot) return nullptr;
    Item(newroot) = key;

    //
    // Connect new root to old root.
    //
    if (is_list) {
        // We're a doubly-linked list.
        if (cmp > 0) {
            // Add us to the left of the root
            Right(newroot) = root;
            unsigned l = Left(root);
            Left(newroot) = l;
            if (l) Right(l) = newroot;
            Left(root) = newroot;
        } else {
            // Add us to the right of the root
            Left(newroot) = root;
            unsigned r = Right(root);
            Right(newroot) = r;
            if (r) Left(r) = newroot;
            Right(root) = newroot;
        }
    } else {
        // We're a BST.
        if (cmp > 0) {
            // old root is our right child
            Right(newroot) = root;
            Left(newroot) = Left(root);
            Left(root) = 0;
        } else {
            // old root is our left child
            Left(newroot) = root;
            Right(newroot) = Right(root);
            Right(root) = 0;
        }
    }
    root = newroot;
    return key;
}


void splayOfShared::updateRoot(shared_object* old_r, shared_object* new_r)
{
    if (!root) return;  // empty tree
    if (Item(root) != old_r) return;    // root mismatch
    DCASSERT(0 == old_r->Compare(new_r));
    Item(root) = new_r;
}

shared_object* splayOfShared::remove(shared_object* key)
{
    if (!root)  return nullptr;
    int cmp = splay(key);
    if (cmp)    return nullptr;

    //
    // Item is definitely in the tree; need to remove it.
    // First, check if it's time to convert back to a list.
    //
    if (!is_list && (num_elements < tree2list)) {
        ConvertToList();
    }

    unsigned oldroot = root;
    unsigned oldleft = Left(root);
    unsigned oldright = Right(root);
    if (is_list) {
        //
        // Remove from a doubly-linked list:
        //      oldleft <-> oldroot <-> oldright
        // Root will become either oldleft or oldright.
        //
        root = 0;
        if (oldleft) {
            Right(oldleft) = oldright;
            root = oldleft;
        }
        if (oldright) {
            Left(oldright) = oldleft;
            root = oldright;
        }
    } else {
        //
        // Remove root node from the splay tree
        //
        //                  oldroot
        //                 /      \
        //           oldleft      oldright
        if (oldleft) {
            root = oldleft;
            splay(Item(oldroot));
            // Reorder oldleft tree; it will make the
            // largest element < oldroot the root,
            // meaning it is guaranteed not to have a right child.
            DCASSERT(!Right(root));
            Right(root) = oldright;
        } else {
            // no left child, root can just become the right.
            root = oldright;
        }
    }

    //
    // Tree has been reshaped.
    // Recycle the old node and return.
    //
    shared_object* found = Item(oldroot);
    recycleNode(oldroot);
    return found;
}

void splayOfShared::show(std::ostream &s) const
{
    if (is_list)    s << "Doubly-linked list\n";
    else            s << "Splay tree\n";
    s << "  root: " << root << "\n";
    s << "  free: " << free_list << "\n";
    s << "  nodes:\n";
    s << "\t-------------------------------------------\n";
    for (unsigned i=1; i<=last_element; i++) {
        s << "\tnode # " << i << "\n";
        s << "\titem : ";
        Item(i)->Print(s, 0);
        s << "\n";
        s << "\tleft : " << Left(i) << "\n";
        s << "\tright: " << Right(i) << "\n";
        s << "\t-------------------------------------------\n";
    }
}

//
// Private methods
//

void splayOfShared::Expand()
{
    unsigned new_elements = 2*max_elements;
    if (new_elements > 1024) new_elements = max_elements + 1024;
    if (new_elements < 4) new_elements = 4;

    shared_object** newitem = (shared_object**)
        realloc(item, new_elements * sizeof(shared_object*));
    unsigned* newleft = (unsigned*)
        realloc(left, new_elements * sizeof(unsigned));
    unsigned* newright = (unsigned*)
        realloc(right, new_elements * sizeof(unsigned));

    if (newitem) item = newitem;
    if (newleft) left = newleft;
    if (newright) right = newright;
    if ((0==newitem) || (0==newleft) || (0==newright)) return;
    max_elements = new_elements;
}

void splayOfShared::ConvertToTree()
{
    unsigned i;
    for (i=Left(root); i; i=Left(i)) {
        Right(i) = 0;
    }
    for (i=Right(root); i; i=Right(i)) {
        Left(i) = 0;
    }
    is_list = false;
}

void splayOfShared::ConvertToList()
{
    unsigned front = 0;
    StackClear();
    unsigned n = root;
    // "reverse" inorder traversal using the stack
    while (n) {
        if (Right(n)) {
            Push(n);
            n = Right(n);
            continue;
        }
        while (n) {
            // Visit...
            Right(n) = front;
            if (front) Left(front) = n;
            front = n;
            // ...end of visit
            if (Left(n)) {
                n = Left(n);
                break;
            }
            n = Pop();
        } // inner while
    } // outer while
}

unsigned splayOfShared::newNode()
{
    unsigned ans;
    if (free_list) {
        ans = free_list;
        free_list = Right(free_list);
    } else {
        if (last_element >= max_elements)  Expand();
        if (last_element >= max_elements)  return 0;
        ans = ++last_element;
    }
    ++num_elements;
    return ans;
}

void splayOfShared::recycleNode(unsigned x)
{
    DCASSERT(x);

    Item(x) = nullptr;
    if (x == last_element) {
        last_element--;
        while (free_list == last_element) {
            if (0==free_list) return;
            free_list = Right(free_list);
            last_element--;
        }
    } else {
        Right(x) = free_list;
        Left(x) = free_list;
        free_list = x;
    }
    --num_elements;
}

void splayOfShared::enlargeStack()
{
    stack_size += 256;
    stack = (unsigned*) realloc(stack, stack_size * sizeof(unsigned));
    DCASSERT(stack);
}

int splayOfShared::splay(const shared_object* key)
{
    if (!root) return -1;   // empty tree
    int cmp;
    if (is_list) {
        //
        // Doubly-linked list.
        // Move root pointer to correct spot in the list.
        //
        cmp = Item(root)->Compare(key);
        if (0==cmp) return 0;  // key is already at the root.
        if (cmp > 0) {
            // traverse to the left
            while (Left(root)) {
                root = Left(root);
                cmp = Item(root)->Compare(key);
                if (cmp <= 0) return cmp;
            } // while
            return cmp;
        } else {
            // traverse to the right
            while (Right(root)) {
                root = Right(root);
                cmp = Item(root)->Compare(key);
                if (cmp >= 0) return cmp;
            } // while
            return cmp;
        }
        DCASSERT(0);
    }
    //
    // Must be a splay tree.
    // Trace the search path to the key or leaf.
    //
    unsigned child = root;
    StackClear();
    while (child) {
        Push(child);
        cmp = Item(child)->Compare(key);
        if (0==cmp)   break;
        if (cmp > 0)  child = Left(child);
        else          child = Right(child);
    } // while child
    //
    // Now, re-shape the tree in a series of rotations,
    // that will bring the last node in our search
    // (match or not) to the root of the tree.
    //
    child = Pop();
    unsigned parent = Pop();
    unsigned grandp = Pop();
    unsigned greatgp = Pop();
    while (parent) {
        // splay step
        if (!grandp) {
            // we're one level away
            TreeRotate(child, parent, grandp);
            break;
        }
        if ( (Right(grandp) == parent) == (Right(parent) == child) ) {
            // parent and child are either both right children,
            // or both left children
            TreeRotate(parent, grandp, greatgp);
            TreeRotate(child, parent, greatgp);
        } else {
            TreeRotate(child, parent, grandp);
            TreeRotate(child, grandp, greatgp);
        }
        // continue up the tree
        parent = greatgp;
        grandp = Pop();
        greatgp = Pop();
    } // while parent
    root = child;
    return cmp;
}


