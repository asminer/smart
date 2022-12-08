
#include "../ExprLib/startup.h"

#include <cstdio>

class myinitd : public startup {
  public:
    myinitd();
    virtual bool execute();
};

myinitd::myinitd() : startup("myinitd")
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
