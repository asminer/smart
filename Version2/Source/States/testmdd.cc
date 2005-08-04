
// $Id$

#include "mdds.h"

void smart_exit()
{
}

int main()
{
  node_manager bar;
  bar.Dump(Output);
  Output << "Later!\n";
  return 0;
}

