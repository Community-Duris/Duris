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

#include "helpfile.h"

HelpFilesCPPClass help_index;

HelpFilesCPPClass::HelpFilesCPPClass()
{
	keywords  = NULL;
	filenames = NULL;
	HelpFilesCPPClass::reload();
}
bool HelpFilesCPPClass::reload()
{
	if (keywords == NULL && filenames == NULL)
	{
		// This function should only be called by the constructor, or
		// IMMEDIATELY after the clear() function has been called.
		defaultHelpOutput    = "Help topic cannot be found.\n\n";
		defaultHelpIndexXML  = "helpfiles/helpkeywordindex.xml";
		defaultHelpDirectory = "helpfiles/";
		helpWordWrapWidth    = 76;
		keywords             = NULL;
		filenames            = NULL;
		// Adjust these values to change speed vs. ram consumption
		keywordinitsize  = 1500; // This is about 50% larger than the helpsystem when I am writing this file, room for improvement for years to come
		keywordnotchsize = 500;  // It takes bloody forever to upgrade the size, so I'm doing it in very large increments.  More ram consumption, but faster to load.
		curmaxkeywords = numkeywords = 0;
		for (int i = 0; i < 26; i++)
		{
			indexPtrs[i] = 0;
		}
		return HelpFilesCPPClass::loadHelpFileIndexXML();
	}
}
HelpFilesCPPClass::~HelpFilesCPPClass() { HelpFilesCPPClass::clear(); }
void HelpFilesCPPClass::clear()
{
	if (keywords)
		delete[] keywords;
	if (filenames)
		delete[] filenames;
	keywords  = NULL;
	filenames = NULL;
}
string HelpFilesCPPClass::getHelpfile(string inputParameter)
{
	uint   i;
	string finalOutput = defaultHelpOutput;
	// ASSUMPTION - I like file pointers.  They're easier to use than fstreams, and not so flaky.
	FILE *input = NULL;

	// Step 1, remove spaces, tabs, and lowercase everything
	inputParameter = toLCase(HelpFilesCPPClass::cleanSpaces(inputParameter));

	// Step 2, clean non-alphanumeric characters
	for (i = 0; i < inputParameter.length(); i++)
	{
		if (!isAZ10(inputParameter.at(i)))
		{
			// replace(current char, one char, nothing)
			inputParameter = inputParameter.replace(i, 1, "");
		}
	}

	// Step 4, find file matching search inquiry
	// ******************************************************************************************************
	// Expand this section to include partial matches and the index.xml
	// ******************************************************************************************************
	string filename = defaultHelpDirectory + inputParameter + ".xml";
	input           = NULL;
	input           = fopen(filename.c_str(), "rt");
	// Step 4a is to just open the file directly - Exact match
	if (input == NULL || ferror(input) || feof(input))
	{
		// Step 4b is to ask the keyword index for a partial match, open THAT file, or return if there's still no results.
		filename = HelpFilesCPPClass::lookupXMLFilename(inputParameter);
		filename = defaultHelpDirectory + filename;
		input    = fopen(filename.c_str(), "rt");
	}

	if (input == NULL || ferror(input) || feof(input))
	{
		// finalOutput is still default, and if the file fails to load, then (for now) just fail and return
		return finalOutput;
	}
	else
	{
		// Step 5, file has been opened, read and format data.
		bool   readingContent = false;
		string helpTitle;
		string helpCategory;
		string helpContent;
		string helpSeeNext;
		string helpSeeAlso;
		string temp;
		string templc;

		while (input != NULL && !ferror(input) && !feof(input))
		{
			// This is nasty but neccessary, two copies of the current line, one lowercase and one normal
			// The helpfile format is a little more strict than the standard xml rules.
			// If the XML rules are followed, ALMOST all problems should be caught, if MY rules
			// aren't followed, then it should only result in formating errors, not a crash.
			temp   = readLine(input);
			templc = toLCase(temp);

			// Start of analysis loop: This will determine whether the line is useful,
			// and whether to put the line into a variable.  It can parse an XML file in any order, as long
			// as it is an XML file designed specifically for helpfiles, and the formating is correct.
			// Specifics of formating will be commented in each section.
			// Title should NOT include the "<>" characters, as they are used for formating.
			if (templc.find("</content>") != -1)
			{
				// This line will be ignored, and will signal the end of the content area.
				readingContent = false;
			}
			else if (readingContent)
			{
				// If you are reading content, All characters are preserved, html coded characters are converted,
				// and no other formating changes are made.  This includes tabbing, so make sure the content area is
				// NOT tabbed in unless it is intentional.  The web display also depends on this.
				helpContent += convertHTMLCodes(temp);
			}
			else if (templc.find("<content>") != -1)
			{
				// This line will be ignored, however all following lines will be placed into helpContent
				readingContent = true;
			}
			else if (templc.find("<title>") != -1)
			{
				// title line should be formated as follows:
				// <title>Title of Helpfile</title>
				// spaces inside title are preserved, the rest are removed.
				// Remvoe anything before the first '>' char
				while (temp.at(0) != '>')
				{
					// replace(first char, one char, nothing)
					temp = temp.replace(0, 1, "");
				}
				// Remove anything after the last '<' char
				while (temp.at(temp.length() - 1) != '<')
				{
					// replace(last char, one char, nothing)
					temp = temp.replace(temp.length() - 1, 1, "");
				}
				// Remove the rest
				temp = temp.replace(0, 1, "");
				temp = temp.replace(temp.length() - 1, 1, "");

				// Title is correctly formated and ready
				helpTitle = temp;
			}
			else if (templc.find("<category") != -1)
			{
				// Category line should be formated as follows:
				// <category type="Information" />
				// Category should not include excess " (quotes) for they are used for formating.
				// Remvoe anything before the first '"' char
				while (temp.at(0) != '\"')
				{
					// replace(first char, one char, nothing)
					temp = temp.replace(0, 1, "");
				}
				// Remove anything after the last '"' char
				while (temp.at(temp.length() - 1) != '\"')
				{
					// replace(last char, one char, nothing)
					temp = temp.replace(temp.length() - 1, 1, "");
				}
				// Remove the rest
				temp = temp.replace(0, 1, "");
				temp = temp.replace(temp.length() - 1, 1, "");

				// Category is correctly formated and ready
				helpCategory = temp;
			}
			else if (templc.find("<see-next") != -1)
			{
				// See-next line should be formated as follows:
				// <see-next file="filename" />
				// Category should not include excess " (quotes) for they are used for formating.
				// Remvoe anything before the first '"' char
				while (temp.at(0) != '\"')
				{
					// replace(first char, one char, nothing)
					temp = temp.replace(0, 1, "");
				}
				// Remove anything after the last '"' char
				while (temp.at(temp.length() - 1) != '\"')
				{
					// replace(last char, one char, nothing)
					temp = temp.replace(temp.length() - 1, 1, "");
				}
				// Remove the rest
				temp = temp.replace(0, 1, "");
				temp = temp.replace(temp.length() - 1, 1, "");

				// Category is correctly formated and ready
				helpSeeNext = temp;
			}
			else if (templc.find("<see-also") != -1)
			{
				// See-next line should be formated as follows:
				// <see-also file="filename" />
				// Category should not include excess " (quotes) for they are used for formating.
				// Remvoe anything before the first '"' char
				while (temp.at(0) != '\"')
				{
					// replace(first char, one char, nothing)
					temp = temp.replace(0, 1, "");
				}
				// Remove anything after the last '"' char
				while (temp.at(temp.length() - 1) != '\"')
				{
					// replace(last char, one char, nothing)
					temp = temp.replace(temp.length() - 1, 1, "");
				}
				// Remove the rest
				temp = temp.replace(0, 1, "");
				temp = temp.replace(temp.length() - 1, 1, "");

				// All caps
				temp = toUCase(temp);

				// Category is correctly formated and ready
				// Complex formating of helpSeeAlso is done HERE, as opposed to the formating section, due to
				// the way there are multiple identically formated see-also lines.
				if (helpSeeAlso.length() == 0)
				{
					// First see-also entry
					helpSeeAlso = "See also: ";
				}
				else
				{
					// Not the first entry
					helpSeeAlso += ", ";
				}
				helpSeeAlso += temp;
			}
		}

		// Step 6, copy formatted output to exit string
		// Line 1, Title (Category)
		finalOutput = "\n";
		finalOutput += helpTitle; // It is assumed there MUST be a title
		if (helpCategory.length() > 0)
		{ // Category is optional
			finalOutput += " (";
			finalOutput += helpCategory;
			finalOutput += ")";
		}
		finalOutput += "\n";
		// Line 3, helpWordWrapWidth = signs
		finalOutput += HelpFilesCPPClass::separator();
		finalOutput += "\n";
		// Main content area, starting on line 5
		finalOutput += helpContent; // Body is mandatory
		// 5th to last line, helpWordWrapWidth equal signs
		finalOutput += "\n";
		finalOutput += HelpFilesCPPClass::separator();
		// 3rd to last line, the next suggested reading line (at the moment, this is only used by the ship helpfiles) - but I thought it was a nice feature
		if (helpSeeNext.length() > 0)
		{ // SeeNext is optional
			finalOutput += "\n";
			finalOutput += "Next suggested reading: \"help ";
			finalOutput += helpSeeNext;
			finalOutput += "\"";
		}
		// last line, the See also line
		if (helpSeeAlso.length() > 0)
		{ // SeeAlso is optional
			finalOutput += "\n";
			finalOutput += helpSeeAlso;
		}
	}

	// Step 7, return output, either unchanged from default, or formated from step 6
	finalOutput += "\n\n";
	return finalOutput;
	// finalOutput is only changed once, at success of step 6.  If step 6 was not successful, then
	// it was never changed, therefore it will be the default output.
}
string HelpFilesCPPClass::separator()
{
	// This function generates a helpWordWrapWidth width line of equal signs
	string output = "";
	for (int i = 0; i < helpWordWrapWidth; i++)
		output += "=";

	return output;
}
string HelpFilesCPPClass::cleanSpaces(string input)
{
	while (input.length() > 0 && (input.at(input.length() - 1) == ' ' || input.at(input.length() - 1) == '	'))
		input = input.erase(input.length() - 1, 1);
	return input;
}
bool HelpFilesCPPClass::loadHelpFileIndexXML()
{
	string templc, newfilename, newkeyword, testkeywordfilename;
	size_t pos;
	FILE  *input = NULL;
	input        = fopen(defaultHelpIndexXML.c_str(), "rt");
	if (input == NULL || ferror(input) || feof(input))
	{
		// OMG ERROR ERROR!  Output some kind of error message here.
		return false;
	}
	else
	{
		// Read and add the keywords
		while (input != NULL && !ferror(input) && !feof(input))
		{
			templc = toLCase(readLine(input));

			if (templc.find("<keyword") != -1)
			{
				// title line should be formated as follows:
				// <keyword file="file.xml">keyword</keyword>
				// spaces inside title are preserved, the rest are removed.
				// Remvoe anything before the first '\"' char
				while (templc.at(0) != '\"')
				{
					// replace(first char, one char, nothing)
					templc = templc.replace(0, 1, "");
				}
				// Remove anything after the last '<' char
				while (templc.at(templc.length() - 1) != '<')
				{
					// replace(last char, one char, nothing)
					templc = templc.replace(templc.length() - 1, 1, "");
				}
				// Remove the \"
				templc = templc.replace(0, 1, "");
				// Remove the <
				templc = templc.replace(templc.length() - 1, 1, "");

				// Title is now formated:
				// file.xml">keyword
				pos                 = templc.find("\"");
				newfilename         = templc.substr(0, pos);
				newkeyword          = templc.substr(pos + 2, templc.length());
				testkeywordfilename = newkeyword + ".xml";
				addkeyword(newkeyword, newfilename);
			}
		}

		// Sort the keyword list and Set the first letter index flags
		sortkeywords();
	}
	return true;
}
string HelpFilesCPPClass::lookupXMLFilename(string input)
{
	string outputfile = "null.xml";
	if (input.size() < 1)
		return outputfile;

	// Find a partial keyword match
	char findPtr    = input.at(0) - 'a';
	int  startIndex = indexPtrs[findPtr];
	int  endIndex   = indexPtrs[findPtr + 1];
	// Again, like before, think of characters as integers and this makes sense - See previous rant about how awesome I am in sortkeywords() function
	for (int i = startIndex; i < endIndex; i++)
	{
		if (keywords[i].find(input) == 0)
			return filenames[i];
	}
	return outputfile;
}
void HelpFilesCPPClass::addkeyword(string newkeyword, string newfilename)
{
	// IMPORTANT!!!!!!  This function does NOT SORT THE DATA!
	// It is assumed that this function doesn't miraculously get called at odd times,
	// it is called ONLY during construction, and never again.
	// If you want to reset/renew/update the helpfiles, deconstruct the current HelpFilesCPPClass,
	// and construct a new one from scratch.
	newkeyword  = toLCase(HelpFilesCPPClass::cleanSpaces(newkeyword));
	newfilename = toLCase(HelpFilesCPPClass::cleanSpaces(newfilename));
	// If you BASTARDS put punctuation in the index.xml file, it's your own damn fault,
	// I fixed any other problems you might cause with the toLCase and cleanSpaces functions
	// I am of course referring to newbs who write new helpfiles.
	if (keywords == NULL && filenames == NULL)
	{
		// Begining of keyword list, initialize data
		keywords  = new  string[keywordinitsize];
		filenames = new string[keywordinitsize];
	}
	else if (curmaxkeywords == numkeywords)
	{
		// keywords has expanded too much, expand capacity
		string *tempkeywords  = new string[curmaxkeywords + keywordnotchsize];
		string *tempfilenames = new string[curmaxkeywords + keywordnotchsize];
		// copy current keys
		for (int i = 0; i < curmaxkeywords; i++)
		{
			tempkeywords[i]  = keywords[i];
			tempfilenames[i] = filenames[i];
		}
		// finish
		if (keywords)
			delete[] keywords;
		keywords = tempkeywords;
		if (filenames)
			delete[] filenames;
		filenames      = tempfilenames;
		curmaxkeywords = +keywordnotchsize;
	}
	// At this point, there should be extra space to add no matter what.
	keywords[numkeywords]  = newkeyword;
	filenames[numkeywords] = newfilename;
	numkeywords++;
}
void HelpFilesCPPClass::sortkeywords()
{
	// Quicksort Algorithm using the midpoint as key:
	// Reasons:  The index.xml SHOULD already be "mostly" sorted.  (It is not a requirement that index.xml be sorted, but it helps)
	// Chances are, the middle index is about or exactly the middle
	// Therefore, you're looking at very low chance of worst case quicksorts.
	sortkeywords(0, numkeywords - 1);

	// Sorted, set keyword index flags
	char find = 'a';
	for (int index = 0; find <= 'z' && index < numkeywords; index++)
	{
		// Search for the first instance of -find- as the first letter, mark it, and then increment find
		while (keywords[index].at(0) >= find)
		{
			// This is the line you will have trouble with if you're reading this code
			indexPtrs[find - 'a'] = index; // BUAHAHAHAHA, I'm so beefy!  This is twisted and horrible, but I like it!
			// basically, find is an integer, and 'a' is another integer.  find starts with 'a' and increments.
			// if you subtract 'a' (an integer) you have how many times find has been incremented.
			// NOTE, the condition statement on the for loop is when find has "passed" 'z' (an integer)
			// Damn I'm good.  I amaze myself sometimes.
			// NOTE, the WHILE loop ensures that if there's no "w" entries, that w and x are set to the same place, and the whole thing doesn't jam.
			find++;
		}
	}
}
void HelpFilesCPPClass::sortkeywords(int lowindex, int highindex)
{
	/*	char * temp = new char[15];
	    temp=itoa(lowindex,temp,10);
	    fputs(temp,stdout);
	    fputs(" - ",stdout);
	    temp=itoa(highindex,temp,10);
	    fputs(temp,stdout);
	    fputs(" \n",stdout);
	*/	// This is a quicksort sub-function
	// lowindex and highindex are inclusive
	int keyindex = (highindex - lowindex) / 2 + lowindex;
	int lowscan  = lowindex;
	int highscan = highindex;
	while (lowscan < highscan)
	{
		// Test lowscan index against keyindex index
		// increment till a key is found > keyindex index
		while (keywords[lowscan] <= keywords[keyindex] && lowscan < keyindex)
		{ // string comparison is brutal proccessor time, but neccessary
			lowscan++;
		}
		// Test highscan index against keyindex index
		// decrement till a key is found < keyindex index
		while (keywords[highscan] >= keywords[keyindex] && highscan > keyindex)
		{ // string comparison is brutal proccessor time, but neccessary
			highscan--;
		}
		// lowscan is at the first instance of a keyword > index keyword
		// highscan is at the last instance of a keyword < index keyword
		// Case 1: low<key<high
		if (lowscan < keyindex && keyindex < highscan)
		{
			sSwap(lowscan, highscan);
			// Case 2: low<key==high
		}
		else if (lowscan < keyindex && keyindex == highscan)
		{
			sSwap(lowscan, keyindex - 1);
			sSwap(keyindex, keyindex - 1);
			keyindex--;
			// Case 3:  low==key<high
		}
		else if (lowscan == keyindex && keyindex < highscan)
		{
			sSwap(highscan, keyindex + 1);
			sSwap(keyindex, keyindex + 1);
			keyindex++;
		}
		// Case 4: high==low==key - FINISHED!
	}
	// Recursive call to the two halves
	if (keyindex - lowindex > 1)
		sortkeywords(lowindex, keyindex - 1);
	if (highindex - keyindex > 1)
		sortkeywords(keyindex + 1, highindex);
	// SORTED!
	// Temporary check for sortedness
	/*	if(lowindex==0 && highindex>1000){
	        for(int i=0;i+1<highindex;i++){
	            if(keywords[i]>keywords[i+1]){
	                fputs(keywords[i].c_str(),stdout);
	                fputs(" - ",stdout);
	                fputs(keywords[i+1].c_str(),stdout);
	                fputs("\n",stdout);
	            }
	        }
	    }*/
}
void HelpFilesCPPClass::sSwap(int indexA, int indexB)
{
	// This is a quicksort sub-function
	// swap two indexes
	string tempkeyword  = keywords[indexA];
	string tempfilename = filenames[indexA];
	keywords[indexA]    = keywords[indexB];
	filenames[indexA]   = filenames[indexB];
	keywords[indexB]    = tempkeyword;
	filenames[indexB]   = tempfilename;
}

/* utilities moved from weebuseful.c */
// From my "useful" function library
// Tests if a character is 1-0,a-z, or A-Z returns true if above is true, false if above is false
bool isAZ10(char test)
{
	// Compare version
	if ((test >= 'a' && test <= 'z') || (test >= 'A' && test <= 'Z') || (test >= '0' && test <= '9'))
	{
		return true;
	}
	else
	{
		return false;
	}
}
// Reads a line from the input, and returns it, including the \n
string readLine(FILE *input)
{
	// This function will read one line from input, until a return character, end of file, or error occurs.
	bool   reading = true;
	char   temp;
	string output = "";

	while (reading && input != NULL && !ferror(input) && !feof(input))
	{
		temp = fgetc(input);
		if (temp != '\n')
		{
			output += temp;
		}
		else
		{
			output += '\n';
			reading = false;
		}
	}
	return output;
}
// Converts all capitol characters in input to lowercase
string toLCase(string input)
{
	for (uint i = 0; i < input.length(); i++)
	{
		char test = input.at(i);
		if (test >= 'A' && test <= 'Z')
		{
			char replace[2] = " ";
			replace[0]      = test + ('a' - 'A');
			string temp     = replace;
			input.replace(i, 1, temp);
		}
	}
	return input;
}
// Converts all capitol characters in input to lowercase
string toUCase(string input)
{
	for (uint i = 0; i < input.length(); i++)
	{
		char test = input.at(i);
		if (test >= 'a' && test <= 'z')
		{
			char replace[2] = " ";
			replace[0]      = test + ('A' - 'a');
			string temp     = replace;
			input.replace(i, 1, temp);
		}
	}
	return input;
}
// Converts common html character codes to to the corresponding output. Also converts all the foreground xml color codes to ansi color codes
string convertHTMLCodes(string input)
{
	// This is a function that converts common html character codes in the string to the actual characters.
	// This function also converts the xml color tags to ansi color tags.  NOTE, helpfiles are only capable
	// of coloring the foreground, not the background.
	// This function is probably the most time consuming part, as it processes an indefinite amount
	// of codes in the text.  Thankfully, the need for these codes is likely to be slim.

	// Common Html character codes
	while (input.find("&quot;") != -1)
	{
		input = stringReplace(input, "&quot;", "\"");
	}
	while (input.find("&amp;") != -1)
	{
		input = stringReplace(input, "&amp;", "&");
	}
	while (input.find("&lt;") != -1)
	{
		input = stringReplace(input, "&lt;", "<");
	}
	while (input.find("&gt;") != -1)
	{
		input = stringReplace(input, "&gt;", ">");
	}
	while (input.find("&nbsp;") != -1)
	{
		input = stringReplace(input, "&nbsp;", " ");
	}

	// Custom character codes (often c++ equivalents)
	while (input.find("\\n") != -1)
	{
		input = stringReplace(input, "\\n", "\n");
	}

	// This list is not complete, if more are needed, more should be added.  As far as I know, no extra codes should
	// be needed for the characters that are currently in the helpfiles

	// XML color codes to ansi color codes
	while (input.find("<L>") != -1)
	{
		input = stringReplace(input, "<L>", "&+L");
	}
	while (input.find("<l>") != -1)
	{
		input = stringReplace(input, "<l>", "&+l");
	}
	while (input.find("<W>") != -1)
	{
		input = stringReplace(input, "<W>", "&+W");
	}
	while (input.find("<w>") != -1)
	{
		input = stringReplace(input, "<w>", "&+w");
	}
	while (input.find("<R>") != -1)
	{
		input = stringReplace(input, "<R>", "&+R");
	}
	while (input.find("<r>") != -1)
	{
		input = stringReplace(input, "<r>", "&+r");
	}
	while (input.find("<G>") != -1)
	{
		input = stringReplace(input, "<G>", "&+G");
	}
	while (input.find("<g>") != -1)
	{
		input = stringReplace(input, "<g>", "&+g");
	}
	while (input.find("<C>") != -1)
	{
		input = stringReplace(input, "<C>", "&+C");
	}
	while (input.find("<c>") != -1)
	{
		input = stringReplace(input, "<c>", "&+c");
	}
	while (input.find("<Y>") != -1)
	{
		input = stringReplace(input, "<Y>", "&+Y");
	}
	while (input.find("<y>") != -1)
	{
		input = stringReplace(input, "<y>", "&+y");
	}
	while (input.find("<B>") != -1)
	{
		input = stringReplace(input, "<B>", "&+B");
	}
	while (input.find("<b>") != -1)
	{
		input = stringReplace(input, "<b>", "&+b");
	}
	while (input.find("<M>") != -1)
	{
		input = stringReplace(input, "<M>", "&+M");
	}
	while (input.find("<m>") != -1)
	{
		input = stringReplace(input, "<m>", "&+m");
	}

	while (input.find("</L>") != -1)
	{
		input = stringReplace(input, "</L>", "&n");
	}
	while (input.find("</l>") != -1)
	{
		input = stringReplace(input, "</l>", "&n");
	}
	while (input.find("</W>") != -1)
	{
		input = stringReplace(input, "</W>", "&n");
	}
	while (input.find("</w>") != -1)
	{
		input = stringReplace(input, "</w>", "&n");
	}
	while (input.find("</R>") != -1)
	{
		input = stringReplace(input, "</R>", "&n");
	}
	while (input.find("</r>") != -1)
	{
		input = stringReplace(input, "</r>", "&n");
	}
	while (input.find("</G>") != -1)
	{
		input = stringReplace(input, "</G>", "&n");
	}
	while (input.find("</g>") != -1)
	{
		input = stringReplace(input, "</g>", "&n");
	}
	while (input.find("</C>") != -1)
	{
		input = stringReplace(input, "</C>", "&n");
	}
	while (input.find("</c>") != -1)
	{
		input = stringReplace(input, "</c>", "&n");
	}
	while (input.find("</Y>") != -1)
	{
		input = stringReplace(input, "</Y>", "&n");
	}
	while (input.find("</y>") != -1)
	{
		input = stringReplace(input, "</y>", "&n");
	}
	while (input.find("</B>") != -1)
	{
		input = stringReplace(input, "</B>", "&n");
	}
	while (input.find("</b>") != -1)
	{
		input = stringReplace(input, "</b>", "&n");
	}
	while (input.find("</M>") != -1)
	{
		input = stringReplace(input, "</M>", "&n");
	}
	while (input.find("</m>") != -1)
	{
		input = stringReplace(input, "</m>", "&n");
	}

	return input;
}
// This is a function that SHOULD be a member function of string but is not for my version.
string stringReplace(string input, string remove, string add)
{
	// Precondition: remove is in the input string
	// Postcondition: add string has replaced the first instance of remove
	size_t tempPos = input.find(remove);
	input.replace(tempPos, remove.length(), add);
	return input;
}
