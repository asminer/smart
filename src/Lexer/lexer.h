#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include "location.h"

class token;

class lexer {
        class buffer {
                buffer* next;
                char* inchars;
                const char* ptr;
                location L;
                FILE* infile;
            public:
                buffer(const char* filename, buffer* nxt);
                ~buffer();

                operator bool() const {
                    return infile;
                }

                inline buffer* getNext() const { return next; }

                inline int peek() const { return infile ? *ptr : EOF; }
                inline int getc() {
                    if (infile) {
                        int ans = *(ptr++);
                        if ('\n' == ans) L.newline();
                        if (0==*ptr) refill();
                        return ans;
                    } else {
                        return EOF;
                    }
                }

                inline const location& where() const { return L; }

            private:
                buffer(const buffer&) = delete;
                void operator=(const buffer&) = delete;

                void refill();
        };
    private:
        buffer* topfile;
        const char** filenames;
        unsigned numfiles;
        unsigned fileindex;

        // pushed back tokens TBD
    public:
        lexer(const char** fns, unsigned nfs);
        ~lexer();

    private:
        lexer(const lexer&) = delete;
        void operator=(const lexer&) = delete;
};

#endif
