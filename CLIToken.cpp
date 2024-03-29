/***********************************************************************************************************************
*                                                                                                                      *
* embedded-cli v0.1                                                                                                    *
*                                                                                                                      *
* Copyright (c) 2021-2023 Andrew D. Zonenberg and contributors                                                         *
* All rights reserved.                                                                                                 *
*                                                                                                                      *
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that the     *
* following conditions are met:                                                                                        *
*                                                                                                                      *
*    * Redistributions of source code must retain the above copyright notice, this list of conditions, and the         *
*      following disclaimer.                                                                                           *
*                                                                                                                      *
*    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the       *
*      following disclaimer in the documentation and/or other materials provided with the distribution.                *
*                                                                                                                      *
*    * Neither the name of the author nor the names of any contributors may be used to endorse or promote products     *
*      derived from this software without specific prior written permission.                                           *
*                                                                                                                      *
* THIS SOFTWARE IS PROVIDED BY THE AUTHORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   *
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL *
* THE AUTHORS BE HELD LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES        *
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR       *
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE       *
* POSSIBILITY OF SUCH DAMAGE.                                                                                          *
*                                                                                                                      *
***********************************************************************************************************************/

/**
	@file
	@brief Implementation of CLIToken
 */
#include "stdio.h"
#include "CLIToken.h"
#include <string.h>
#include <ctype.h>

/**
	@brief Checks if a short-form command matches this token
 */
bool CLIToken::PrefixMatch(const char* fullcommand)
{
	//Null is legal to pass if we match against the end of the token array.
	//It never matches, since it's not a valid command
	if(fullcommand == NULL)
		return false;

	for(size_t i = 0; i < MAX_TOKEN_LEN; i++)
	{
		//End of input with no mismatches? It's a match
		if(m_text[i] == '\0')
			return true;

		//Early-out on the first mismatch
		if(m_text[i] != fullcommand[i])
			return false;
	}

	return true;
}

/**
	@brief Checks if a command exactly matches this token
 */
bool CLIToken::ExactMatch(const char* fullcommand)
{
	//Null is legal to pass if we match against the end of the token array.
	//It never matches, since it's not a valid command
	if(fullcommand == NULL)
		return false;

	return (0 == strcmp(m_text, fullcommand) );
}
