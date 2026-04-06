/*
XML Helpfile System,
Copyright 2005, Paul Gilmore
All use is prohibited without permission from
author, Paul Gilmore.

This copy is for the exclusive use of Duris: Land of the Bloodlust
*/

#include <stdio.h>
#include <string>
using namespace std;

#ifndef XML_HELP_SYSTEM_HPP
#define XML_HELP_SYSTEM_HPP

/*
This file contains the function prototypes of all functions used'
in this version of the XML Helpfile System.
Prerequisites:
#include <string> must be declared, or
a string class similar to the microsoft visual studio 6.0 string class
#include <stdio.h> must be declared
*/

class HelpFilesCPPClass
{
public:
	// Constructor loads keyword data from the index.
	HelpFilesCPPClass(); // calls reload
	// Destructor deletes dynamic memory allocation
	~HelpFilesCPPClass(); // calls clear

	// calling clear, and then reload will reload the keyword index
	bool reload();
	void clear();

	// Primary function
	string getHelpfile(string inputParameter);
	/*
	    Function:	string getHelpfile(string inputParameter)
	    Preconditons:
	      1. inputParameter contains alphanumeric text input directly from the command input from a player.
	         Example:  User types "help commands" then inputParameter should be "commands"
	      2. IMPORTANT - operating system can have more than 8 character filenames.  (most modern ones do, to my knowledge)
	         (my assumptions are pretty basic for current operating systems, but I just want to make sure you know)
	    Postconditions:
	         the inputParameter will be cleaned of all non-alphanumeric characters before further processing
	         IF preconditions are met, two outcomes can follow:
	      a) A string will be returned, which is the output of the helpfile that matches search inquiry
	         described in inputParameter
	      b) A default string will be returned, indicating failure to find specified file.
	         IF precondition 1a is failed, then the inputParameter will be cleaned of all characters that cause failure
	         until precondition is met, and then function will continue.
	         IF for whatever reason inputParameter cannot be cleaned, or is empty after cleaning, see outcome b.
	    NOTES:
	         All spaces/tabs/punctuation will be removed
	    */
protected:
	string separator();
	// Returns a separator helpWordWrapWidth characters wide, consisting of = chars
	string cleanSpaces(string input);
	// This function returns a string that has had all of it's spaces and tabs stripped.
	bool loadHelpFileIndexXML();
	// This function loads the keyword index from the filename in the defaultHelpIndexXML string
	string lookupXMLFilename(string input);
	// This function searches the keyword/filename pairs for a keyword match, and returns the filename associated with that match.
	void addkeyword(string newkeyword, string newfilename);
	// This function adds the new keyword to the list of keywords, and if neccessary, expands the size of the keyword section
	void sortkeywords();
	void sortkeywords(int lowindex, int highindex);
	void sSwap(int indexA, int indexB);
	// These functions quicksort all the keywords in the list of keywords.  This is the ONLY place that the keywords get sorted.
	// These will also set the indexPtrs.  (only call sortkeywords(), the other two are controlled by this one)

	// This is the keyword data, a pair of strings, one is a keyword, one is a filename matching the keyword
	// I'd like to note:  These keywords are the files that do NOT match their keyword.xml filename.
	string *keywords;         // This is a sorted list of keywords.
	string *filenames;        // There will be several repeats in this list, but usually not immediately adjacent.
	int     indexPtrs[26];    // This is an index of where each starting letter keyword is, a=0, b=100 or so, etc.  Makes for faster finding.
	int     keywordinitsize;  // This is the initial size of the keyword system
	int     keywordnotchsize; // This is how much extra space the keyword system will increment at a time
	int     curmaxkeywords;   // This is how many keywords can be defined before expanding again.
	int     numkeywords;      // This is how many keywords are currently defined.

	// These are declared in the constructor, but I thought it would be useful if you can see the values of these in the comments.
	// This is the default string returned if helpfile cannot be found
	string defaultHelpOutput; // = "Help topic cannot be found.";
	// This is the default string returned if helpfile cannot be found
	string defaultHelpIndexXML; // = "helpfiles/helpkeywordindex.xml";
	// This is the directory prefix put in front of the string filename, when searching for a file
	// If this is not clear:  looking for "commands.xml", the file that would be searched for is:
	// "helpfiles/commands.xml".  This can also be changed to an absolute directory, so that the search is:
	// "c:/mud.org/helpfiles/commands.xml".  It CAN search network drives and shared folders, but it will be slow.
	// It CANNOT search webpages or ftp without an improvement in the file opener.
	string defaultHelpDirectory; // = "helpfiles/";
	// This is the wordwrap width that the mud helpfiles are restricted to.
	// This number has been defined already, but can be changed later if neccessary, by changing only this number
	// Unfortunately, it doesn't work yet.
	int helpWordWrapWidth; // = 76;
};

/* moved from weebuseful.hpp */

// From my "useful" function library
// Tests if a character is 1-0,a-z, or A-Z returns true if above is true, false if above is false
bool isAZ10(char test);
// Reads a line from the input, and returns it, including the \n
string readLine(FILE *input);
// Converts all capitol characters in input to lowercase
string toLCase(string input);
// Converts all capitol characters in input to lowercase
string toUCase(string input);
// Converts common html character codes to to the corresponding output. Also converts all the foreground xml color codes to ansi color codes
string convertHTMLCodes(string input);
// This is a function that SHOULD be a member function of string but is not for my version.
string stringReplace(string input, string remove, string add);

#endif // XML_HELP_SYSTEM_HPP
