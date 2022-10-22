
#include "location.h"
#include "../Streams/streams.h"

location::location()
{
	filename = 0;
	linenumber = 0;
}

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
	if (filename != L.filename) {
		Delete(filename);
		filename = Share(L.filename);
	}
	linenumber = L.linenumber;
}

location::~location()
{
	Delete(filename);
}

void location::show(OutputStream &s) const
{
	s.PutFile( filename ? filename->getStr() : 0, linenumber );
}
