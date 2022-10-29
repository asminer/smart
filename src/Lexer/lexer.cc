
#include "lexer.h"

#define BUFSIZE 16384
#define MAX_LEXEME 1024

// #define DEBUG_LEXER

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

lexer::buffer::buffer(FILE* inf, const char* fn, buffer* nxt)
{
    DCASSERT(inf);
    next = nxt;
    inchars = new char[BUFSIZE+1];
    pushback = 0;

    infile = inf;
    L.start(fn);

    refill();
}

lexer::buffer::~buffer()
{
    if (infile) {
        if (stdin != infile) fclose(infile);
    }
    delete[] inchars;
}

void lexer::buffer::refill()
{
    DCASSERT(infile);
    ptr = inchars;
    unsigned long got = fread(inchars, 1, BUFSIZE, infile);
    inchars[got] = 0;

    if (got) return;

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
    DCASSERT(em);

    option* debug = em->findOption("Debug");
    lexer_debug.Initialize(debug,
        "lexer",
        "When set, very low-level lexer messages are displayed.",
#ifdef DEBUG_LEXER
        true
#else
        false
#endif
    );

    topfile = 0;
    filenames = fns;
    numfiles = nfs;
    fileindex = 0;
    tlp = 0;

    report_newline = false;
    report_temporal = true;
    report_smart_keywords = true;
    report_icp_keywords = true;

    lookaheads[0].set_begin();
}

lexer::~lexer()
{
    while (topfile) {
        buffer* n = topfile->getNext();
        delete topfile;
        topfile = n;
    }
}


bool lexer::push_input(const location& from, const char* filename)
{
    if ( 0 == strcmp("-", filename) ) {
        // Special case: standard input
        if (lexer_debug.startReport()) {
            lexer_debug.report() << "opening standard input\n";
            lexer_debug.stopIO();
        }

        topfile = new buffer(stdin, "-", topfile);
        return true;
    }

    FILE* inf = fopen(filename, "r");
    if (inf) {
        if (lexer_debug.startReport()) {
            lexer_debug.report() << "opening " << filename << "\n";
            lexer_debug.stopIO();
        }
        topfile = new buffer(inf, filename, topfile);
        return true;
    }

    em->startError(from, 0);
    em->cerr() << "Couldn't open file '" << filename << "', ignoring";
    em->stopIO();
    return false;
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

            push_input(location::NOWHERE(), filenames[fileindex]);
            ++fileindex;
            continue;
        }

        int c = topfile->getc();

        //
        // Handle end of the top file
        //
        if (EOF == c) {
            if (lexer_debug.startReport()) {
                lexer_debug.report() << "End of file "
                                     << topfile->where().getFile() << "\n";
                lexer_debug.stopIO();
            }
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
                        debug_token();
                        return;

            /*
             * Single char symbols
             */
            case '#':   lookaheads[0].tokenID = token::SHARP;
                        report_newline = true;
                        debug_token();
                        return;

            case ',':   lookaheads[0].tokenID = token::COMMA;
                        debug_token();
                        return;

            case ';':   lookaheads[0].tokenID = token::SEMI;
                        debug_token();
                        return;

            case '(':   lookaheads[0].tokenID = token::LPAR;
                        debug_token();
                        return;

            case ')':   lookaheads[0].tokenID = token::RPAR;
                        debug_token();
                        return;

            case '[':   lookaheads[0].tokenID = token::LBRAK;
                        debug_token();
                        return;

            case ']':   lookaheads[0].tokenID = token::RBRAK;
                        debug_token();
                        return;

            case '{':   lookaheads[0].tokenID = token::LBRACE;
                        debug_token();
                        return;

            case '}':   lookaheads[0].tokenID = token::RBRACE;
                        debug_token();
                        return;

            case '+':   lookaheads[0].tokenID = token::PLUS;
                        debug_token();
                        return;

            case '*':   lookaheads[0].tokenID = token::TIMES;
                        debug_token();
                        return;

            case '%':   lookaheads[0].tokenID = token::MOD;
                        debug_token();
                        return;

            case '\\':  lookaheads[0].tokenID = token::SET_DIFF;
                        debug_token();
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
                        debug_token();
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
                        debug_token();
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
                        debug_token();
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
                        debug_token();
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
                        debug_token();
                        return;

            /*
             * Equals?
             */
            case '=':   if ('=' == topfile->peek()) {
                            topfile->getc();
                            lookaheads[0].tokenID = token::EQUALS;
                            debug_token();
                            return;
                        }
                        IllegalChar('=');
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
                        debug_token();
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
                        debug_token();
                        return;

            /*
             * OR
             */
            case '|':   if ('|' == topfile->peek()) {
                            topfile->getc();
                            lookaheads[0].tokenID = token::OR;
                            debug_token();
                            return;
                        }
                        IllegalChar('|');
                        continue;

            /*
             * AND
             */
            case '&':   if ('&' == topfile->peek()) {
                            topfile->getc();
                            lookaheads[0].tokenID = token::AND;
                            debug_token();
                            return;
                        }
                        IllegalChar('&');
                        continue;

            /*
             * String constants
             */
            case '"':
                        consume_strconst();
                        debug_token();
                        return;

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
                        consume_number();
                        debug_token();
                        return;


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
                        consume_ident();
                        debug_token();
                        return;

            /*
             * Illegal character.
             */
            default:
                        IllegalChar(c);
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
            if (em && em->startWarning(lookaheads[0].where, "/*")) {
                em->warn() << "Unclosed comment";
                em->stopIO();
            }
            return;
        }
    }
}

void lexer::consume_strconst()
{
    text.pop(); // remove leading "
    for (int c=0; c != '"'; ) {
        c = topfile->getc();
        if (EOF == c) {
            if (em && em->startError(lookaheads[0].where, "\"")) {
                em->cerr() << "Unclosed string";
                em->stopIO();
            }
            c = '"';
        }
        text.append(c);
    }
    text.pop(); // remove trailing "
    finish_attributed_token(token::STRCONST);
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
        finish_attributed_token(token::MODIF);
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

void lexer::IllegalChar(char c)
{
    char txt[2];
    txt[0] = c;
    txt[1] = 0;

    if (em && em->startError(lookaheads[0].where, txt)) {
        em->cerr() << "Ignoring unexpected character '" << c << "'";
        em->stopIO();
    }
}

void lexer::finish_attributed_token(token::type t)
{
    text.finish();
    lookaheads[0].attribute = new shared_string(strdup(text.get()));
    lookaheads[0].tokenID = t;

    if (text.is_truncated()) {
        if (em && em->startWarning(lookaheads[0].where, text.get())) {
            em->warn() << "Token too long; only keeping first " << MAX_LEXEME << " characters.";
            em->stopIO();
        }
    }
}

