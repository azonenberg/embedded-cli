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
	@brief Implementation of CLISessionContext
 */
#include "stdio.h"
#include "CLISessionContext.h"
#include "CLIOutputStream.h"
#include <string.h>
#include <ctype.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Setup

void CLISessionContext::Initialize(CLIOutputStream* ctx, const char* username)
{
	strncpy(m_username, username, CLI_USERNAME_MAX);
	m_username[CLI_USERNAME_MAX-1] = 0;
	m_command.Clear();

	m_output = ctx;
	m_escapeState = STATE_NORMAL;

	m_lastToken = 0;
	m_currentToken = 0;
	m_tokenOffset = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Input handling

/**
	@brief Handles an incoming keystroke
 */
void CLISessionContext::OnKeystroke(char c)
{
	//Square bracket in escape sequence
	if(m_escapeState == STATE_EXPECT_BRACKET)
	{
		if(c == '[')
			m_escapeState = STATE_EXPECT_PAYLOAD;

		//ignore malformed escape sequences
		else
			m_escapeState = STATE_NORMAL;

		return;
	}

	//Escape sequence payload
	else if(m_escapeState == STATE_EXPECT_PAYLOAD)
	{
		switch(c)
		{
			//B = down, A = up

			case 'C':
				OnArrowRight();
				break;

			case 'D':
				OnArrowLeft();
				break;

			//ignore unknown escape sequences
			default:
				break;
		}

		//escape sequence is over
		m_escapeState = STATE_NORMAL;
	}

	//Newline? Execute the command
	else if( (c == '\r') || (c == '\n') )
	{
		m_output->PutCharacter('\n');
		OnLineReady();
		if(ParseCommand())
			OnExecute();
		OnExecuteComplete();
	}

	//Backspace? Delete the current character.
	//But we might have to move left a token
	else if( (c == '\b') || (c == '\x7f') )
		OnBackspace();

	//Tab? Do tab completion
	else if(c == '\t')
		OnTabComplete();

	//Question mark? Print help text
	else if(c == '?')
		OnHelp();

	//Space starts a new token
	else if(c == ' ')
		OnSpace();

	//Start an escape sequence
	else if(c == '\x1b')
		m_escapeState = STATE_EXPECT_BRACKET;

	else
		OnChar(c);

	//Update the last-token index
	for(size_t i=0; i<MAX_TOKENS_PER_COMMAND; i++)
	{
		//If we type two spaces in the middle of a command we can have an empty command momentarily.
		//This is totally fine.
		if(m_command[i].IsEmpty())
			continue;
		m_lastToken = i;
	}

	//If we backspaced over the beginning of a token, it's going to be empty.
	//But we still want to count the empty token as present for now
	if(m_currentToken > m_lastToken)
		m_lastToken = m_currentToken;

	//All done with whatever we're printing, flush stdout
	m_output->Flush();

	//DebugPrint();
}

///@brief Handles a printable character
void CLISessionContext::OnChar(char c)
{
	//If the token doesn't have room for another character, abort
	char* token = m_command[m_currentToken].m_text;
	int len = strlen(token);
	if(len >= (MAX_TOKEN_LEN - 1))
		return;

	//If we're NOT at the end of the token, we need to move everything right to make room for the new character
	bool redrawLine = (m_currentToken != m_lastToken);
	if(m_tokenOffset != len)
	{
		redrawLine = true;
		for(int i=len; i > m_tokenOffset; i--)
			token[i] = token[i-1];
	}

	//Append the character to the current token and echo it
	token[m_tokenOffset ++] = c;
	len++;
	m_output->PutCharacter(c);

	//Update the remainder of the line.
	if(redrawLine)
		RedrawLineRightOfCursor();
}

///@brief Handles a tab character
void CLISessionContext::OnTabComplete()
{
	printf("tab complete todo\n");
}

///@brief Handles a '?' character
void CLISessionContext::OnHelp()
{
	if(m_rootCommands == NULL)
		return;

	//If we have NO command, show all legal top level commands and descriptions
	if(m_command[0].IsEmpty())
	{
		PrintHelp(m_rootCommands, NULL);
		return;
	}

	//Go through each token and figure out if it matches anything we know about
	const clikeyword_t* node = m_rootCommands;
	for(int i = 0; i < MAX_TOKENS_PER_COMMAND; i ++)
	{
		//If this is the current token, always search it
		if(i == m_currentToken)
		{
			PrintHelp(node, m_command[i].m_text);
			return;
		}

		//See what's legal at this position
		if(m_command[i].IsEmpty())
		{
			if(node != NULL)
			{
				PrintHelp(node, NULL);
				return;
			}
		}

		for(auto row = node; row->keyword != NULL; row++)
		{
			//Wildcards always match
			if(row->id == FREEFORM_TOKEN)
			{
			}

			else
			{
				//If the token doesn't match the prefix, we're definitely not a hit
				if(!m_command[i].PrefixMatch(row->keyword))
					continue;

				//If it matches, but the subsequent token matches too, there's a few options
				if(m_command[i].PrefixMatch(row[1].keyword))
				{
					PrintHelp(node, m_command[i].m_text);
					return;
				}
			}

			//Match!
			m_command[i].m_commandID = row->id;
			node = row->children;
		}
	}
}

///@brief Prints help
void CLISessionContext::PrintHelp(const clikeyword_t* node, const char* prefix)
{
	bool filter = true;
	if( (prefix == NULL) || (strlen(prefix) == 0) )
		filter = false;

	m_output->Printf("?\n");
	for(size_t i=0; node[i].keyword != NULL; i++)
	{
		//Skip stuff with the wrong prefix
		if(filter)
		{
			if(node[i].keyword != strstr(node[i].keyword, prefix))
				continue;
		}

		m_output->Printf("    %-20s %s\n", node[i].keyword, node[i].help);
	}

	PrintPrompt();

	//Re-print the current command
	//TODO: handle ? at anywhere other than end of command
	for(int i=0; i<MAX_TOKENS_PER_COMMAND; i++)
	{
		if(m_command[i].IsEmpty())
			continue;

		if(i > 0)
			m_output->Printf(" ");
		m_output->Printf("%s", m_command[i].m_text);
	}

	//If the current token is blank, add a trailing space
	if((m_currentToken > 0) && m_command[m_currentToken].IsEmpty())
		m_output->Printf(" ");
}

///@brief Handles a backspace character
void CLISessionContext::OnBackspace()
{
	//We're in the middle/end of a token
	if(m_tokenOffset > 0)
	{
		//Delete the character
		m_output->Backspace();

		//Move back one character and shift the token left
		m_tokenOffset --;
		char* text = m_command[m_currentToken].m_text;
		for(int i=m_tokenOffset; i<MAX_TOKEN_LEN-1; i++)
			text[i] = text[i+1];
	}

	//We're at the start of a token, but not the first one
	else if(m_currentToken > 0)
	{
		//Move before the space (already blank, no need to space over it)
		m_output->CursorLeft();

		//Move to end of previous token
		m_currentToken --;
		m_tokenOffset = strlen(m_command[m_currentToken].m_text);

		//Merge the token we were in with the current one
		strncpy(
			m_command[m_currentToken].m_text + m_tokenOffset,
			m_command[m_currentToken+1].m_text,
			MAX_TOKEN_LEN - m_tokenOffset);

		//If we have any tokens to the right of us, move them left
		for(int i = m_currentToken+1; i < m_lastToken; i++)
			strncpy(m_command[i].m_text, m_command[i+1].m_text, MAX_TOKEN_LEN);

		//Blow away the removed token at the far right
		memset(m_command[m_lastToken].m_text, 0, MAX_TOKEN_LEN);
	}

	//Backspace at the start of the prompt. Ignore it.
	else
	{
	}

	RedrawLineRightOfCursor();
}

///@brief Handles a space character
void CLISessionContext::OnSpace()
{
	//If we're out of token space, stop
	if(m_lastToken >= (MAX_TOKENS_PER_COMMAND - 1) )
		return;

	//Ignore consecutive spaces - if we already made an empty token between the existing ones,
	//there's no need to do another one
	if(m_command[m_currentToken].IsEmpty())
		return;

	//OK, we're definitely adding a space. The only question now is where.
	m_output->PutCharacter(' ' );

	//If we're at the end of the token, just insert a new token
	if(m_tokenOffset == m_command[m_currentToken].Length())
	{
		//Not empty. Start a new token.
		m_currentToken ++;

		//If this was the last token, nothing more to do.
		if(m_currentToken > m_lastToken)
		{
			m_tokenOffset = 0;
			return;
		}

		//If we have tokens to the right, move them right and wipe the current one.
		for(int i = m_lastToken+1; i > m_currentToken; i-- )
			strncpy(m_command[i].m_text, m_command[i-1].m_text, MAX_TOKEN_LEN);
		memset(m_command[m_currentToken].m_text, 0, MAX_TOKEN_LEN);

		m_tokenOffset = 0;
	}

	//We're in the middle of a token. Need to split it.
	else
	{
		//If we have tokens to the right, move them right
		if(m_currentToken < m_lastToken)
		{
			for(int i = m_lastToken+1; i > (m_currentToken+1); i-- )
				strncpy(m_command[i].m_text, m_command[i-1].m_text, MAX_TOKEN_LEN);
		}

		//Move the right half of the split token into a new one at right
		strncpy(
			m_command[m_currentToken+1].m_text,
			m_command[m_currentToken].m_text + m_tokenOffset,
			MAX_TOKEN_LEN);

		//Truncate the left half of the split token
		for(size_t i=m_tokenOffset; i<MAX_TOKEN_LEN; i++)
			m_command[m_currentToken].m_text[i] = '\0';

		//We're now at the start of the new token
		m_currentToken ++;
		m_tokenOffset = 0;
	}

	//Definitely have to redraw after doing this!
	RedrawLineRightOfCursor();

}

///@brief Handles a left arrow key press
void CLISessionContext::OnArrowLeft()
{
	//At somewhere other than the start of the current token?
	//Just move left and go to the previous character
	if(m_tokenOffset > 0)
	{
		m_tokenOffset --;
		m_output->CursorLeft();
	}

	//Move left, but go to the previous token
	else if(m_currentToken > 0)
	{
		m_output->CursorLeft();
		m_currentToken --;
		m_tokenOffset = strlen(m_command[m_currentToken].m_text);
	}

	//Start of prompt, can't go left any further
	else
	{
	}
}

///@brief Handles a right arrow key press
void CLISessionContext::OnArrowRight()
{
	//Are we at the end of the current token?
	char* text = m_command[m_currentToken].m_text;
	if(m_tokenOffset == (int)strlen(text))
	{
		//Is this the last token? Can't go any further
		if(m_currentToken == m_lastToken)
		{
		}

		//Move to the start of the next one
		else
		{
			m_tokenOffset = 0;
			m_currentToken ++;
			m_output->CursorRight();
		}
	}

	//Nope, just move one to the right
	else
	{
		m_tokenOffset ++;
		m_output->CursorRight();
	}
}

///@brief Prepares a line to be executed
void CLISessionContext::OnLineReady()
{
	//If we have any empty tokens, move stuff left to form a canonical command for execution
	for(int i=0; i<m_lastToken; i++)
	{
		if(m_command[i].IsEmpty())
		{
			for(int j=i+1; j <= m_lastToken; j++)
				strncpy(m_command[j-1].m_text, m_command[j].m_text, MAX_TOKEN_LEN);
			m_lastToken --;
		}
	}
}

///@brief Cleans up a line after it executes
void CLISessionContext::OnExecuteComplete()
{
	m_command.Clear();

	m_lastToken = 0;
	m_currentToken = 0;
	m_tokenOffset = 0;

	PrintPrompt();
}

///@brief Redraws the portion of the line right of the cursor (for typing mid line)
void CLISessionContext::RedrawLineRightOfCursor()
{
	//Draw the remainder of this token
	char* token = m_command[m_currentToken].m_text;
	char* restOfToken = token + m_tokenOffset;
	int charsDrawn = strlen(restOfToken);
	m_output->PutString(restOfToken);

	//Draw all subsequent tokens
	for(int i = m_currentToken + 1; i < MAX_TOKENS_PER_COMMAND; i++)
	{
		char* tok = m_command[i].m_text;
		charsDrawn += strlen(tok) + 1;
		m_output->PutCharacter(' ');
		m_output->PutString(tok);
	}

	//Draw a space at the end to clean up anything we may have deleted
	m_output->PutCharacter(' ');
	charsDrawn ++;

	//Move the cursor back to where it belongs
	for(int i=0; i<charsDrawn; i++)
		m_output->CursorLeft();
}

void CLISessionContext::DebugPrint()
{
	printf("Cursor: char %d of token %d/%d\n", m_tokenOffset, m_currentToken, m_lastToken);
	for(int i=0; i <= m_lastToken; i++)
		printf("    [%2d] %s (%d)\n", i, m_command[i].m_text, m_command[i].m_commandID);
}

/**
	@brief Parses a command to numeric command IDs
 */
bool CLISessionContext::ParseCommand()
{
	if(m_rootCommands == NULL)
		return false;

	//Go through each token and figure out if it matches anything we know about
	const clikeyword_t* node = m_rootCommands;
	for(size_t i = 0; i < MAX_TOKENS_PER_COMMAND; i ++)
	{
		//If the node at the end of the command is not NULL, we're missing arguments!
		if(m_command[i].IsEmpty())
		{
			if(node != NULL)
			{
				if(i > 0)
					m_output->Printf("Incomplete command: \"%s\" expects arguments\n", m_command[i-1].m_text);
				return false;
			}

			break;
		}

		m_command[i].m_commandID = INVALID_COMMAND;

		for(auto row = node; row->keyword != NULL; row++)
		{
			//Wildcards always match
			if(row->id == FREEFORM_TOKEN)
			{
			}

			else
			{
				//If the token doesn't match the prefix, we're definitely not a hit
				if(!m_command[i].PrefixMatch(row->keyword))
					continue;

				//If it matches, but the subsequent token matches too, the command is ambiguous!
				//Fail with an error.
				if(m_command[i].PrefixMatch(row[1].keyword))
				{
					m_output->Printf("Ambiguous command: \"%s\" could mean \"%s\" or \"%s\"\n",
						m_command[i].m_text,
						row->keyword,
						row[1].keyword);
					return false;
				}
			}

			//Match!
			m_command[i].m_commandID = row->id;
			node = row->children;
		}

		//Didn't match anything at all, give up
		if(m_command[i].m_commandID == INVALID_COMMAND)
		{
			m_output->Printf("Unrecognized command: \"%s\"\n", m_command[i].m_text);
			return false;
		}

	}

	//all good
	return true;
}
