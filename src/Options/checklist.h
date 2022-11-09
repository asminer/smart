
#ifndef CHECKLIST_H
#define CHECKLIST_H

#include "opt_enum.h"
#include "options.h"
#include "../include/splay.h"

#include <iostream>
#include <fstream>

// **************************************************************************
// *                                                                        *
// *                          checklist_enum class                          *
// *                                                                        *
// **************************************************************************

/** Constant in a checklist.
    Can be an actual item, or a virtual item (e.g., a group of items).
*/
class checklist_enum : public option_enum {
    public:
        checklist_enum(const char* n, const char* d);

        /** Called when this item is checked in a checklist.
            @return  true on success, false otherwise.
        */
        virtual bool CheckMe() = 0;

        /** Called when this item is unchecked in a checklist.
            @return  true on success, false otherwise.
        */
        virtual bool UncheckMe() = 0;

        /// Returns true iff this is a checked item in a checklist.
        virtual bool IsChecked() const = 0;
};

// **************************************************************************
// *                            checklist  items                            *
// **************************************************************************

class checklist_item : public checklist_enum {
    bool &is_set;
public:
    checklist_item(const char* n, const char* d, bool &l);
    virtual bool CheckMe();
    virtual bool UncheckMe();
    virtual bool IsChecked() const;
};

// **************************************************************************
// *                            checklist groups                            *
// **************************************************************************

class checklist_group : public checklist_enum {
    checklist_enum** items;
    unsigned num_items;
    unsigned max_items;
public:
    checklist_group(const char* n, const char* d, unsigned mi);
    virtual ~checklist_group();
    virtual bool CheckMe();
    virtual bool UncheckMe();
    virtual bool IsChecked() const;

    inline void addItem(checklist_enum* x) {
        DCASSERT(num_items < max_items);
        items[num_items] = x;
        ++num_items;
    }
};

// **************************************************************************
// *                             checkall class                             *
// **************************************************************************

/// Class to check or uncheck everything on a checklist.
class checkall : public checklist_enum {
    option* parent;
public:
    checkall(const char* n, const char* d, option* p);
    virtual bool CheckMe();
    virtual bool UncheckMe();
    virtual bool IsChecked() const;
};

// **************************************************************************
// *                                                                        *
// *                          checklist_opt  class                          *
// *                                                                        *
// **************************************************************************

class checklist_opt : public option {
    SplayOfPointers <checklist_enum>* itemlist;
    checklist_enum** possible;
    unsigned numpossible;
public:
    checklist_opt(const char* n, const char* d);
    virtual ~checklist_opt();
    virtual unsigned NumConstants() const;
    virtual option_enum* GetConstant(unsigned i) const;
    virtual void ShowHeader(OutputStream &s) const;
    virtual void ShowCurrent(OutputStream &s) const;
    // These are a bit more interesting...
    virtual option_enum* FindConstant(const char* name) const;
    virtual void ShowRange(doc_formatter* df) const;
    virtual void Finish();
    virtual bool isApropos(const doc_formatter* df, const char* keyword) const;

    virtual checklist_enum* addChecklistItem(checklist_enum* grp,
                const char* name, const char* doc, bool &link);
    /*
    virtual checklist_enum* addChecklistItem(checklist_enum* grp,
                const char* name, const char* doc, abstract_msg &m);
                */
    virtual checklist_enum* addChecklistGroup(const char* name,
                const char* doc, unsigned ni);
};


#endif

