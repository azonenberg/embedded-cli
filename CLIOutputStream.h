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
	@brief Declaration of CLIOutputStream
 */
#ifndef CLIOutputStream_h
#define CLIOutputStream_h

/**
	@brief An output stream for text content

	May be backed by a UART, socket, SSH session, or something else.

	Implementations of this class are expected to perform translation from \n to \r\n if required by the
	underlying output device.

	Provides a minimal printf-compatible output formatting helper that does not actually call the libc printf.
	This is important since printf in most embedded libc's can trigger a dynamic allocation.
 */
class CLIOutputStream
{
public:

	virtual ~CLIOutputStream();

	void Backspace()
	{ PutString("\b \b"); }

	void CursorLeft()
	{ PutString("\x1b[D"); }

	void CursorRight()
	{ PutString("\x1b[C"); }

	/**
		@brief Prints a single character
	 */
	virtual void PutCharacter(char ch) =0;

	/**
		@brief Prints a string with no formatting
	 */
	virtual void PutString(const char* str) =0;

	void WritePadded(const char* str, int minlen, char padding, int prepad);
	void Printf(const char* format, ...);

	/**
		@brief Flushes pending content so that it's displayed to the user.
	 */
	virtual void Flush() =0;
};

#endif
