
#ifndef HELP_H
#define HELP_H

#include "symbols.h"

class function;

/**   Help topic.
      Derive a class from this one, and override the
      PrintDocs() method, to build a new help topic.
*/
class help_topic : public symbol {
        const char* summary;
    public:
        help_topic(const char* topic_name, const char* summary);
        help_topic();
    protected:
        virtual ~help_topic();
        void setName(char* name);
        void setName(const std::string &str);
        inline void setSummary(const char* sum) {
            DCASSERT(0==summary);
            summary = sum;
        }
    public:
        virtual bool DocumentHeader(doc_formatter &df) const;
        inline const char* Summary() const { return summary; }
};

/**
    A help topic that consists of documentation,
    and lists of symbols and/or options.
*/
class help_group : public help_topic {
        const char* docs;
        List <function> funcs;
        List <option> options;
    public:
        help_group(const char* name, const char* summary, const char* docs);
    protected:
        virtual ~help_group();
    public:
        virtual void PrintDocs(doc_formatter &df, const char* keyword) const;

        inline void addFunction(function* s) {
            if (s) funcs.Append(s);
        }
        inline void addOption(option* o) {
            if (o) options.Append(o);
        }
};

#endif
