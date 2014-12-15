/************************************
*  	TimeVertor
*	Copyright © 2012 Henry++
*
*	GNU General Public License v2
*	http://www.gnu.org/licenses/
*
*	http://www.henrypp.org/
*************************************/

#ifndef __TIMEVERTOR_H__
#define __TIMEVERTOR_H__

// Define
#define APP_NAME L"TimeVertor"
#define APP_NAME_SHORT L"timevertor"
#define APP_VERSION L"1.0"
#define APP_VERSION_RES 1,0
#define APP_HOST L"www.henrypp.org"
#define APP_WEBSITE L"http://" APP_HOST

// Settings Structure
struct CONFIG
{
	HWND hWnd; // main window handle

	HINSTANCE hLocale; // language module handle
	HFONT hBold; // bold font for titles

	WCHAR szCurrentDir[MAX_PATH]; // current directory
};

#endif // __TIMEVERTOR_H__
