
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


/** 
	Allows the addition of "help topics", i.e., additional
	documentation not tied to functions or options.
	This is for "static" documentation.
*/
void AddTopic(const char* topic, const char* doc);

/**
	The function signature to use for "dynamic" documentation:
	void MyTopic(OutputStream &display, int LM, int RM);
*/
typedef void (*Topic_func) (OutputStream &display, int LM, int RM);

/** 
	Allows the addition of "help topics", i.e., additional
	documentation not tied to functions or options.
	This is for "dynamic" documentation, e.g., listing
	all known types.
*/
void AddTopic(const char* topic, Topic_func f);

/**
	Show all the topic names.
*/
void ShowTopicNames(OutputStream &display, int LM, int RM);

/**
	Display all topics matching the specified keyword.
*/
void MatchTopics(const char* key, OutputStream &display, int LM, int RM);

#endif

