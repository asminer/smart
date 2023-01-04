#ifndef ORDARRAY_H
#define ORDARRAY_H

#include "../include/shared.h"

class splayOfShared;

class orderedShared {
    public:
        orderedShared(const splayOfShared &S);
        ~orderedShared();
        shared_object* find(const shared_object* key) const;
        void traverse(shared_visitor &v) const;

        inline unsigned numElements() const { return num_items; }
        inline const shared_object* get(unsigned i) const {
            DCASSERT(i<num_items);
            return items[i];
        }
        inline shared_object* get(unsigned i) {
            DCASSERT(i<num_items);
            return items[i];
        }

    private:
        shared_object** items;
        unsigned num_items;

        class copier;
        friend class copier;
};


template <class SHARED>
class orderedArray : public orderedShared {
    public:
        orderedArray(const splayOfShared &S) : orderedShared(S) { }
        inline const SHARED* get(unsigned i) const {
            return dynamic_cast <SHARED*> (orderedShared::get(i));
        }
        inline SHARED* get(unsigned i) {
            return dynamic_cast <SHARED*> (orderedShared::get(i));
        }
        inline SHARED* find(const shared_object* key) const {
            return dynamic_cast <SHARED*> (orderedShared::find(key));
        }
};

#endif
