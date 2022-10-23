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

        token lookaheads[5];
        unsigned tlp;    // token lookahead pointer
    public:
        lexer(const char** fns, unsigned nfs);
        ~lexer();

        void include_input(const char* filename);

        // What's the next token
        inline const token& peek() const {
            return lookaheads[tlp];
        }

        // Consume and discard the next token
        inline void consume() {
            if (tlp) {
                lookaheads[tlp].set_end();
                --tlp;
            } else {
                scan_token();
            }
        }

        // Consume the next token and store it
        inline void consume(token &t) {
            t = lookaheads[tlp];
            consume();
        }

        // Put back a token
        inline void unconsume(const token &t) {
            ++tlp;
            CHECK_RANGE(0, tlp, 5);
            lookaheads[tlp] = t;
        }

    private:
        lexer(const lexer&) = delete;
        void operator=(const lexer&) = delete;

    private:
        void scan_token();
};

#endif
