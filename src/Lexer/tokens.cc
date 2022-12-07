
#include "tokens.h"

token::token()
{
    tokenID = END;
    attribute = 0;
    type_attrib = 0;
}

token::token(const token &T)
{
    where = T.where;
    tokenID = T.tokenID;
    attribute = Share(T.attribute);
    type_attrib = T.type_attrib;
    bool_const = T.bool_const;
}

void token::operator=(const token& T)
{
    if (attribute != T.attribute) {
        Delete(attribute);
        attribute = Share(T.attribute);
    }
    where = T.where;
    tokenID = T.tokenID;
    type_attrib = T.type_attrib;
    bool_const = T.bool_const;
}

token::~token()
{
    Delete(attribute);
}

std::ostream& token::show(std::ostream &s) const
{
    const char* astr = attribute ? attribute->getStr() : 0;
    switch (tokenID) {
        case END:               return s << "(eof)";
        case BEGIN:             return s << "(bof)";
        case NEWLINE:           return s << "(newline)";
        case SHARP:             return s << "#";
        case COMMA:             return s << ",";
        case DOT:               return s << ".";
        case SEMI:              return s << ";";
        case LPAR:              return s << "(";
        case RPAR:              return s << ")";
        case LBRAK:             return s << "[";
        case RBRAK:             return s << "]";
        case LBRACE:            return s << "{";
        case RBRACE:            return s << "}";
        case GT:                return s << ">";
        case LT:                return s << "<";
        case PLUS:              return s << "+";
        case MINUS:             return s << "-";
        case TIMES:             return s << "*";
        case DIVIDE:            return s << "/";
        case MOD:               return s << "%";
        case COLON:             return s << ":";
        case BANG:              return s << "!";
        case OR:                return s << "|";
        case AND:               return s << "&";

        case FORALL:            return s << "A";
        case EXISTS:            return s << "E";
        case FUTURE:            return s << "F";
        case PAST:              return s << "P";
        case GLOBALLY:          return s << "G";
        case HISTORICALLY:      return s << "H";
        case UNTIL:             return s << "U";
        case SINCE:             return s << "S";
        case NEXT:              return s << "X";
        case PREV:              return s << "Y";

        case GETS:              return s << ":=";
        case EQUALS:            return s << "==";
        case NEQUAL:            return s << "!=";
        case GE:                return s << ">=";
        case LE:                return s << "<=";
        case SET_DIFF:          return s << "\\";
        case IMPLIES:           return s << "->";
        case DOTDOT:            return s << "..";

        case IN:                return s << "in";
        case FOR:               return s << "for";
        case CONVERGE:          return s << "converge";
        case GUESS:             return s << "guess";
        case DEFAULT:           return s << "default";
        case PROC:              return s << "proc";
        case NUL:               return s << "null";

        case MAXIMIZE:          return s << "maximize";
        case MINIMIZE:          return s << "minimize";
        case SATISFIABLE:       return s << "satisfiable";

        case BOOLCONST:         return s << (bool_const ? "true" : "false");

        case STRCONST:          if (astr) {
                                    return s << "\"" << astr << "\"";
                                }
                                return s << "(null)";

        case INTCONST:
        case REALCONST:
        case IDENT:
                                if (!astr) {
                                    return s << "(null)";
                                }
                                return s << astr;

        case FORMALISM:
        case TYPE:
                                if (!type_attrib) {
                                    return s << "(null type)";
                                } else {
                                    const char* tn = type_attrib->getName();
                                    if (tn) return s << tn;
                                }
                                return s << "(null type name)";

        case MODIF:
                                if (!astr) {
                                    return s << "(null)";
                                }
                                return s << astr << " (index "
                                         << modif_attrib << ")";


        default:                return s << "unknown token";
    }
}

const char* token::getIdName() const
{
    switch (tokenID) {
        case END:       return "END";
        case BEGIN:     return "BEGIN";
        case NEWLINE:   return "NEWLINE";
        case SHARP:     return "SHARP";
        case COMMA:     return "COMMA";
        case DOT:       return "DOT";
        case SEMI:      return "SEMI";
        case LPAR:      return "LPAR";
        case RPAR:      return "RPAR";
        case LBRAK:     return "LBRAK";
        case RBRAK:     return "RBRAK";
        case LBRACE:    return "LBRACE";
        case RBRACE:    return "RBRACE";
        case GT:        return "GT";
        case LT:        return "LT";
        case PLUS:      return "PLUS";
        case MINUS:     return "MINUS";
        case TIMES:     return "TIMES";
        case DIVIDE:    return "DIVIDE";
        case MOD:       return "MOD";
        case COLON:     return "COLON";
        // case QUEST:     return "QUEST";
        case BANG:      return "BANG";
        case OR:        return "OR";
        case AND:       return "AND";

        case FORALL:        return "FORALL";
        case EXISTS:        return "EXISTS";
        case FUTURE:        return "FUTURE";
        case PAST:          return "PAST";
        case GLOBALLY:      return "GLOBALLY";
        case HISTORICALLY:  return "HISTORICALLY";
        case UNTIL:         return "UNTIL";
        case SINCE:         return "SINCE";
        case NEXT:          return "NEXT";
        case PREV:          return "PREV";

        case BOOLCONST:     return "BOOLCONST";
        case INTCONST:      return "INTCONST";
        case REALCONST:     return "REALCONST";
        case STRCONST:      return "STRCONST";
        case TYPE:          return "TYPE";
        case FORMALISM:     return "FORMALISM";
        case MODIF:         return "MODIF";
        case IDENT:         return "IDENT";

        case GETS:      return "GETS";
        case EQUALS:    return "EQUALS";
        case NEQUAL:    return "NEQUAL";
        case GE:        return "GE";
        case LE:        return "LE";
        case SET_DIFF:  return "SET_DIFF";
        case IMPLIES:   return "IMPLIES";
        case DOTDOT:    return "DOTDOT";

        case IN:        return "IN";
        case FOR:       return "FOR";
        case CONVERGE:  return "CONVERGE";
        case GUESS:     return "GUESS";
        case DEFAULT:   return "DEFAULT";
        case PROC:      return "PROC";
        case NUL:       return "NUL";

        case MAXIMIZE:      return "MAXIMIZE";
        case MINIMIZE:      return "MINIMIZE";
        case SATISFIABLE:   return "SATISFIABLE";

        default:            return "unknown token";
    }
}

void token::debug(outputStream &s) const
{
    s << "Token " << getIdName() << " " << where << " from text ";
    show(s.stream());
}

void token::set_special(type t)
{
    Delete(attribute);
    attribute = 0;
    where.clear();
    tokenID = t;
}

