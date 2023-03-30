
#include "ordarray.h"
#include "splay.h"

class orderedShared::copier : public shared_updater {
        orderedShared &A;
        unsigned slot;
    public:
        copier(orderedShared &a) : A(a) {
            slot = 0;
        }
        virtual void update(shared_object* item) {
            DCASSERT(slot < A.num_items);
            A.items[slot++] = item;
        }
};

// **********************************************************************

orderedShared::orderedShared(const splayOfShared &S)
{
    num_items = S.numElements();
    items = num_items ? new shared_object* [num_items] : nullptr;
    copier T(*this);
    S.traverse(T);
}

orderedShared::~orderedShared()
{
    // TBD: delete each item?
    delete[] items;
}

shared_object* orderedShared::find(const shared_object* key) const
{
    //
    // Binary search
    //
    unsigned low = 0;
    unsigned high = num_items;
    while (low < high) {
        unsigned mid = (low+high)/2;
        int cmp = items[mid]->Compare(key);
        if (0==cmp) return items[mid];
        if (cmp>0) {
            high = mid;
        } else {
            low = mid+1;
        }
    }
    // not found
    return nullptr;
}

void orderedShared::traverse(shared_updater &u) const
{
    for (unsigned i=0; i<num_items; i++) {
        u.update(items[i]);
    }
}

void orderedShared::traverse(shared_visitor &v) const
{
    for (unsigned i=0; i<num_items; i++) {
        v.visit(items[i]);
    }
}

