---
title: Help pages
---

SMART has several built-in functions.
One useful function is ```help```,
which can be used to search within SMART for documentation.
Function ```help``` takes a string parameter,
causing SMART to search for items containing that string.
For example, the program
```c
help("help");
```
might display
```

void help(string search)
    An on-line help mechanism. Searches for help topics, functions,
    options, and option constants containing the substring <search>.
    Documentation is displayed for all matches. Use the search string
    "topics" to view the available help topics. For function
    documentation, parameters between elipses ("..."s) may repeat.

void help_function(string search)
    An on-line help mechanism. Searches for functions containing the
    substring <search>. Works like "help" but displays functions only.

void help_option(string search)
    An on-line help mechanism. Searches for options and option constants
    containing the substring <search>. Works like "help" but displays
    options only.

void help_topic(string search)
    An on-line help mechanism. Searches for help topics containing the
    substring <search>. Works like "help" but displays help topics only.

```
indicating that there are built-in functions named ```help```,
```help_function```, ```help_option```, and ```help_topic```.
Equivalently,
the ```help``` function can be invoked from the command line, using
```bash
smart -h keyword
```
which is equivalent to running a program containing exactly
```c
help("keyword");
```
Running ```help``` with an empty string argument will display
*all* available documentation.
