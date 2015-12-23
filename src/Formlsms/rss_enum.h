
// $Id$

#ifndef RSS_ENUM_H
#define RSS_ENUM_H

#include "rss.h"

class model_enum;
struct LS_Vector;

class enum_reachset : public reachset {
  public:
    enum_reachset(const lldsm* p, LS_Vector &init, model_enum* ss);
    virtual ~enum_reachset();

    virtual void getNumStates(long &ns) const;
    virtual void showInternal(OutputStream &os) const;
    virtual void showState(OutputStream &os, const shared_state* st) const;
    virtual iterator& iteratorForOrder(int display_order);

  private:
    
    /**
      Iterator for discovery, natural orders
    */
    class natural_iter : public reachset::iterator {

      // required interface
      public:
        natural_iter(model_enum* ss);
        virtual ~natural_iter();
        virtual void start();
        virtual void operator++(int);
        virtual operator bool() const;
        virtual long index() const;
        virtual void copyState(shared_state* st) const;

      private:
        model_enum* states;
        long i;
    };

    /**
      Iterator for lexical order
    */
    class lexical_iter : public reachset::iterator {

      // required interface
      public:
        lexical_iter(model_enum* ss);
        virtual ~lexical_iter();
        virtual void start();
        virtual void operator++(int);
        virtual operator bool() const;
        virtual long index() const;
        virtual void copyState(shared_state* st) const;

      private:
        model_enum* states;
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
