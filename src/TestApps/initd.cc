
// $Id:$

#include "../ExprLib/startup.h"

#include <cstdio>

class myinitd : public initializer {
  public:
    myinitd();
    virtual bool execute();
};

myinitd::myinitd() : initializer("myinitd")
{
  usesResource("d");
  buildsResource("d");
}

bool myinitd::execute()
{
  printf("This should deadlock!\n");
  return true;
}

myinitd foo4;
