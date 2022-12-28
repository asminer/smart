
#ifndef BOGUS_H
#define BOGUS_H

#include "expr.h"

// ******************************************************************
// *                        bogus_expr class                        *
// ******************************************************************

/** Bogus expressions.
    Used for the special, compile-time expressions "error" and "default".
*/
class bogus_expr : public expr {
    const char* which;
public:
    bogus_expr(const char* w);
    virtual bool Print(std::ostream &s, int width) const;
    virtual int Compare(const shared_object* o) const;
    virtual void PrintType(std::ostream &s) const;

    // Only need one instance of these:

    static expr* makeError();
    static expr* makeDefault();
    static inline bool isError(const expr* x)   { return x == the_error; }
    static inline bool isDefault(const expr* x) { return x == the_default; }

    static inline bool orNull(expr* x) {
        if (!x) return true;
        return dynamic_cast <bogus_expr*> (x);
    }
protected:
    virtual void Traverse(traverse_data &x);
private:
    static expr* the_error;
    static expr* the_default;
};

#endif

