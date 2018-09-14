//
//    Copyright (C) Microsoft.  All rights reserved.
//
#define DEFINE_CONSOLEV2_PROPERTIES

// System headers
#include <windows.h>
#include <Wincon.h>

// Standard library C-style
#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING  0x04
#endif

#define ESC "\x1b"
#define CSI "\x1b["

bool EnableVTMode()
{
	// Set output mode to handle virtual terminal sequences
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	DWORD dwMode = 0;
	if (!GetConsoleMode(hOut, &dwMode))
	{
		return false;
	}

	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	if (!SetConsoleMode(hOut, dwMode))
	{
		return false;
	}
	return true;
}

void PrintVerticalBorder()
{
	printf(ESC "(0");       // Enter Line drawing mode
	printf(CSI "104;93m");   // bright yellow on bright blue
	printf("x");            // in line drawing mode, \x78 -> \u2502 "Vertical Bar"
	printf(CSI "0m");       // restore color
	printf(ESC "(B");       // exit line drawing mode
}

void PrintHorizontalBorder(COORD const Size, bool fIsTop)
{
	printf(ESC "(0");       // Enter Line drawing mode
	printf(CSI "104;93m");  // Make the border bright yellow on bright blue
	printf(fIsTop ? "l" : "m"); // print left corner 

	for (int i = 1; i < Size.X - 1; i++)
		printf("q"); // in line drawing mode, \x71 -> \u2500 "HORIZONTAL SCAN LINE-5"

	printf(fIsTop ? "k" : "j"); // print right corner
	printf(CSI "0m");
	printf(ESC "(B");       // exit line drawing mode
}

void PrintStatusLine(char* const pszMessage, COORD const Size)
{
	printf(CSI "%d;1H", Size.Y);
	printf(CSI "K"); // clear the line
	printf(pszMessage);
}

int main(int argc, char* argv[])
{
	argc; // unused
	argv; // unused
		  //First, enable VT mode
	bool fSuccess = EnableVTMode();
	if (!fSuccess)
	{
		printf("Unable to enter VT processing mode. Quitting.\n");
		return -1;
	}
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE)
	{
		printf("Couldn't get the console handle. Quitting.\n");
		return -1;
	}

	CONSOLE_SCREEN_BUFFER_INFO ScreenBufferInfo;
	GetConsoleScreenBufferInfo(hOut, &ScreenBufferInfo);
	COORD Size;
	Size.X = ScreenBufferInfo.srWindow.Right - ScreenBufferInfo.srWindow.Left + 1;
	Size.Y = ScreenBufferInfo.srWindow.Bottom - ScreenBufferInfo.srWindow.Top + 1;

	// Enter the alternate buffer
	printf(CSI "?1049h");

	// Clear screen, tab stops, set, stop at columns 16, 32
	printf(CSI "1;1H");
	printf(CSI "2J"); // Clear screen

	int iNumTabStops = 4; // (0, 20, 40, width)
	printf(CSI "3g"); // clear all tab stops
	printf(CSI "1;20H"); // Move to column 20
	printf(ESC "H"); // set a tab stop

	printf(CSI "1;40H"); // Move to column 40
	printf(ESC "H"); // set a tab stop

					 // Set scrolling margins to 3, h-2
	printf(CSI "3;%dr", Size.Y - 2);
	int iNumLines = Size.Y - 4;

	printf(CSI "1;1H");
	printf(CSI "102;30m");
	printf("Windows 10 Anniversary Update - VT Example");
	printf(CSI "0m");

	// Print a top border - Yellow
	printf(CSI "2;1H");
	PrintHorizontalBorder(Size, true);

	// // Print a bottom border
	printf(CSI "%d;1H", Size.Y - 1);
	PrintHorizontalBorder(Size, false);

	wchar_t wch;

	// draw columns
	printf(CSI "3;1H");
	int line = 0;
	for (line = 0; line < iNumLines * iNumTabStops; line++)
	{
		PrintVerticalBorder();
		if (line + 1 != iNumLines * iNumTabStops) // don't advance to next line if this is the last line
			printf("\t"); // advance to next tab stop

	}

	PrintStatusLine("Press any key to see text printed between tab stops.", Size);
	wch = _getwch();

	// Fill columns with output
	printf(CSI "3;1H");
	for (line = 0; line < iNumLines; line++)
	{
		int tab = 0;
		for (tab = 0; tab < iNumTabStops - 1; tab++)
		{
			PrintVerticalBorder();
			printf("line=%d", line);
			printf("\t"); // advance to next tab stop
		}
		PrintVerticalBorder();// print border at right side
		if (line + 1 != iNumLines)
			printf("\t"); // advance to next tab stop, (on the next line)
	}

	PrintStatusLine("Press any key to demonstrate scroll margins", Size);
	wch = _getwch();

	printf(CSI "3;1H");
	for (line = 0; line < iNumLines * 2; line++)
	{
		printf(CSI "K"); // clear the line
		int tab = 0;
		for (tab = 0; tab < iNumTabStops - 1; tab++)
		{
			PrintVerticalBorder();
			printf("line=%d", line);
			printf("\t"); // advance to next tab stop
		}
		PrintVerticalBorder(); // print border at right side
		if (line + 1 != iNumLines * 2)
		{
			printf("\n"); //Advance to next line. If we're at the bottom of the margins, the text will scroll.
			printf("\r"); //return to first col in buffer
		}
	}

	PrintStatusLine("Press any key to exit", Size);
	wch = _getwch();

	// Exit the alternate buffer
	printf(CSI "?1049h");

}
