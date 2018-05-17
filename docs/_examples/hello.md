---
title: Hello world
---

## Running SMART

SMART reads input from an input text file written in the SMART language,
which is similar to C and C++ but with several significant differences.
For a simple first program, create a text file named ```hello.sm```:
```c
/*
  This is a comment 
*/
// This is a single-line comment
print("Hello, world!\n");
```


SMART can be run on one or more input files, from the command line,
by passing the input files as arguments.
To run the program above, use
```bash
smart hello.sm
```
(you may need to adjust your ```PATH```).
This should give the output
```bash
Hello, world!
```
in your terminal.


## Reading input

Now consider the program ```hello2.sm``` below:
```c
string name := read_string("Name", 50);
print("Hello, ", name, "!\n");
```
The first statement causes SMART to prompt the user to enter a string
with at most 50 characters (reading until whitespace), 
and store the result in a constant
variable called ```name```.
Technically, there are *no* variables in SMART;
instead, we have defined a function with no parameters called ```name```.
The ```print``` statement displays all of its arguments, in order.
If we run this, though, we will get something like:
```
Hello, Enter the (string, length 50) value for Name : Alan Turing
Alan!
```
This is because SMART is very lazy, and does not compute a value for
```name``` until needed (in this case, when we try to print ```name```,
after printing the string "Hello, ".).
A better version of this program would be:
```c
string name := read_string("Name", 50);
compute(name);
print("Hello, ", name, "!\n");
```
where ```compute(name)``` forces SMART to compute the value for
constant function ```name```.
If we run this program, we should obtain:
```
Enter the (string, length 50) value for Name : Alan Turing
Hello, Alan!
```
