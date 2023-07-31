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
	@brief Declaration of CLIToken
 */
#ifndef CLIToken_h
#define CLIToken_h

#include <stdint.h>
#include <string.h>

#ifndef MAX_TOKEN_LEN

	///@brief Maximum number of characters in a single token
	#define MAX_TOKEN_LEN 32

#endif

///@brief Empty string or otherwise malformed
#define INVALID_COMMAND 0xffff

///@brief This token can consist of arbitrary freeform text without spaces
#define FREEFORM_TOKEN 0xfffe

///@brief This token denotes an early end-of-command (for optional arguments)
#define OPTIONAL_TOKEN 0xfffd

///@brief This token consumes all input to the end of the command line (including spaces)
#define TEXT_TOKEN 0xfffc

/**
	@brief A single token within a command
 */
class CLIToken
{
public:

	CLIToken()
	{
		Clear();
	}

	/**
		@brief Returns true if the token is empty
	 */
	bool IsEmpty()
	{ return strlen(m_text) == 0; }

	/**
		@brief Returns the length of this token, in characters
	 */
	int Length()
	{ return strlen(m_text); }

	/**
		@brief Resets this token to the empty state
	 */
	void Clear()
	{
		memset(m_text, 0, MAX_TOKEN_LEN);
		m_commandID = INVALID_COMMAND;
	}

	/**
		@brief Helper operator for string matching
	 */
	bool operator==(const char* rhs)
	{ return strcmp(m_text, rhs) == 0; }

	bool PrefixMatch(const char* fullcommand);
	bool ExactMatch(const char* fullcommand);

public:

	///@brief Text representation of the token
	char m_text[MAX_TOKEN_LEN];

	///@brief Parsed command ID (if matched) or INVALID_TOKEN otherwise
	uint16_t m_commandID;
};

#endif
