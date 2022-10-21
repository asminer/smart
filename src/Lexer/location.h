#ifndef LOCATION_H
#define LOCATION_H

#include "strings.h"

class location {
		shared_string* filename;
		unsigned linenumber;
	public:
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
};

#endif
