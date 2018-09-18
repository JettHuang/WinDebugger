// \brief
//		console scrolling: window(view), screen-buffer
//

#if 0 // Scrolling a Screen Buffer's Contents

#include <windows.h>
#include <stdio.h>

int main(void)
{
	HANDLE hStdout;
	CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
	SMALL_RECT srctScrollRect, srctClipRect;
	CHAR_INFO chiFill;
	COORD coordDest;
	int i;

	printf("\nPrinting 20 lines for reference. ");
	printf("Notice that line 6 is discarded during scrolling.\n");
	for (i = 0; i <= 20; i++)
		printf("%d\n", i);

	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

	if (hStdout == INVALID_HANDLE_VALUE)
	{
		printf("GetStdHandle failed with %d\n", GetLastError());
		return 1;
	}

	// Get the screen buffer size. 

	if (!GetConsoleScreenBufferInfo(hStdout, &csbiInfo))
	{
		printf("GetConsoleScreenBufferInfo failed %d\n", GetLastError());
		return 1;
	}

	// The scrolling rectangle is the bottom 15 rows of the 
	// screen buffer. 

	srctScrollRect.Top = csbiInfo.dwCursorPosition.Y - 15;
	srctScrollRect.Bottom = csbiInfo.dwCursorPosition.Y - 1;
	srctScrollRect.Left = 0;
	srctScrollRect.Right = csbiInfo.dwSize.X - 1;

	// The destination for the scroll rectangle is one row up. 

	coordDest.X = 0;
	coordDest.Y = csbiInfo.dwCursorPosition.Y - 16;

	// The clipping rectangle is the same as the scrolling rectangle. 
	// The destination row is left unchanged. 

	srctClipRect = srctScrollRect;

	// Fill the bottom row with green blanks. 

	chiFill.Attributes = BACKGROUND_GREEN | FOREGROUND_RED;
	chiFill.Char.UnicodeChar = TEXT(' ');

	// Scroll up one line. 

	if (!ScrollConsoleScreenBuffer(
		hStdout,         // screen buffer handle 
		&srctScrollRect, // scrolling rectangle 
		&srctClipRect,   // clipping rectangle 
		coordDest,       // top left destination cell 
		&chiFill))       // fill character and color
	{
		printf("ScrollConsoleScreenBuffer failed %d\n", GetLastError());
		return 1;
	}
	return 0;
}

#else // Scrolling a Screen Buffer's Window

#include <windows.h>
#include <stdio.h>
#include <conio.h>

HANDLE hStdout;

int ScrollByAbsoluteCoord(int iRows)
{
	CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
	SMALL_RECT srctWindow;

	// Get the current screen buffer size and window position. 

	if (!GetConsoleScreenBufferInfo(hStdout, &csbiInfo))
	{
		printf("GetConsoleScreenBufferInfo (%d)\n", GetLastError());
		return 0;
	}

	// Set srctWindow to the current window size and location. 

	srctWindow = csbiInfo.srWindow;

	// Check whether the window is too close to the screen buffer top

	if (srctWindow.Top >= iRows)
	{
		srctWindow.Top -= (SHORT)iRows;     // move top up
		srctWindow.Bottom -= (SHORT)iRows;  // move bottom up

		if (!SetConsoleWindowInfo(
			hStdout,          // screen buffer handle 
			TRUE,             // absolute coordinates 
			&srctWindow))     // specifies new location 
		{
			printf("SetConsoleWindowInfo (%d)\n", GetLastError());
			return 0;
		}
		return iRows;
	}
	else
	{
		printf("\nCannot scroll; the window is too close to the top.\n");
		return 0;
	}
}

int ScrollByRelativeCoord(int iRows)
{
	CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
	SMALL_RECT srctWindow;

	// Get the current screen buffer window position. 

	if (!GetConsoleScreenBufferInfo(hStdout, &csbiInfo))
	{
		printf("GetConsoleScreenBufferInfo (%d)\n", GetLastError());
		return 0;
	}

	// Check whether the window is too close to the screen buffer top

	if (csbiInfo.srWindow.Top >= iRows)
	{
		srctWindow.Top = -(SHORT)iRows;     // move top up
		srctWindow.Bottom = -(SHORT)iRows;  // move bottom up 
		srctWindow.Left = 0;         // no change 
		srctWindow.Right = 0;        // no change 

		if (!SetConsoleWindowInfo(
			hStdout,          // screen buffer handle 
			FALSE,            // relative coordinates
			&srctWindow))     // specifies new location 
		{
			printf("SetConsoleWindowInfo (%d)\n", GetLastError());
			return 0;
		}
		return iRows;
	}
	else
	{
		printf("\nCannot scroll; the window is too close to the top.\n");
		return 0;
	}
}

int main(void)
{
	int i;

	printf("\nPrinting 120 lines, then scrolling up five lines.\n");
	printf("Press any key to scroll up ten lines; ");
	printf("then press another key to stop the demo.\n");
	for (i = 0; i <= 120; i++)
		printf("%d\n", i);

	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

	if (ScrollByAbsoluteCoord(5))
		_getch();
	else return 0;

	if (ScrollByRelativeCoord(10))
		_getch();
	else return 0;
}

#endif
