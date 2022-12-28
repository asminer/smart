
#include "bogus.h"
// #include "exprman.h"

// ******************************************************************
// *                       bogus_expr methods                       *
// ******************************************************************

expr* bogus_expr::the_error = nullptr;
expr* bogus_expr::the_default = nullptr;

bogus_expr::bogus_expr(const char* w)
    : expr( location::NOWHERE(), (typelist*) 0)
{
    which = w;
}

bool bogus_expr::Print(std::ostream &s, int width) const
{
    s << which;
    return (which[0]!=0);
}

void bogus_expr::PrintType(std::ostream &s) const
{
    s << which;
}

int bogus_expr::Compare(const shared_object* o) const
{
    if (o==this) return 0;
    const bogus_expr* foo = dynamic_cast <const bogus_expr*> (o);
    if (!foo) return 1;
    if (!which) {
        if (!foo->which) return 0;
        return -1;
    }
    return strcmp(which, foo->which);
}

expr* bogus_expr::makeError()
{
    if (!the_error) {
        the_error = new bogus_expr("error");
    }
    Share(the_error);
    return the_error;
}

expr* bogus_expr::makeDefault()
{
    if (!the_default) {
        the_default = new bogus_expr("default");
    }
    Share(the_default);
    return the_default;
}


void bogus_expr::Traverse(traverse_data &x)
{
    internal_error E(__FILE__, __LINE__);
    E << "Trying to traverse the " << which << " expression";
}


