
#include <iostream>
#include <fstream>

using namespace std;

void check(fstream &f)
{
    if (f.is_open()) {
        cout << "open\n";
    } else {
        cout << "closed\n";
    }

}

inline void showbit(const char* name, bool value)
{
    cout << name << " is " << (value ? "true" : "false") << "\n";
}

int main()
{
    fstream foo;

    check(foo);

    foo.open("hi.txt", std::fstream::in);
    showbit("eof", foo.eof());
    showbit("bad", foo.bad());
    showbit("fail", foo.fail());
    showbit("good", foo.good());
    check(foo);
    foo.close();

    foo.open("ho.txt", std::fstream::in);
    showbit("eof", foo.eof());
    showbit("bad", foo.bad());
    showbit("fail", foo.fail());
    showbit("good", foo.good());

    check(foo);

    /*
    foo.open("ho.txt", std::fstream::out);

    check(foo);

    foo << "Howdy ho!\n";
    */

    cout << "Type an integer\n";
    int x;
    cin >> x;

    cout << "You typed " << x << "\n";

    showbit("eof", cin.eof());
    showbit("bad", cin.bad());
    showbit("fail", cin.fail());
    showbit("good", cin.good());

    return 0;

}
