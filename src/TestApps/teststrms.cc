
/*
	Test of streams.
*/

#include "../Streams/streams.h"

io_environ myio;

long num[] = { 1, 12, 123, 1234, 12345, 123456, 1234567, 12345678, 123456789 };

int main()
{
  DisplayStream& cout = myio.Output;

  cout << "Using stream module\n";

  for (int i=0; i<9; i++) {
    cout.Put('|');
    cout.Put(num[i], 10);
    cout << "|\n";
  }
  for (int i=0; i<9; i++) {
    cout.Put('|');
    cout.Put(num[i], -10);
    cout << "|\n";
  }

  doc_formatter* foo = MakeTextFormatter(80, cout);
  foo->begin_heading();
  foo->Out() << "int my_function(int a)";
  foo->end_heading();
  foo->begin_indent();
  foo->Out() << "This is a very long description about this function.  ";
  foo->Out() << "Hopefully it is long enough that it requires to wrap ";
  foo->Out() << "around several lines.\nThis should be a new paragraph.\n\n";
  foo->begin_description(6);
  foo->item("first");
  foo->Out() << "Description for the first item.";
  foo->item("second");
  foo->Out() << "Description for the second item.  Let's make it long enough that it will be forced to wrap around.";
  foo->item("third");
  foo->Out() << "The third item.";
  foo->end_description();
  foo->Out() << "\nAnd, this paragraph should pick up after the description environment and wrap around as usual.  Cheers.";
  foo->end_indent();
  foo->eject_page();

  return 0;
}
