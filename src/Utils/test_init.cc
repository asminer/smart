
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

myinit foo1("foo1", "a");
myinit foo2("foo2", "b");
myinit foo3("foo3", "c", "a", "b");

int main()
{
    std::cout << "Starting...\n";

    initializer::execute_all(true);

    std::cout << "Done!\n";
    return 0;
}
