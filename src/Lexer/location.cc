
#include "location.h"

location::location(shared_string* fn,  unsigned ln)
{
	filename = Share(fn);
	linenumber = ln;
}
	
location::location(const location& L)
{
	filename = Share(L.filename);
	linenumber = L.linenumber;
}

void location::operator=(const location& L)
{
	if (this != &L) {
		Delete(filename);
		filename = Share(L.filename);
		linenumber = L.linenumber;
	}
}

location::~location()
{
	Delete(filename);
}


