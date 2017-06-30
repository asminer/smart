
#ifndef RSS_ENUM_H
#define RSS_ENUM_H

#include "rss_indx.h"

class model_enum;

class enum_reachset : public indexed_reachset {
  public:
    enum_reachset(model_enum* ss);
    virtual ~enum_reachset();
    virtual const char* getClassName() const {
      return "enum_reachset";
    }

    virtual void getNumStates(long &ns) const;
    virtual void showInternal(OutputStream &os) const;
    virtual void showState(OutputStream &os, const shared_state* st) const;
    virtual iterator& iteratorForOrder(state_lldsm::display_order ord);
    virtual iterator& easiestIterator() const;

    shared_object* getEnumeratedState(long i) const;

    virtual void Renumber(const GraphLib::node_renumberer* Ren);

  private:
    
    /**
      Iterator for natural orders
    */
    class natural_iter : public indexed_iterator {
      // required interface
      public:
        natural_iter(const model_enum &ss);
        virtual ~natural_iter();
        virtual void copyState(shared_state* st, long o) const;

      private:
        const model_enum &states;
    };

    /**
      Iterator for lexical order
    */
    class lexical_iter : public natural_iter {
      // required interface
      public:
        lexical_iter(const model_enum &ss);
        virtual ~lexical_iter();
    };

    /**
      Iterator for discovery order
    */
    class discovery_iter : public natural_iter {
      // required interface
      public:
        discovery_iter(const model_enum &ss);
        virtual ~discovery_iter();
    };

  private:
    model_enum* states;
    long* state_handle;    
    iterator* natorder;
    iterator* lexorder;
    iterator* discorder;
};

#endif
