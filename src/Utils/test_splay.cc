
/*
	Test of the splay tree class.
*/

#include <iostream>
#include "splay.h"
#include "strings.h"


int main()
{
    using namespace std;

    cout << "Creating splay tree of strings\n";
    cout.flush();

    splayOfShared dict(0, 0);

    cout << "Adding words\n";
    cout.flush();

    dict.insert(new shared_string("The"));
    dict.insert(new shared_string("quick"));
    dict.insert(new shared_string("brown"));
    dict.insert(new shared_string("fox"));
    dict.insert(new shared_string("jumped"));
    dict.insert(new shared_string("over"));
    dict.insert(new shared_string("the"));
    dict.insert(new shared_string("lazy"));
    dict.insert(new shared_string("dogs"));
    dict.insert(new shared_string("A"));
    dict.insert(new shared_string("man"));
    dict.insert(new shared_string("said"));

    cout << "Current representation:\n";
    cout.flush();

    dict.show(cout);

    cout << "Copying to array of strings\n";
    cout.flush();

    unsigned length = dict.numElements();
    shared_string** sorted = new shared_string* [length];

    copy_traversal <shared_string> copy(sorted, length);
    dict.traverse(copy);

    cout << "Sorted list of words:\n";
    for (unsigned i=0; i<length; i++) {
        cout << "\t" << *sorted[i] << "\n";
    }

    cout << "Done\n";
    cout.flush();
    return 0;
}
