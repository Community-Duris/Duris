//
//  File: readfile.cpp    originally part of durisEdit
//
//  Usage: generalized routines for reading and verifying text from area files
//

/*
 * Copyright (c) 1995-2007, Michael Glosenger
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * The name of Michael Glosenger may not be used to endorse or promote 
 *       products derived from this software without specific prior written 
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MICHAEL GLOSENGER ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO 
 * EVENT SHALL MICHAEL GLOSENGER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */




#include "fh.h"
#include "readfile.h"



//
// readAreaFileLine : EOF not allowed
//
//   intMaxLen is size without terminating NULL, \n, and ~ - so strn should be + 3 chars at least
//

bool readAreaFileLine(FILE *file, char *strn, const size_t intMaxLen, const uint intEntityType,
                      const uint intEntityNumb, const uint intEntityType2, 
                      const uint intEntityNumb2, const char *strnReading, 
                      const size_t intNumbArgs, const size_t *intNumbArgsArr, 
                      const bool blnEndTilde, const bool blnRemLeadingSpaces)
{
  return readAreaFileLineAllowEOF(file, strn, intMaxLen, intEntityType, intEntityNumb, intEntityType2, 
                                  intEntityNumb2, strnReading, intNumbArgs, intNumbArgsArr, blnEndTilde, 
                                  blnRemLeadingSpaces, NULL);
}


//
// displayReadAreaFileLineEntityInfo : common display
//

void displayReadAreaFileLineEntityInfo(const uint intEntityType, const uint intEntityNumb, 
                                       const uint intEntityType2, const uint intEntityNumb2)
{
  char outstrn[512];

  _outtext(getEntityTypeStrn(intEntityType));

  if (intEntityNumb != ENTITY_NUMB_UNUSED)
  {
    sprintf(outstrn, " #%u", intEntityNumb);

    _outtext(outstrn);
  }

 // if there is a second entity type, display it

  if (intEntityType2 != ENTITY_TYPE_UNUSED)
  {
    _outtext(" of ");
    _outtext(getEntityTypeStrn(intEntityType2));

    sprintf(outstrn, " #%u", intEntityNumb2);

    _outtext(outstrn);
  }
}


//
// readAreaFileLineAllowEOF : all errors displayed via _outtext, returns false on error
//
//   intMaxLen is size without terminating NULL, \n, and ~ - so strn should be + 3 chars at least
//

bool readAreaFileLineAllowEOF(FILE *file, char *strn, const size_t intMaxLen, 
                              const uint intEntityType, const uint intEntityNumb, 
                              const uint intEntityType2, const uint intEntityNumb2, 
                              const char *strnReading, const size_t intNumbArgs, 
                              const size_t *intNumbArgsArr, const bool blnEndTilde, 
                              const bool blnRemLeadingSpaces, bool *hitEOF)
{
  if (hitEOF)
    *hitEOF = false;

 // skip blank lines

  do
  {
    if (!fgets(strn, intMaxLen + 3, file))  // I know this generates a warning but I don't care
    {
      if (hitEOF)
      {
        *hitEOF = true;

        return true;
      }
      else
      {
        _outtext("\nError: Hit end of file trying to read ");
        _outtext(strnReading);
        _outtext(" for ");
        
        displayReadAreaFileLineEntityInfo(intEntityType, intEntityNumb, intEntityType2, intEntityNumb2);

        _outtext(".\n\n       Aborting.\n");

        return false;
      }
    }

    if (!lastCharLF(strn))  // couldn't read whole string
    {
      _outtext("\nError: Couldn't read entire ");
      _outtext(strnReading);
      _outtext(" line for ");
      
      displayReadAreaFileLineEntityInfo(intEntityType, intEntityNumb, intEntityType2, intEntityNumb2);
      
      char outstrn[512];

      sprintf(outstrn,
	      ".\n"
	      "       The maximum length that can be read is %zu.\n"
	      "       The text that could be read was '",
	      intMaxLen);

      _outtext(outstrn);

      _outtext(strn);
      
      _outtext("'.\n\n       Aborting.\n");

      return false;
    }

    if (blnRemLeadingSpaces)
      remLeadingSpaces(strn);
  }
  while (!strcmp(strn, "\n"));

  nolf(strn);
  remTrailingSpaces(strn);

  if (blnEndTilde)
  {
    if (strn[strlen(strn) - 1] != '~')
    {
      _outtext("\nError: ");
      _outtext(strnReading);
      _outtext(" line for ");
      
      displayReadAreaFileLineEntityInfo(intEntityType, intEntityNumb, intEntityType2, intEntityNumb2);

      _outtext(" doesn't terminate with a '~'.\n"
"       The string read was '");
      
      _outtext(strn);
      
      _outtext("'.\n\n       Aborting.\n");

      return false;
    }

    strn[strlen(strn) - 1] = '\0';
    remTrailingSpaces(strn);
  }

 // check number of args if specified

  if (intNumbArgs || intNumbArgsArr)
  {
    bool gotOne = false;

    const size_t numb = numbArgs(strn);

    char argStrn[512] = "";

    if (intNumbArgs)
    {
      if (numb == intNumbArgs)
        gotOne = true;
      else
	      sprintf(argStrn, "%zu", intNumbArgs);
    }
    else
    {
      const size_t *argptr = intNumbArgsArr;

      while (*argptr)
      {
        if (*argptr == numb)
        {
          gotOne = true;
          break;
        }

	sprintf(argStrn + strlen(argStrn), "%zu", *argptr);

	argptr++;

	if (*argptr)
	{
		if (*(argptr + 1))
			strcat(argStrn, ", ");
		else if (argptr == (intNumbArgsArr + 1))
			strcat(argStrn, " or ");
		else
			strcat(argStrn, ", or ");
	}
      }
    }

    if (!gotOne)
    {
      _outtext("\n"
"Error: Number of fields in line specifying ");
      
      _outtext(strnReading);
      
      _outtext(" for ");

      displayReadAreaFileLineEntityInfo(intEntityType, intEntityNumb, intEntityType2, intEntityNumb2);

      char outstrn[1024];

      sprintf(outstrn,
	      "\n       is incorrect (%zu instead of %s).\n"
	      "       The string read was '",
	      numbArgs(strn), argStrn);

      _outtext(outstrn);

      _outtext(strn);

      _outtext("'.\n\n       Aborting.\n");

      return false;
    }
  }


  return true;
}


//
// readAreaFileLineTildeLine : same as readAreaFileLine, but there must be a line with just a tilde after
//                             this line
//
//   intMaxLen is size without terminating NULL, \n, and ~ - so strn should be + 3 chars at least
//

bool readAreaFileLineTildeLine(
                      FILE *file, char *strn, const size_t intMaxLen, const uint intEntityType,
                      const uint intEntityNumb, const uint intEntityType2, 
                      const uint intEntityNumb2, const char *strnReading, 
                      const size_t intNumbArgs, const size_t *intNumbArgsArr, 
                      const bool blnEndTilde, const bool blnRemLeadingSpaces)
{
 // read data line

  if (!readAreaFileLine(file, strn, intMaxLen, intEntityType, intEntityNumb, intEntityType2, intEntityNumb2,
                        strnReading, intNumbArgs, intNumbArgsArr, blnEndTilde, blnRemLeadingSpaces))
    return false;

 // read tilde line

  char tildestrn[513];
  bool tooBig = false;  // simplify error-checking

  if (!fgets(tildestrn, 513, file))
  {
    _outtext("\n"
"Error: Hit end of file trying to read tilde line after ");
    _outtext(strnReading);
    _outtext(" for ");

    displayReadAreaFileLineEntityInfo(intEntityType, intEntityNumb, intEntityType2, intEntityNumb2);

    _outtext(".\n\n       Aborting.\n");

    return false;
  }

  if (!lastCharLF(tildestrn))
  {
    tooBig = true;
  }
  else
  {
    nolf(tildestrn);
    remLeadingSpaces(tildestrn);
    remTrailingSpaces(tildestrn);
  }

  if (tooBig || strcmp(tildestrn, "~"))
  {
    _outtext("\n"
"Error: Should-be tilde line after ");
    _outtext(strnReading);
    _outtext(" line for ");
    
    displayReadAreaFileLineEntityInfo(intEntityType, intEntityNumb, intEntityType2, intEntityNumb2);

    _outtext(" is invalid.\n"
"       The string read was '");
    
    _outtext(tildestrn);
    
    _outtext("'.\n\n       Aborting.\n");

    return false;
  }

  return true;
}
