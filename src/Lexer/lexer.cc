#include "lexer.h"
#include "../ExprLib/exprman.h"

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

lexer::lexer(const exprman* _em, const char** fns, unsigned nfs)
    : text(MAX_LEXEME)
{
    em = _em;
    topfile = 0;
    filenames = fns;
    numfiles = nfs;
    fileindex = 0;
    tlp = 0;

    report_newline = false;
    report_temporal = true;
    report_smart_keywords = true;
    report_icp_keywords = true;

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
    lookaheads[0].type_attrib = 0;

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
        switch (text.start(c)) {
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
                        report_newline = true;
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
        finish_attributed_token(token::INTCONST);
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
            finish_attributed_token(token::INTCONST);
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

    finish_attributed_token(token::REALCONST);
}


void lexer::consume_ident()
{
    // We already have the first character.
    // Build up letters, underscores, digits
    int c;
    for (;;) {
        c = topfile->peek();
        if (('A' <= c) && (c <= 'Z')) {
            text.append(topfile->getc());
            continue;
        }
        if (('a' <= c) && (c <= 'z')) {
            text.append(topfile->getc());
            continue;
        }
        if (('0' <= c) && (c <= '9')) {
            text.append(topfile->getc());
            continue;
        }
        if ('_' == c) {
            text.append(topfile->getc());
            continue;
        }
        break;
    }
    text.finish();

    //
    // Check for boolean literals
    //
    if (text.matches("true")) {
        lookaheads[0].tokenID = token::BOOLCONST;
        lookaheads[0].bool_const = true;
        return;
    }
    if (text.matches("false")) {
        lookaheads[0].tokenID = token::BOOLCONST;
        lookaheads[0].bool_const = false;
        return;
    }

    // Check for keywords
    // Could do a binary search, but there's not very many.
    // Also, this allows us to turn off keywords more easily.
    //

    if (text.matches("in")) {
        lookaheads[0].tokenID = token::IN;
        return;
    }

    if (report_smart_keywords) {
        if (text.matches("for")) {
            lookaheads[0].tokenID = token::FOR;
            return;
        }
        if (text.matches("converge")) {
            lookaheads[0].tokenID = token::CONVERGE;
            return;
        }
        if (text.matches("guess")) {
            lookaheads[0].tokenID = token::GUESS;
            return;
        }
        if (text.matches("default")) {
            lookaheads[0].tokenID = token::DEFAULT;
            return;
        }
        if (text.matches("proc")) {
            lookaheads[0].tokenID = token::PROC;
            return;
        }
        if (text.matches("null")) {
            lookaheads[0].tokenID = token::NUL;
            return;
        }
    }

    if (report_icp_keywords) {
        if (text.matches("maximize")) {
            lookaheads[0].tokenID = token::MAXIMIZE;
            return;
        }
        if (text.matches("minimize")) {
            lookaheads[0].tokenID = token::MINIMIZE;
            return;
        }
        if (text.matches("satisfiable")) {
            lookaheads[0].tokenID = token::SATISFIABLE;
            return;
        }
    }

    if (report_temporal) if ((text.get()[0] != 0) && (text.get()[1] == 0)) {
        // it's a one character identifier.

        switch (text.get()[0]) {
            case 'A':   lookaheads[0].tokenID = token::FORALL;          return;
            case 'E':   lookaheads[0].tokenID = token::EXISTS;          return;
            case 'F':   lookaheads[0].tokenID = token::FUTURE;          return;
            case 'P':   lookaheads[0].tokenID = token::PAST;            return;
            case 'G':   lookaheads[0].tokenID = token::GLOBALLY;        return;
            case 'H':   lookaheads[0].tokenID = token::HISTORICALLY;    return;
            case 'U':   lookaheads[0].tokenID = token::UNTIL;           return;
            case 'S':   lookaheads[0].tokenID = token::SINCE;           return;
            case 'X':   lookaheads[0].tokenID = token::NEXT;            return;
            case 'Y':   lookaheads[0].tokenID = token::PREV;            return;
        }
    }

    //
    // Check for type names, modifier names.
    //
    lookaheads[0].modif_attrib = em->findModifier(text.get());
    if (lookaheads[0].modif_attrib != NO_SUCH_MODIFIER) {
        lookaheads[0].tokenID = token::MODIF;
        return;
    }
    const type* t = lookaheads[0].type_attrib = em->findOWDType(text.get());
    if (t) {
        lookaheads[0].tokenID
            = (t->isAFormalism()) ? token::FORMALISM : token::TYPE;
        return;
    }
    if (text.matches("proc")) {
        lookaheads[0].tokenID = token::PROC;
        return;
    }

    // None of the above
    finish_attributed_token(token::IDENT);
}

void lexer::finish_attributed_token(token::type t)
{
    text.finish();
    lookaheads[0].attribute = new shared_string(strdup(text.get()));
    lookaheads[0].tokenID = t;

    if (text.is_truncated()) {
        // TBD: warning message here!
    }
}

