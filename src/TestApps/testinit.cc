
/*
  Test the clever initializer class :^)
*/

#include "../ExprLib/startup.h"

#include <cstdio>

class myinita : public initializer {
  public:
    myinita();
    virtual bool execute();
};

myinita::myinita() : initializer("myinita")
{
  buildsResource("a");
}

bool myinita::execute()
{
  printf("Executed myinita\n");
  return true;
}

//============================================================

class myinitb : public initializer {
  public:
    myinitb();
    virtual bool execute();
};

myinitb::myinitb() : initializer("myinitb")
{
  usesResource("b");
}

bool myinitb::execute()
{
  printf("Executed myinitb\n");
  return true;
}

//============================================================

class myinitc : public initializer {
  public:
    myinitc();
    virtual bool execute();
};

myinitc::myinitc() : initializer("myinitc")
{
  usesResource("a");
  buildsResource("b");
}

bool myinitc::execute()
{
  printf("Executed myinitc\n");
  return true;
}

//============================================================

myinita foo1;
myinitb foo2;
myinitc foo3;

int main()
{
  printf("Starting...\n");

  bool ok = initializer::executeAll();

  printf("Done\n");
  if (ok) printf("  No deadlock\n");
  else    printf("  Deadlock!\n");

  return 0;
}
