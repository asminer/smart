
#include "tokens.h"

token::token()
{
	tokenID = END;
	attribute = 0;
}

token::token(const token &T)
{
	where = T.where;
	tokenID = T.tokenID;
	attribute = Share(T.attribute);
}

void token::operator=(const token& T)
{
	if (this != &T) {
		Delete(attribute);
		where = T.where;
		tokenID = T.tokenID;
		attribute = Share(T.attribute);
	}
}

token::~token()
{
	Delete(attribute);
}
