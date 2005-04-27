
// $Id$

#ifndef DOCS_H
#define DOCS_H

#include "streams.h"

/**	
	A standardized way to display documentation.

	@param	display		The output stream to use
	@param	doc		The documentation string.
				Certain "commands" can be 
				used for formatting.

	@param	LM		Left margin (number of spaces)
	@param	RM 		Right margin (number of spaces)

	@param	rule	If true, a rule is put before documentation

	Margins are measured in terms of displayed characters.
	On an 80-character wide display, one might use a left margin
	of 5 and a right margin of 75, for example.
*/
void DisplayDocs(OutputStream &display, const char* doc, 
		int LM, int RM, bool rule);


/**
	Does this item match a help query.
	Returns true if the search string is contained in the
	documentation, ignoring case.
*/
bool DocMatches(const char* doc, const char* srch);

#endif

