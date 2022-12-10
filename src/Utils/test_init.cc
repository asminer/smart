
/*
  Test the clever initializer class :^)
*/

#include "initializer.h"

#include <cstring>

class myinit : public initializer {
    public:
        myinit(const char* name, const char* b=nullptr,
                const char* n1=nullptr, const char* n2=nullptr);
        virtual void execute();
};

myinit::myinit(const char* name, const char* b, const char* n1, const char* n2)
    : initializer(name, 1, 2)
{
    if (b) {
        builds_resource(b);
    }

    if (n1) {
        needs_resource(n1);
    }

    if (n2) {
        needs_resource(n2);
    }
}

void myinit::execute()
{
    std::cout << "Executing " << name << "\n";
}

//============================================================
// Build an interesting dependency graph

myinit A1("A1", "a");
myinit A2("A2", "a");
myinit B("B", "b");
myinit C1("C1", "c");
myinit C2("C2", "c");
myinit C3("C3", "c");
myinit D("D", "d");
myinit E("E", "e");
myinit F("F", "f");
myinit G("G", "g");

myinit H1("H1", "h", "a", "b");
myinit H2("H2", "h");

myinit I("I", "i", "c", "d");
myinit J("J", "j", "h", "i");
myinit K("K", "k", "j", "e");
myinit L1("L1", "l", "k", "f");
myinit L2("L2", "l", "a");
myinit L3("L3", "l");
myinit M("M", "m", "l", "g");

// Deadlock cycle

myinit X("X", "x", "a", "z");
myinit Y("Y", "y", "b", "x");
myinit Z("Z", "z", "c", "y");

int main()
{
    std::cout << "Starting...\n";

    initializer::execute_all(true);

    std::cout << "Done!\n";
    return 0;
}
