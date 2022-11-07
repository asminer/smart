
#ifndef RADIO_OPT_H
#define RADIO_OPT_H

#include "options.h"
#include "opt_enum.h"

// **************************************************************************
// *                                                                        *
// *                           radio_button class                           *
// *                                                                        *
// **************************************************************************

/** Radio button.
    If a "customized" radio button option is needed, derive the radio
    button items from this class and provide an AssignToMe()
    method.
*/
class radio_button : public option_enum {
    public:
        radio_button(const char* n, const char* d, unsigned ndx);

        inline unsigned getIndex() const { return index; }

        /** Called when this radio button is selected.
            @return  true on success, false otherwise.
        */
        virtual bool AssignToMe();

    private:
        unsigned index;
};


// **************************************************************************
// *                            radio_opt  class                            *
// **************************************************************************

class radio_opt : public option {
        unsigned& which;
        radio_button** possible;
        unsigned numpossible;
        unsigned numadded;
    public:
        radio_opt(const char* n, const char* d, unsigned np, unsigned& w);
        virtual ~radio_opt();
        virtual error SetValue(option_enum* v);
        virtual option_enum* FindConstant(const char* name) const;
        virtual unsigned NumConstants() const;
        virtual option_enum* GetConstant(unsigned i) const;

        virtual void Finish();

        virtual void ShowHeader(OutputStream &s) const;
        virtual void ShowRange(doc_formatter* df) const;
        virtual bool isApropos(const doc_formatter* df, const char* keyword) const;
        virtual void RecurseDocs(doc_formatter* df, const char* keyword) const;

        virtual radio_button* addRadioButton(const char* n, const char* d, unsigned ndx);
};

#endif

