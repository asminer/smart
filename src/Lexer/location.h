#ifndef LOCATION_H
#define LOCATION_H

#include "strings.h"

class location {
		shared_string* filename;
		unsigned linenumber;
	public:
		location();
		location(shared_string* fn,  unsigned ln);
		location(const location& L);
		void operator=(const location& L);

		~location();

		inline const char* getFile() const {
			return filename ? filename->getStr() : 0;
		}
		inline unsigned getLine() const {
			return linenumber;
		}
		inline void newline() {
			++linenumber;
		}
		
		void show(OutputStream &s) const;	
};

inline OutputStream& operator<< (OutputStream &s, const location &L)
{
	L.show(s);
	return s;
}

#endif
