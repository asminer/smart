#ifndef LEXER_H
#define LEXER_H

#include "../Utils/location.h"
#include "../ExprLib/exprman.h"
#include "tokens.h"
#include <cstring>
#include <fstream>

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
                std::ifstream infile;
                std::istream* from; // either &infile or &cin
                buffer* next;
                location L;
            public:
                /// For reading from a file
                buffer(const char* filename, buffer* nxt);

                /// For reading from stdin
                buffer(buffer* nxt);

                ~buffer();

                inline bool isGood() const {
                    return from->good();
                }

                inline buffer* getNext() const { return next; }

                inline int peek() {
                    return from->peek();
                }
                inline int getc() {
                    int c = from->get();
                    if ('\n' == c) L.newline();
                    return c;
                }
                inline void ungetc(char c) {
                    DCASSERT('\n' != c);
                    from->unget();
                }

                inline const location& where() const { return L; }

            private:
                buffer(const buffer&) = delete;
                void operator=(const buffer&) = delete;
        };
    private:
        debugging_msg lexer_debug;

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
