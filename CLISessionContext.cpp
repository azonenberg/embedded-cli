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
		return;
	}

	//Newline? Execute the command
	else if( (c == '\r') || (c == '\n') )
	{
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

	//All done with whatever we're printing, flush stdout
	m_output->Flush();

	DebugPrint();
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

	m_output->PutCharacter('\n');
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

	/*
	//Go through each token and figure out if it matches anything we know about
	const clikeyword_t* node = root;
	for(size_t i = 0; i < MAX_TOKENS; i ++)
	{
		if(m_command[i].IsEmpty())
		{
			//If the node is not NULL, we're missing arguments!
			if(node != NULL)
			{
				m_uart->Printf("\nIncomplete command\n");
				return false;
			}

			break;
		}

		//Debug print
		//m_uart->Printf("    %d: %s ", i, m_command[i].m_text);

		m_command[i].m_commandID = CMD_NULL;

		for(const clikeyword_t* row = node; row->keyword != NULL; row++)
		{
			//TODO: handle wildcards

			//If the token doesn't match the prefix, we're definitely not a hit
			if(!m_command[i].PrefixMatch(row->keyword))
				continue;

			//If it matches, but the subsequent token matches too, the command is ambiguous!
			//Fail with an error.
			if(m_command[i].PrefixMatch(row[1].keyword))
			{
				m_uart->Printf(
					"\nAmbiguous command (at word %d): you typed \"%s\", but this could be short for \"%s\" or \"%s\"\n",
					i,
					m_command[i].m_text,
					row->keyword,
					row[1].keyword);
				return false;
			}

			//Debug print
			//m_uart->Printf("(matched \"%s\")\n", row->keyword);

			//Match!
			m_command[i].m_commandID = row->id;
			node = row->children;
		}

		//Didn't match anything at all, give up
		if(m_command[i].m_commandID == CMD_NULL)
		{
			m_uart->Printf(
				"\nUnrecognized command (at word %d): you typed \"%s\", but this did not match any known commands\n",
				i,
				m_command[i].m_text);
			return false;
		}

	}

	//If we get here, all good
	return true;
	*/

	//all good
	return true;
}