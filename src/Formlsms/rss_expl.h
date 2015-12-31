
// $Id$

#ifndef RSS_EXPL_H
#define RSS_EXPL_H

#include "rss_indx.h"

// External libs
#include "statelib.h"

class expl_reachset : public indexed_reachset {
  public:
    expl_reachset(StateLib::state_db* ss);
    virtual ~expl_reachset();

    virtual void getNumStates(long &ns) const;
    virtual void showInternal(OutputStream &os) const;
    virtual void showState(OutputStream &os, const shared_state* st) const;
    virtual iterator& iteratorForOrder(int display_order);
    virtual iterator& easiestIterator() const;

    // For now: shrink the db to a more static structure
    // TBD: probably need hooks for renumbering states
    void Finish();
  
  private:

    /**
        Base class to collect common implementations
    */
    class base_iterator : public reachset::iterator {
      public:
        base_iterator();
        virtual ~base_iterator();
        virtual void start();
        virtual void operator++(int);
        virtual long index() const;
      protected:
        long i;
    };

    /**
        Base class for state_db iterators
    */
    class db_iterator : public base_iterator {
      public:
        db_iterator(const StateLib::state_db &s);
        virtual ~db_iterator();
        virtual operator bool() const;
        virtual void copyState(shared_state* st) const;
      protected:
        // Called by derived classes.
        inline void setMap(long* m) {
          DCASSERT(0==map);
          map = m;
        }
      private:
        const StateLib::state_db &states;
        long* map;
    };

    /**
        Base class for state_coll iterators
    */
    class coll_iterator : public base_iterator {

      // required interface
      public:
        coll_iterator(const StateLib::state_coll &SC, const long* SH);
        virtual ~coll_iterator();
        virtual operator bool() const;
        virtual void copyState(shared_state* st) const;
      protected:
        // Called by derived classes.
        inline void setMap(long* m) {
          DCASSERT(0==map);
          map = m;
        }

      private:
        const StateLib::state_coll &states;
        const long* state_handle;
        long* map;
    };

    
    /// Iterator for natural order, using the state_db
    class natural_db_iter : public db_iterator {
      public:
        natural_db_iter(const StateLib::state_db &s);
    };

    /// Iterator for natural order, using the state_coll
    class natural_coll_iter : public coll_iterator {
      public:
        natural_coll_iter(const StateLib::state_coll &SC, const long* SH);
    };

    /// Iterator for discovery order, using the state_coll
    class discovery_coll_iter : public coll_iterator {
      public:
        discovery_coll_iter(const StateLib::state_coll &SC, const long* SH);
    };

    /// Iterator for lexical order, using the state_db
    class lexical_db_iter : public db_iterator {
      public:
        lexical_db_iter(const hldsm* hm, const StateLib::state_db &s);
    };

    /// Iterator for lexical order, using the state_coll
    class lexical_coll_iter : public coll_iterator {
      public:
        lexical_coll_iter(const hldsm* hm, const StateLib::state_coll &SC, const long* SH);
    };

  private:
  // TBD: what do we need here?
    iterator* natorder;
    bool needs_discorder;
    iterator* discorder;
    iterator* lexorder;

    StateLib::state_db* state_dictionary;

    long* state_handle;
    StateLib::state_coll* state_collection;
};

#endif
