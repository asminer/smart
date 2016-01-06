
// $Id$

#ifndef RSS_ENUM_H
#define RSS_ENUM_H

#include "rss_indx.h"

class model_enum;

class enum_reachset : public indexed_reachset {
  public:
    enum_reachset(model_enum* ss);
    virtual ~enum_reachset();

    virtual void getNumStates(long &ns) const;
    virtual void showInternal(OutputStream &os) const;
    virtual void showState(OutputStream &os, const shared_state* st) const;
    virtual iterator& iteratorForOrder(state_lldsm::display_order ord);
    virtual iterator& easiestIterator() const;

    shared_object* getEnumeratedState(long i) const;

  private:
    
    /**
      Iterator for discovery, natural orders
    */
    class natural_iter : public reachset::iterator {

      // required interface
      public:
        natural_iter(const model_enum &ss);
        virtual ~natural_iter();
        virtual void start();
        virtual void operator++(int);
        virtual operator bool() const;
        virtual long index() const;
        virtual void copyState(shared_state* st) const;

      private:
        const model_enum &states;
        long i;
    };

    /**
      Iterator for lexical order
    */
    class lexical_iter : public reachset::iterator {

      // required interface
      public:
        lexical_iter(const model_enum &ss);
        virtual ~lexical_iter();
        virtual void start();
        virtual void operator++(int);
        virtual operator bool() const;
        virtual long index() const;
        virtual void copyState(shared_state* st) const;

      private:
        const model_enum &states;
        long* map;
        long i;
    };

  private:
    model_enum* states;
    long* state_handle;    
    iterator* natorder;
    iterator* lexorder;
};

#endif
