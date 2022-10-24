#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include "location.h"
#include "tokens.h"

class lexer {
    private:
        class lexeme {
                char* buffer;
                const unsigned bufmax;
                unsigned p;
                bool truncated;
            public:
                lexeme(unsigned bmax);
                ~lexeme();

                inline char start(char c) {
                    truncated = false;
                    p = 1;
                    return buffer[0] = c;
                }

                inline void append(char c) {
                    if (p < bufmax) {
                        buffer[p++] = c;
                    } else {
                        truncated = true;
                    }
                }

                inline void finish() {
                    buffer[p] = 0;
                }

                inline bool is_truncated() const {
                    return truncated;
                }

                inline const char* get() const {
                    return buffer;
                }
            private:
                lexeme(const lexeme&) = delete;
                void operator=(const lexeme&) = delete;
        };
    private:
        class buffer {
                buffer* next;
                char pushback;
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

                inline int peek() const {
                    return pushback ? pushback :
                        (infile ? *ptr : EOF);
                }
                inline int getc() {
                    int ans;
                    if (pushback) {
                        ans = pushback;
                        pushback = 0;
                        return ans;
                    }
                    if (infile) {
                        ans = *(ptr++);
                        if ('\n' == ans) L.newline();
                        if (0==*ptr) refill();
                        return ans;
                    } else {
                        return EOF;
                    }
                }
                inline void ungetc(char c) {
                    DCASSERT(0==pushback);
                    pushback = c;
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

        lexeme text;

        bool report_newline;
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

        void ignore_cpp_comment();
        void ignore_c_comment();

        void consume_strconst();
        void consume_number();
        void consume_ident();

        void IllegalChar();
        void finish_token(token::type t);
};

#endif
