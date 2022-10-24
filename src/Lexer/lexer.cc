#include "lexer.h"

#include <cstring>

#define BUFSIZE 1024
#define MAX_LEXEME 1024

//
// ======================================================================
//

lexer::lexeme::lexeme(unsigned bmax) : bufmax(bmax)
{
    buffer = new char[bufmax+1];
}

lexer::lexeme::~lexeme()
{
    delete[] buffer;
}

//
// ======================================================================
//

lexer::buffer::buffer(const char* fn, buffer* nxt)
{
    next = nxt;
    inchars = new char[BUFSIZE+1];
    pushback = 0;

    L.start(fn);

    if ( strcmp("-", fn) ) {
        infile = fopen(fn, "r");
    } else {
        infile = stdin;
    }
    if (infile) refill();
}

lexer::buffer::~buffer()
{
    delete[] inchars;
}

void lexer::buffer::refill()
{
    DCASSERT(infile);
    ptr = inchars;
    unsigned long got = fread(inchars, 1, BUFSIZE, infile);
    inchars[got] = 0;
    if (feof(infile)) {
        if (stdin != infile) fclose(infile);
        infile = 0;
    }
}


//
// ======================================================================
//

lexer::lexer(const char** fns, unsigned nfs)
    : text(MAX_LEXEME)
{
    topfile = 0;
    filenames = fns;
    numfiles = nfs;
    fileindex = 0;
    tlp = 0;

    report_newline = false;
    scan_token();
}

lexer::~lexer()
{
    while (topfile) {
        buffer* n = topfile->getNext();
        delete topfile;
        topfile = n;
    }
}

void lexer::scan_token()
{
    //
    // Read from appropriate input stream, the next token
    // into lookaheads[0].
    //
    Delete(lookaheads[0].attribute);
    lookaheads[0].attribute = 0;

    for (;;) {
        //
        // If topfile is null, then advance fileindex to next in the list.
        //
        if (0==topfile) {
            if (fileindex >= numfiles) {
                // We're at the end.
                lookaheads[0].set_end();
                return;
            }
            topfile = new buffer(filenames[fileindex], topfile);
            ++fileindex;
        }

        int c = topfile->getc();

        //
        // Handle end of the top file
        //
        if (EOF == c) {
            buffer* n = topfile->getNext();
            delete topfile;
            topfile = n;
            continue;
        }

        lookaheads[0].where = topfile->where();
        text.start(c);
        switch (c) {
            //
            // Skip whitespace
            //
            case ' ':   continue;
            case '\t':  continue;
            case '\r':  continue;
            case '\n':
                        if (!report_newline) continue;
                        lookaheads[0].tokenID = token::NEWLINE;
                        report_newline = false;
                        return;

            /*
             * Single char symbols
             */
            case '#':   lookaheads[0].tokenID = token::SHARP;
                        return;

            case ',':   lookaheads[0].tokenID = token::COMMA;
                        return;

            case ';':   lookaheads[0].tokenID = token::SEMI;
                        return;

            case '(':   lookaheads[0].tokenID = token::LPAR;
                        return;

            case ')':   lookaheads[0].tokenID = token::RPAR;
                        return;

            case '[':   lookaheads[0].tokenID = token::LBRAK;
                        return;

            case ']':   lookaheads[0].tokenID = token::RBRAK;
                        return;

            case '{':   lookaheads[0].tokenID = token::LBRACE;
                        return;

            case '}':   lookaheads[0].tokenID = token::RBRACE;
                        return;

            case '+':   lookaheads[0].tokenID = token::PLUS;
                        return;

            case '*':   lookaheads[0].tokenID = token::TIMES;
                        return;

            case '%':   lookaheads[0].tokenID = token::MOD;
                        return;

            case '\\':  lookaheads[0].tokenID = token::SET_DIFF;
                        return;


            /*
             * Various comments, or division :)
             */
            case '/':
                        if ('/' == topfile->peek()) {
                            topfile->getc();
                            ignore_cpp_comment();
                            continue;
                        }
                        if ('*' == topfile->peek()) {
                            topfile->getc();
                            ignore_c_comment();
                            continue;
                        }
                        lookaheads[0].tokenID = token::DIVIDE;
                        return;

            /*
             * One or two dots
             */
            case '.':   if ('.' == topfile->peek()) {
                            topfile->getc();
                            lookaheads[0].tokenID = token::DOTDOT;
                        } else {
                            lookaheads[0].tokenID = token::DOT;
                        }
                        return;

            /*
             * Colon or gets
             */
            case ':':   if ('=' == topfile->peek()) {
                            topfile->getc();
                            lookaheads[0].tokenID = token::GETS;
                        } else {
                            lookaheads[0].tokenID = token::COLON;
                        }
                        return;

            /*
             * Bang or neq
             */
            case '!':   if ('=' == topfile->peek()) {
                            topfile->getc();
                            lookaheads[0].tokenID = token::NEQUAL;
                        } else {
                            lookaheads[0].tokenID = token::BANG;
                        }
                        return;

            /*
             * Minus or implies
             */
            case '-':   if ('>' == topfile->peek()) {
                            topfile->getc();
                            lookaheads[0].tokenID = token::IMPLIES;
                        } else {
                            lookaheads[0].tokenID = token::MINUS;
                        }
                        return;

            /*
             * Equals?
             */
            case '=':   if ('=' == topfile->peek()) {
                            topfile->getc();
                            lookaheads[0].tokenID = token::EQUALS;
                            return;
                        }
                        IllegalChar();
                        continue;

            /*
             * LT or LE
             */
            case '<':   if ('=' == topfile->peek()) {
                            topfile->getc();
                            lookaheads[0].tokenID = token::LE;
                        } else {
                            lookaheads[0].tokenID = token::LT;
                        }
                        return;

            /*
             * GT or GE
             */
            case '>':   if ('=' == topfile->peek()) {
                            topfile->getc();
                            lookaheads[0].tokenID = token::GE;
                        } else {
                            lookaheads[0].tokenID = token::GT;
                        }
                        return;

            /*
             * OR
             */
            case '|':   if ('|' == topfile->peek()) {
                            topfile->getc();
                            lookaheads[0].tokenID = token::OR;
                            return;
                        }
                        IllegalChar();
                        continue;

            /*
             * AND
             */
            case '&':   if ('&' == topfile->peek()) {
                            topfile->getc();
                            lookaheads[0].tokenID = token::AND;
                            return;
                        }
                        IllegalChar();
                        continue;

            /*
             * String constants
             */
            case '"':
                        return consume_strconst();

            /*
             * Numerical constants
             */
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                        return consume_number();


            /*
             * Identifier or keyword
             */
            case 'a':
            case 'b':
            case 'c':
            case 'd':
            case 'e':
            case 'f':
            case 'g':
            case 'h':
            case 'i':
            case 'j':
            case 'k':
            case 'l':
            case 'm':
            case 'n':
            case 'o':
            case 'p':
            case 'q':
            case 'r':
            case 's':
            case 't':
            case 'u':
            case 'v':
            case 'w':
            case 'x':
            case 'y':
            case 'z':
            case '_':
            case 'A':
            case 'B':
            case 'C':
            case 'D':
            case 'E':
            case 'F':
            case 'G':
            case 'H':
            case 'I':
            case 'J':
            case 'K':
            case 'L':
            case 'M':
            case 'N':
            case 'O':
            case 'P':
            case 'Q':
            case 'R':
            case 'S':
            case 'T':
            case 'U':
            case 'V':
            case 'W':
            case 'X':
            case 'Y':
            case 'Z':
                        return consume_ident();

            /*
             * Illegal character.
             */
            default:
                        IllegalChar();
                        continue;
        }


    }   // infinite loop
}

void lexer::ignore_cpp_comment()
{
    // We've already consumed the //
    for (;;) {
        if ('\n' == topfile->peek()) {
            return;
            /* Don't consume, in case we're in
             * a state that requires us to produce
             * newline tokens.
             */
        }
        if (EOF == topfile->getc()) {
            return;
        }
    }
}

void lexer::ignore_c_comment()
{
    // We've already consumed the /*
    // so ignore chars until */ or eof.
    // Give a warning if we hit eof before */
    for (;;) {
        int c = topfile->getc();
        while ('*' == c) {
            c = topfile->getc();
            if ('/' == c) return;
        }
        if (EOF == c) {
            // TBD warning; use lookaheads[0] location as starting point
            return;
        }
    }
}

void lexer::consume_number()
{
    int c;
    for (;;) {
        c = topfile->peek();
        if (('0' <= c) && (c <= '9')) {
            text.append(topfile->getc());
            continue;
        }
        break;
    }

    if (('.' != c) && ('e' != c) && ('E' != c)) {
        finish_token(token::INTCONST);
        return;
    }

    //
    // Special case: distinguish between .digit (consume)
    // and .. (next token!)
    //
    if ('.' == c) {
        topfile->getc();
        if ('.' == topfile->peek()) {
            // Oops, put back the first dot
            topfile->ungetc('.');
            finish_token(token::INTCONST);
            return;
        }
        text.append('.');
    }

    //
    // We're a real const.
    // Consume digits until e/E
    //
    for (;;) {
        c = topfile->peek();

        if ('e' == c) break;
        if ('E' == c) break;

        if (('0' <= c) && (c <= '9')) {
            text.append(topfile->getc());
            continue;
        }

        break;
    }

    if (('e' == c) || ('E' == c)) {
        text.append(topfile->getc());

        c = topfile->peek();
        if (('+' == c) || ('-' == c)) {
            text.append(topfile->getc());
        }

        for (;;) {
            c = topfile->peek();
            if (('0' <= c) && (c <= '9')) {
                text.append(topfile->getc());
                continue;
            }
            break;
        }
    }

    finish_token(token::REALCONST);
}


void lexer::finish_token(token::type t)
{
    text.finish();
    lookaheads[0].attribute = new shared_string(strdup(text.get()));
    lookaheads[0].tokenID = t;

    if (text.is_truncated()) {
        // TBD: warning message here!
    }
}

