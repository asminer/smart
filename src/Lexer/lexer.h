#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include "../Streams/location.h"
#include "../ExprLib/exprman.h"
#include "tokens.h"
#include <cstring>

/*
 * Brilliantly-designed, perfect in every way,
 * self-contained lexer class for splitting the input stream(s)
 * into tokens for use by the parser(s).
 */
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

                inline char pop() {
                    if (p) return buffer[--p];
                    return 0;
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

                inline bool matches(const char* text) const {
                    return (0==strcmp(buffer, text));
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
                buffer(FILE* inf, const char* filename, buffer* nxt);
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
        named_msg lexer_debug;

        const exprman* em;
        const char** filenames;
        unsigned numfiles;
        unsigned fileindex;

        buffer* topfile;

        token lookaheads[5];
        unsigned tlp;    // token lookahead pointer

        lexeme text;

        bool report_newline;
        bool report_temporal;
        bool report_smart_keywords;
        bool report_icp_keywords;
    public:
        /**
         * Initialize.
         *  @param  em      exprman for types vs idents and error streams
         *  @param  fns     Input file names given on command line
         *  @param  nfs     Number of input file names
         */
        lexer(const exprman *_em, const char** fns, unsigned nfs);

        // Cleanup
        ~lexer();

        /**
         * Immediately start processing another file, for example
         * due to an #include directive.
         *  @param  from        Location for error message, if any
         *  @param  filename    Name of file to start processing
         *
         *  @return true on success, false on failure (couldn't open)
         */
        bool push_input(const location& from, const char* filename);

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

        // Consume the next token and store it in t
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

        // Turn on debugging
        inline void debug_on() {
            lexer_debug.Activate();
        }

        // Turn off debugging
        inline void debug_off() {
            lexer_debug.Deactivate();
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

        void IllegalChar(char c);
        void finish_attributed_token(token::type t);

        inline void debug_token() {
            if (lexer_debug.startReport()) {
                lookaheads[0].debug(lexer_debug.report());
                lexer_debug.report() << "\n";
                lexer_debug.stopIO();
            }
        }
};

#endif
