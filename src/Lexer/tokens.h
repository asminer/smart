#ifndef TOKENS_H
#define TOKENS_H

#include "location.h"

/**
 * Self-contained token from an input file.
 * Produced by the lexer.  Consumed by the parser.
 */
class token {
    public:
        enum type {
            //
            // Single characters
            //
            END             = 0,
            NEWLINE         = '\n',
            SHARP           = '#',
            COMMA           = ',',
            DOT             = '.',
            SEMI            = ';',
            LPAR            = '(',
            RPAR            = ')',
            LBRAK           = '[',
            RBRAK           = ']',
            LBRACE          = '{',
            RBRACE          = '}',
            GT              = '>',
            LT              = '<',
            PLUS            = '+',
            MINUS           = '-',
            TIMES           = '*',
            DIVIDE          = '/',
            MOD             = '%',
            COLON           = ':',
            QUEST           = '?',
            BANG            = '!',

            FORALL          = 'A',
            EXISTS          = 'E',
            FUTURE          = 'F',
            PAST            = 'P',
            GLOBALLY        = 'G',
            HISTORICALLY    = 'H',
            UNTIL           = 'U',
            SINCE           = 'S',
            NEXT            = 'X',
            PREV            = 'Y',

            //
            // Stuff with attributes
            //
            BOOLCONST   = 300,
            INTCONST    = 301,
            REALCONST   = 302,
            STRCONST    = 303,
            TYPE        = 304,
            MODIF       = 305,
            IDENT       = 306,

            //
            // Symbols
            //
            GETS        = 350,
            EQUALS      = 351,
            NEQUAL      = 352,
            GE          = 353,
            LE          = 354,
            OR          = 355,
            AND         = 356,
            SET_DIFF    = 357,
            IMPLIES     = 358,
            DOTDOT      = 359,

            //
            // Keywords (Smart)
            //

            IN          = 450,
            FOR         = 451,
            CONVERGE    = 452,
            GUESS       = 453,
            DEFAULT     = 454,
            PROC        = 455,
            NUL         = 456,

            //
            // Keywords (ICP)
            //
            MAXIMIZE    = 500,
            MINIMIZE    = 501,
            SATISFIABLE = 502
		};
    private:
        location where;
        type tokenID;
        shared_string* attribute;

    public:
        token();		/* Zero out everything */
        token(const token&);
        void operator=(const token&);

        ~token();		/* unlink shared stuff */

        inline bool matches(type t) const {	return t == tokenID; }
        inline type getId() const { return tokenID; }
        inline const location& getLoc() const { return where; }
        inline const char* getAttr() const {
            return attribute ? attribute->getStr() : 0;
        }
        inline shared_string* shareAttr() {
            return Share(attribute);
        }

        // Set token to END; unlink attribute
        void set_end();

        // Show the actual text (lexeme)
        void show(OutputStream &s) const;

        // Convert tokenID into a string
        const char* getIdName() const;

        friend class lexer;
};

inline OutputStream& operator<< (OutputStream& s, const token& t)
{
    t.show(s);
    return s;
}

#endif
