/***********************************************************************************************************************
*                                                                                                                      *
* embedded-cli v0.1                                                                                                    *
*                                                                                                                      *
* Copyright (c) 2021 Andrew D. Zonenberg and contributors                                                              *
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
	@brief Declaration of CLISessionContext
 */
#ifndef CLISessionContext_h
#define CLISessionContext_h

#include <stdint.h>
#include "CLICommand.h"

class CLIOutputStream;

#ifndef CLI_USERNAME_MAX
#define CLI_USERNAME_MAX 32
#endif

/**
	@brief A single keyword in the CLI command tree
 */
struct clikeyword_t
{
	///@brief ASCII representation of the unabbreviated keyword
	const char*			keyword;

	///@brief Integer identifier used by the command parser
	uint16_t			id;

	///@brief Child nodes for subsequent words
	const clikeyword_t*	children;

	///@brief Help message
	const char*			help;
};

/**
	@brief A session context for a CLI session
 */
class CLISessionContext
{
public:
	CLISessionContext(const clikeyword_t* root)
	: m_rootCommands(root)
	{}

	virtual void Initialize(CLIOutputStream* ctx, const char* username);

	void OnKeystroke(char c);

	/**
		@brief Prints the command prompt
	 */
	virtual void PrintPrompt() =0;

protected:

	///@brief Handles a line of input being fully entered
	virtual void OnExecute() =0;

	void RedrawLineRightOfCursor();

	void OnExecuteComplete();

	void OnBackspace();
	void OnTabComplete();
	void OnSpace();
	void OnChar(char c);
	void OnArrowLeft();
	void OnArrowRight();
	void OnLineReady();
	void OnHelp();
	void PrintHelp(const clikeyword_t* node, const char* prefix);

	bool ParseCommand();

	///@brief The output stream
	CLIOutputStream* m_output;

	///@brief The command currently being
	CLICommand m_command;

	/**
		@brief Name of the currently logged in user

		Provided by upper layer in case of e.g. SSH. May be blank in case of UART or other transports that do not
		include authentication.
	 */
	char m_username[CLI_USERNAME_MAX];

	///@brief State machine for escape sequence parsing
	enum
	{
		STATE_NORMAL,
		STATE_EXPECT_BRACKET,
		STATE_EXPECT_PAYLOAD
	} m_escapeState;

	///@brief Index of the last token in the command
	int m_lastToken;

	///@brief Index of the token we're currently writing to
	int m_currentToken;

	///@brief Position of the cursor within the current token
	int m_tokenOffset;

	///@brief The root of the command tree
	const clikeyword_t* m_rootCommands;
};

#endif
