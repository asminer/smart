
#include "splay.h"


unsigned* splay_void::stack = nullptr;
unsigned  splay_void::stack_top = 0;
unsigned  splay_void::stack_size = 0;

splay_void::tree_traversal::tree_traversal()
{
}

splay_void::tree_traversal::~tree_traversal()
{
}

splay_void::splay_void(unsigned l2t, unsigned t2l)
{
    list2tree = l2t;
    tree2list = t2l;
    item = nullptr;
    left = right = nullptr;
    max_elements = num_elements = last_element = 0;
    free_list = root = 0;
    is_list = (list2tree > 0);
}

splay_void::~splay_void()
{
    free(item);
    free(left);
    free(right);
}

void splay_void::traverse(tree_traversal &t)
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

void splay_void::clear()
{
    for (unsigned i=1; i<=last_element; i++) {
        Item(i) = nullptr;
    }
    last_element = num_elements = 0;
    free_list = root = 0;
    is_list = (list2tree > 0);
}

//
// Protected methods
//

void splay_void::Expand()
{
    unsigned new_elements = 2*max_elements;
    if (new_elements > 1024) new_elements = max_elements + 1024;
    if (new_elements < 4) new_elements = 4;
    void** newitem = (void**) realloc(item, new_elements * sizeof(DATA*));
    unsigned* newleft = (unsigned*) realloc(left, new_elements * sizeof(unsigned));
    unsigned* newright = (unsigned*) realloc(right, new_elements * sizeof(unsigned));
    if (newitem) item = newitem;
    if (newleft) left = newleft;
    if (newright) right = newright;
    if ((0==newitem) || (0==newleft) || (0==newright)) return;
    max_elements = new_elements;
}

void splay_void::ConvertToTree()
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

void splay_void::ConvertToList()
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

unsigned splay_void::newNode()
{
    unsigned ans;
    if (free_list) {
        ans = free_list;
        free_list = Right(free_list);
    } else {
        if (last_element >= max_elements)  Expand();
        if (last_element >= max_elements)  {
            throw "out of memory"
        }
        ans = ++last_element;
    }
    ++num_elements;
    return ans;
}

void splay_void::recycleNode(unsigned x)
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

void splay_void::enlargeStack()
{
    stack_size += 256;
    stack = (unsigned*) realloc(stack, stack_size * sizeof(unsigned));
    DCASSERT(stack);
}

void splay_void::splay_reshape()
{
    unsigned child = Pop();
    unsigned parent = Pop();
    unsigned grandp = Pop();
    unsigned greatgp = Pop();
    while (parent) {
        // splay step
        if (!grandp) {
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
        // continue
        parent = greatgp;
        grandp = Pop();
        greatgp = Pop();
    } // while parent
    root = child;
}


void splay_void::insert_after_splay(void* item, int cmp)
{
    if (!root) {
        //
        // Empty tree/list.
        // Behavior is the same for both!
        //
        root = NewNode();
        Item(root) = item;
        Left(root) = 0;
        Right(root) = 0;
        return;
    }

    //
    // Non-empty tree/list.
    // First, check if we've reached the threshold
    // for conversion to a tree.
    //
    if (is_list && (num_elements > list2tree)) {
        ConvertToTree();
    }

    //
    // Build new root node
    //

    unsigned newroot = NewNode();
    Item(newroot) = item;

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
}

bool splay_void::remove_list_root()
{
    //
    // Check if we've shrunk enough to convert back to a list
    //
    if (!is_list && (num_elements < tree2list)) {
        ConvertToList();
    }
    if (!is_list) return false;

    // Remove from doubly-linked list

    unsigned oldroot = root;
    unsigned oldleft = Left(root);
    unsigned oldright = Right(root);

    root = 0;
    if (oldleft) {
        Right(oldleft) = oldright;
        root = oldleft;
    }
    if (oldright) {
        Left(oldright) = oldleft;
        root = oldright;
    }
    return true;
}

