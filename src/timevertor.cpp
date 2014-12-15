/************************************
*  	TimeVertor
*	Copyright © 2012 Henry++
*
*	GNU General Public License v2
*	http://www.gnu.org/licenses/
*
*	http://www.henrypp.org/
*************************************/

// Include
#include <windows.h>
#include <commctrl.h>
#include <wininet.h>
#include <shlobj.h>
#include <time.h>
#include <atlstr.h> // cstring
#include <process.h> // _beginthreadex

#include "timevertor.h"
#include "routine.h"
#include "resource.h"
#include "ini.h"

INI ini;
CONFIG cfg = {0};
CONST UINT WM_MUTEX = RegisterWindowMessage(APP_NAME_SHORT);

// Check Updates
UINT WINAPI CheckUpdates(LPVOID lpParam)
{
	BOOL bStatus = 0;
	HINTERNET hInternet = 0, hConnect = 0;

	// Disable Menu
	EnableMenuItem(GetMenu(cfg.hWnd), IDM_CHECK_UPDATES, MF_BYCOMMAND | MF_DISABLED);

	// Connect
	if((hInternet = InternetOpen(APP_NAME L"/" APP_VERSION L" (+" APP_WEBSITE L")", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0)) && (hConnect = InternetOpenUrl(hInternet, APP_WEBSITE L"/update.php?product=" APP_NAME_SHORT, NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0)))
	{
		// Get Status
		DWORD dwStatus = 0, dwStatusSize = sizeof(dwStatus);
		HttpQueryInfo(hConnect, HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_STATUS_CODE, &dwStatus, &dwStatusSize, NULL);

		// Check Errors
		if(dwStatus == HTTP_STATUS_OK)
		{
			// Reading
			ULONG ulReaded = 0;
			CHAR szBufferA[MAX_PATH] = {0};

			if(InternetReadFile(hConnect, szBufferA, MAX_PATH, &ulReaded) && ulReaded)
			{
				// Convert to Unicode
				CA2W newver(szBufferA, CP_UTF8);

				// If NEWVER == CURVER
				if(lstrcmpi(newver, APP_VERSION) == 0)
				{
					if(!lpParam)
						MessageBox(cfg.hWnd, MB_OK | MB_ICONINFORMATION, APP_NAME, ls(cfg.hLocale, IDS_UPDATE_NO));
				}
				else
				{
					if(MessageBox(cfg.hWnd, MB_YESNO | MB_ICONQUESTION, APP_NAME, ls(cfg.hLocale, IDS_UPDATE_YES), newver) == IDYES)
						ShellExecute(cfg.hWnd, 0, APP_WEBSITE L"/?product=" APP_NAME_SHORT, NULL, NULL, SW_SHOWDEFAULT);
				}

				// Switch Result
				bStatus = 1;
			}
		}
	}

	if(!bStatus && !lpParam)
		MessageBox(cfg.hWnd, MB_OK | MB_ICONSTOP, APP_NAME, ls(cfg.hLocale, IDS_UPDATE_ERROR));

	// Restore Menu
	EnableMenuItem(GetMenu(cfg.hWnd), IDM_CHECK_UPDATES, MF_BYCOMMAND | MF_ENABLED);

	// Clear Memory
	InternetCloseHandle(hConnect);
	InternetCloseHandle(hInternet);

	return bStatus;
}

// Read and Apply Routine Settings
void ApplySettings(HWND hWnd)
{
	// Always on Top
	SetWindowPos(hWnd, (ini.read(APP_NAME_SHORT, L"AlwaysOnTop", 0) ? HWND_TOPMOST : HWND_NOTOPMOST), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	
	// DateTime Picker's re-style
	DWORD dwStyle = GetWindowLongPtr(GetDlgItem(hWnd, IDC_WINDOWS_TIME), GWL_STYLE);

	if(ini.read(APP_NAME_SHORT, L"EnableWindowsEdit", 0))
		dwStyle |= DTS_APPCANPARSE;
	else
		dwStyle &= ~DTS_APPCANPARSE;

	SetWindowLongPtr(GetDlgItem(hWnd, IDC_WINDOWS_TIME), GWL_STYLE, dwStyle);

	// Format Changer
	SendDlgItemMessage(hWnd, IDC_WINDOWS_TIME, DTM_SETFORMAT, 0, ini.read(APP_NAME_SHORT, L"EnableWindowsFormat", 1) ? (LPARAM)ini.read(APP_NAME_SHORT, L"WindowsFormat", MAX_PATH, L"d MMMM yyyy (HH:mm:ss)").GetBuffer() : NULL);
}

// Covert CString (like as "10,20,30,40") to INT Array
void StringToIntArray(CString string, LPCWSTR lpcszToken, INT iOutArray[], const INT iCount)
{
	INT i = 0, j = 0;
	CString token = string.Tokenize(lpcszToken, i);

	while(!token.IsEmpty())
	{
		// check array range
		if(j >= iCount)
			break;

		iOutArray[j++] = _wtoi(token);

		token = string.Tokenize(lpcszToken, i);
	}
}

CString UnixTimeToString(time_t tUnixTime)
{
	CString buffer;

	buffer.Format(L"%lld\0", tUnixTime);

	return buffer;
}

INT_PTR CALLBACK SettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CString buffer;
	INT iBuffer = 0;
	RECT rc = {0};

	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			// Centering By Parent
			CenterDialog(hwndDlg);

			// Use Bold Font for Label
			SendDlgItemMessage(hwndDlg, IDC_LABEL_1, WM_SETFONT, (WPARAM)cfg.hBold, 0);
			SendDlgItemMessage(hwndDlg, IDC_LABEL_2, WM_SETFONT, (WPARAM)cfg.hBold, 0);
			SendDlgItemMessage(hwndDlg, IDC_LABEL_3, WM_SETFONT, (WPARAM)cfg.hBold, 0);

			// Main (Section)
			CheckDlgButton(hwndDlg, IDC_CHECK_UPDATE_AT_STARTUP_CHK, ini.read(APP_NAME_SHORT, L"CheckUpdateAtStartup", 1) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_ALWAYS_ON_TOP_CHK, ini.read(APP_NAME_SHORT, L"AlwaysOnTop", 0) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_ENABLE_WINDOWS_EDIT_CHK, ini.read(APP_NAME_SHORT, L"EnableWindowsEdit", 0) ? BST_CHECKED : BST_UNCHECKED);

			// Format (Section)
			CheckDlgButton(hwndDlg, IDC_ENABLE_FORMAT_CHK, ini.read(APP_NAME_SHORT, L"EnableWindowsFormat", 1) ? BST_CHECKED : BST_UNCHECKED);
			SetDlgItemText(hwndDlg, IDC_WINDOWS_FORMAT_EDIT, ini.read(APP_NAME_SHORT, L"WindowsFormat", MAX_PATH, L"d MMMM yyyy (HH:mm:ss)"));

			SendMessage(SetDlgItemTooltip(hwndDlg, IDC_WINDOWS_FORMAT_EDIT, ls(cfg.hLocale, IDS_WINDOWS_FORMAT).GetBuffer()), TTM_SETMAXTIPWIDTH, 0, 0); // set tooltip and switch to multiline

			// Language (Section)
			SendDlgItemMessage(hwndDlg, IDC_LANGUAGE_CB, CB_ADDSTRING, 0, (LPARAM)L"English (default)");
			buffer.Format(L"%s\\Languages\\*.dll", cfg.szCurrentDir);

			WIN32_FIND_DATA wfd = {0};
			HANDLE hFind = FindFirstFile(buffer, &wfd);

			if(hFind != INVALID_HANDLE_VALUE)
			{
				do {
					if(!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
					{
						PathRemoveExtension(wfd.cFileName);
						SendDlgItemMessage(hwndDlg, IDC_LANGUAGE_CB, CB_ADDSTRING, 0, (LPARAM)wfd.cFileName);
					}
				} while(FindNextFile(hFind, &wfd));

				FindClose(hFind);
			}
			else
			{
				EnableWindow(GetDlgItem(hwndDlg, IDC_LANGUAGE_CB), FALSE);
			}

			if(SendDlgItemMessage(hwndDlg, IDC_LANGUAGE_CB, CB_SELECTSTRING, 1, (LPARAM)ini.read(APP_NAME_SHORT, L"Language", MAX_PATH, 0).GetBuffer()) == CB_ERR)
				SendDlgItemMessage(hwndDlg, IDC_LANGUAGE_CB, CB_SETCURSEL, 0, 0);

			// Simulate WM_COMMAND
			SendMessage(hwndDlg, WM_COMMAND, MAKELPARAM(IDC_ENABLE_FORMAT_CHK, 0), 0);
			SendMessage(hwndDlg, WM_COMMAND, MAKELPARAM(IDC_LANGUAGE_CB, CBN_SELENDOK), 0);

			// Disable Button's
			EnableWindow(GetDlgItem(hwndDlg, IDC_OK), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_WINDOWS_FORMAT_STATIC), 0);

			break;
		}

		case WM_CLOSE:
		{
			EndDialog(hwndDlg, 0);
			break;
		}

		case WM_PAINT:
		{
			PAINTSTRUCT ps = {0};
			HDC hDC = BeginPaint(hwndDlg, &ps);

			GetClientRect(hwndDlg, &rc);
			rc.top = rc.bottom - 43;

			// Instead FillRect
			COLORREF clrOld = SetBkColor(hDC, GetSysColor(COLOR_BTNFACE));
			ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
			SetBkColor(hDC, clrOld);

			// Draw Line
			for(int i = 0; i < rc.right; i++)
				SetPixel(hDC, i, rc.top, GetSysColor(COLOR_BTNSHADOW));

			EndPaint(hwndDlg, &ps);

			return 0;
		}

		case WM_CTLCOLORSTATIC:
		case WM_CTLCOLORDLG:
		{
			return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
		}

		case WM_NOTIFY:
		{
			switch(((LPNMHDR)lParam)->code)
			{
				case NM_CLICK:
				case NM_RETURN:
				{
					NMLINK* nmlink = (NMLINK*)lParam;
					
					if(lstrlen(nmlink->item.szUrl))
						ShellExecute(hwndDlg, 0, nmlink->item.szUrl, 0, 0, SW_SHOW);

					break;
				}
			}

			break;
		}

		case WM_COMMAND:
		{
			if(lParam && (HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == EN_CHANGE || HIWORD(wParam) == CBN_SELENDOK))
				EnableWindow(GetDlgItem(hwndDlg, IDC_OK), TRUE);

			if(HIWORD(wParam) == CBN_SELENDOK && LOWORD(wParam) == IDC_LANGUAGE_CB)
			{
				HINSTANCE hModule = NULL;
				WCHAR szBuffer[MAX_PATH] = {0};

				iBuffer = (SendDlgItemMessage(hwndDlg, IDC_LANGUAGE_CB, CB_GETCURSEL, 0, 0) > 0);

				if(iBuffer)
				{
					GetDlgItemText(hwndDlg, IDC_LANGUAGE_CB, szBuffer, MAX_PATH);
					buffer.Format(L"%s\\Languages\\%s.dll", cfg.szCurrentDir, szBuffer);

					hModule = LoadLibraryEx(buffer, 0, LOAD_LIBRARY_AS_DATAFILE);
				}

				SetDlgItemText(hwndDlg, IDC_LANGUAGE_INFO, (iBuffer && !hModule) ? szBuffer : ls(hModule, IDS_TRANSLATION_INFO));

				if(hModule)
					FreeLibrary(hModule);
			}

			switch(LOWORD(wParam))
			{
				case IDC_ENABLE_FORMAT_CHK:
				{
					EnableWindow(GetDlgItem(hwndDlg, IDC_WINDOWS_FORMAT_EDIT), IsDlgButtonChecked(hwndDlg, IDC_ENABLE_FORMAT_CHK) == BST_CHECKED);
					break;
				}

				case IDC_OK:
				{
					if((IsDlgButtonChecked(hwndDlg, IDC_ENABLE_FORMAT_CHK) == BST_CHECKED) && (!SendDlgItemMessage(hwndDlg, IDC_WINDOWS_FORMAT_EDIT, WM_GETTEXTLENGTH, 0, 0)))
					{
						ShowEditBalloonTip(hwndDlg, IDC_WINDOWS_FORMAT_EDIT, NULL, ls(cfg.hLocale, IDS_REQUIRED_FIELD), 0);
						return 0;
					}

					// Main (Section)
					ini.write(APP_NAME_SHORT, L"CheckUpdateAtStartup", (IsDlgButtonChecked(hwndDlg, IDC_CHECK_UPDATE_AT_STARTUP_CHK) == BST_CHECKED));
					ini.write(APP_NAME_SHORT, L"AlwaysOnTop", (IsDlgButtonChecked(hwndDlg, IDC_ALWAYS_ON_TOP_CHK) == BST_CHECKED));
					ini.write(APP_NAME_SHORT, L"EnableWindowsEdit", (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_WINDOWS_EDIT_CHK) == BST_CHECKED));

					// Format (Section)
					ini.write(APP_NAME_SHORT, L"EnableWindowsFormat", (IsDlgButtonChecked(hwndDlg, IDC_ENABLE_FORMAT_CHK) == BST_CHECKED));
					
					GetDlgItemText(hwndDlg, IDC_WINDOWS_FORMAT_EDIT, buffer.GetBuffer(MAX_PATH), MAX_PATH); buffer.ReleaseBuffer();
					ini.write(APP_NAME_SHORT, L"WindowsFormat", buffer);

					// Language (Section)
					iBuffer = SendDlgItemMessage(hwndDlg, IDC_LANGUAGE_CB, CB_GETCURSEL, 0, 0);
						
					if(cfg.hLocale)
						FreeLibrary(cfg.hLocale);

					if(iBuffer <= 0)
					{
						ini.write(APP_NAME_SHORT, L"Language", (DWORD)0);
						cfg.hLocale = 0;
					}
					else
					{
						wchar_t szBuffer[MAX_PATH] = {0};

						GetDlgItemText(hwndDlg, IDC_LANGUAGE_CB, szBuffer, MAX_PATH);
						ini.write(APP_NAME_SHORT, L"Language", szBuffer);

						buffer.Format(L"%s\\Languages\\%s.dll", cfg.szCurrentDir, szBuffer);
						cfg.hLocale = LoadLibraryEx(buffer, 0, LOAD_LIBRARY_AS_DATAFILE);
					}

					// Apply All Settings
					ApplySettings(GetParent(hwndDlg));

					// Disable Button
					EnableWindow(GetDlgItem(hwndDlg, IDC_OK), FALSE);

					break;
				}

				case IDCANCEL: // process Esc key
				case IDC_CANCEL:
				{
					SendMessage(hwndDlg, WM_CLOSE, 0, 0);
					break;
				}
			}

			break;
		}
	}

	return 0;
}

LRESULT CALLBACK DlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CString buffer;
	SYSTEMTIME st = {0};

	if(uMsg == WM_MUTEX)
		return WmMutexWrapper(hwndDlg, wParam, lParam);

	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			// Check Mutex
			CreateMutex(NULL, TRUE, APP_NAME_SHORT);

			if(GetLastError() == ERROR_ALREADY_EXISTS)
			{
				PostMessage(HWND_BROADCAST, WM_MUTEX, GetCurrentProcessId(), 1);
				SendMessage(hwndDlg, WM_CLOSE, 0, 0);

				return 0;
			}

			// Set Window Title
			SetWindowText(hwndDlg, APP_NAME L" " APP_VERSION);

			// Set Icons
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(IDI_MAIN), IMAGE_ICON, 32, 32, 0));
			SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(IDI_MAIN), IMAGE_ICON, 16, 16, 0));

			// Modify System Menu
			HMENU hMenu = GetSystemMenu(hwndDlg, 0);
			InsertMenu(hMenu, -1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
			InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, IDM_ABOUT, ls(cfg.hLocale, IDS_ABOUT));

			// Create Bold Font
			LOGFONT lf = {0};
			GetObject((HFONT)SendMessage(hwndDlg, WM_GETFONT, 0, 0), sizeof(lf), &lf);
			lf.lfWeight = FW_BOLD;

			cfg.hBold = CreateFontIndirect(&lf);

			// Conversion Mode
			CheckDlgButton(hwndDlg, ini.read(APP_NAME_SHORT, L"ConversionMode", 0) == 1 ? IDC_UNIX_RB : IDC_WINDOWS_RB, BST_CHECKED);
			SendMessage(hwndDlg, WM_COMMAND, MAKELPARAM(IsDlgButtonChecked(hwndDlg, IDC_WINDOWS_RB) ? IDC_WINDOWS_RB : IDC_UNIX_RB, 0), 0);

			// Set Tooltips
			SetDlgItemTooltip(hwndDlg, IDC_WINDOWS_CURRENT_BTN, ls(cfg.hLocale, IDS_CURRENT_TIME).GetBuffer());
			SetDlgItemTooltip(hwndDlg, IDC_UNIX_CURRENT_BTN, ls(cfg.hLocale, IDS_CURRENT_TIME).GetBuffer());

			SetDlgItemTooltip(hwndDlg, IDC_WINDOWS_RB, ls(cfg.hLocale, IDS_W2U).GetBuffer());
			SetDlgItemTooltip(hwndDlg, IDC_UNIX_RB, ls(cfg.hLocale, IDS_U2W).GetBuffer());

			// Restore Last Time Values
			INT iArray[6] = {0};
			StringToIntArray(ini.read(APP_NAME_SHORT, L"LatestWindowsTime", MAX_PATH, NULL), L",", iArray, 6);
			
			for(int i = 0; i < (sizeof(iArray) / sizeof(iArray[0])); i++)
			{
				if(iArray[i])
				{
					st.wDay = iArray[0];
					st.wMonth = iArray[1];
					st.wYear = iArray[2];
					st.wHour = iArray[3];
					st.wMinute = iArray[4];
					st.wSecond = iArray[5];

					SendDlgItemMessage(hwndDlg, IDC_WINDOWS_TIME, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);

					break;
				}
			}

			SetDlgItemText(hwndDlg, IDC_UNIX_TIME, ini.read(APP_NAME_SHORT, L"LatestUnixTime", MAX_PATH, NULL));

			// Apply Routine Settings
			ApplySettings(hwndDlg);

			// Check Updates
			if(ini.read(APP_NAME_SHORT, L"CheckUpdateAtStartup", 1))
				_beginthreadex(NULL, 0, &CheckUpdates, (LPVOID)1, 0, NULL);

			break;
		}

		case WM_CLOSE:
		{
			// Destroy Resources
			if(cfg.hBold)
				DeleteObject(cfg.hBold);

			if(cfg.hLocale)
				FreeLibrary(cfg.hLocale);

			// Save Settings
			ini.write(APP_NAME_SHORT, L"ConversionMode", IsDlgButtonChecked(hwndDlg, IDC_WINDOWS_RB) ? 0 : 1);

			SendDlgItemMessage(hwndDlg, IDC_WINDOWS_TIME, DTM_GETSYSTEMTIME, 0, (LPARAM)&st);
			buffer.Format(L"%d,%d,%d,%d,%d,%d", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);
			ini.write(APP_NAME_SHORT, L"LatestWindowsTime", buffer);

			GetDlgItemText(hwndDlg, IDC_UNIX_TIME, buffer.GetBuffer(MAX_PATH), MAX_PATH); buffer.ReleaseBuffer();
			ini.write(APP_NAME_SHORT, L"LatestUnixTime", buffer);
			
			// Destroy Window and Quit
			DestroyWindow(hwndDlg);
			PostQuitMessage(0);

			break;
		}

		case WM_PAINT:
		{
			RECT rc = {0};
			PAINTSTRUCT ps = {0};
			HDC hDC = BeginPaint(hwndDlg, &ps);

			GetClientRect(hwndDlg, &rc);
			rc.top = rc.bottom - 43;

			// Instead FillRect
			COLORREF clrOld = SetBkColor(hDC, GetSysColor(COLOR_BTNFACE));
			ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
			SetBkColor(hDC, clrOld);

			// Draw Line
			for(int i = 0; i < rc.right; i++)
				SetPixel(hDC, i, rc.top, GetSysColor(COLOR_BTNSHADOW));

			EndPaint(hwndDlg, &ps);

			return 0;
		}

		case WM_CTLCOLORSTATIC:
		case WM_CTLCOLORDLG:
		{
			return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
		}

		case WM_SYSCOMMAND:
		{
			if(wParam == IDM_ABOUT)
				SendMessage(hwndDlg, WM_COMMAND, MAKELPARAM(IDM_ABOUT, 0), 0);

			break;
		}

		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case IDM_SETTINGS:
				{
					DialogBox(cfg.hLocale, MAKEINTRESOURCE(IDD_SETTINGS), hwndDlg, SettingsDlgProc);
					break;
				}

				case IDCANCEL: // process Esc key
				case IDM_EXIT:
				{
					SendMessage(hwndDlg, WM_CLOSE, 0, 0);
					break;
				}

				case IDM_COPY_WINDOWS:
				case IDM_COPY_UNIX:
				{
					if(LOWORD(wParam) == IDM_COPY_UNIX && !SendDlgItemMessage(hwndDlg, IDC_UNIX_TIME, WM_GETTEXTLENGTH, 0, 0))
					{
						ShowEditBalloonTip(hwndDlg, IDC_UNIX_TIME, NULL, ls(cfg.hLocale, IDS_REQUIRED_FIELD), 0);
						return 0;
					}

					GetDlgItemText(hwndDlg, LOWORD(wParam) == IDM_COPY_WINDOWS ? IDC_WINDOWS_TIME : IDC_UNIX_TIME, buffer.GetBuffer(MAX_PATH), MAX_PATH); buffer.ReleaseBuffer();
					ClipboardPut(buffer);

					break;
				}

				case IDM_WEBSITE:
				{
					ShellExecute(hwndDlg, 0, APP_WEBSITE, NULL, NULL, SW_SHOWDEFAULT);
					break;
				}

				case IDM_CHECK_UPDATES:
				{
					_beginthreadex(NULL, 0, &CheckUpdates, 0, 0, NULL);
					break;
				}
				
				case IDM_ABOUT:
				{
					buffer.Format(ls(cfg.hLocale, IDS_COPYRIGHT), APP_WEBSITE, APP_HOST);
					AboutBoxCreate(hwndDlg, MAKEINTRESOURCE(IDI_MAIN), ls(cfg.hLocale, IDS_ABOUT), APP_NAME L" " APP_VERSION, L"Copyright © 2012 Henry++\r\nAll Rights Reversed\r\n\r\n" + buffer);

					break;
				}
				
				case IDC_WINDOWS_RB:
				case IDC_UNIX_RB:
				{
					EnableWindow(GetDlgItem(hwndDlg, IDC_WINDOWS_TIME), LOWORD(wParam) == IDC_WINDOWS_RB ? TRUE : FALSE);
					SendDlgItemMessage(hwndDlg, IDC_UNIX_TIME, EM_SETREADONLY, LOWORD(wParam) == IDC_UNIX_RB ? FALSE : TRUE, 0);

					break;
				}

				case IDC_WINDOWS_CURRENT_BTN:
				case IDC_UNIX_CURRENT_BTN:
				{
					GetLocalTime(&st);

					if(LOWORD(wParam) == IDC_WINDOWS_CURRENT_BTN)
						SendDlgItemMessage(hwndDlg, IDC_WINDOWS_TIME, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
					else
						SetDlgItemText(hwndDlg, IDC_UNIX_TIME, UnixTimeToString(SystemTimeToUnixTime(&st)));

					break;
				}

				case IDC_CONVERT:
				{
					if(IsDlgButtonChecked(hwndDlg, IDC_WINDOWS_RB))
					{
						SendDlgItemMessage(hwndDlg, IDC_WINDOWS_TIME, DTM_GETSYSTEMTIME, 0, (LPARAM)&st);
						SetDlgItemText(hwndDlg, IDC_UNIX_TIME, UnixTimeToString(SystemTimeToUnixTime(&st)));
					}
					else
					{
						if(!SendDlgItemMessage(hwndDlg, IDC_UNIX_TIME, WM_GETTEXTLENGTH, 0, 0))
						{
							ShowEditBalloonTip(hwndDlg, IDC_UNIX_TIME, NULL, ls(cfg.hLocale, IDS_REQUIRED_FIELD), 0);
						}
						else
						{
							UnixTimeToSystemTime((time_t)GetDlgItemInt(hwndDlg, IDC_UNIX_TIME, NULL, FALSE), &st);
							SendDlgItemMessage(hwndDlg, IDC_WINDOWS_TIME, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
						}
					}

					break;
				}
			}
		}
	}

	return 0;
}

INT APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR lpCmdLine, INT nShowCmd)
{
	CString buffer;

	// Load Settings
	GetModuleFileName(0, buffer.GetBuffer(MAX_PATH), MAX_PATH);
	PathRenameExtension(buffer.GetBuffer(MAX_PATH), L".cfg"); buffer.ReleaseBuffer();
	ini.load(buffer);

	// Current Dir
	PathRemoveFileSpec(buffer.GetBuffer(MAX_PATH)); buffer.ReleaseBuffer();
	StringCchCopy(cfg.szCurrentDir, MAX_PATH, buffer);

	// Language
	buffer.Format(L"%s\\Languages\\%s.dll", cfg.szCurrentDir, ini.read(APP_NAME_SHORT, L"Language", MAX_PATH, 0));

	if(FileExists(buffer))
		cfg.hLocale = LoadLibraryEx(buffer, 0, LOAD_LIBRARY_AS_DATAFILE);

	// Initialize and Create Window
	MSG msg = {0};
	INITCOMMONCONTROLSEX icex = {0};

	icex.dwSize = sizeof(icex);
	icex.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES;

	if(!InitCommonControlsEx(&icex))
		return 0;

	if(!(cfg.hWnd = CreateDialog(cfg.hLocale, MAKEINTRESOURCE(IDD_MAIN), NULL, (DLGPROC)DlgProc)))
		return 0;

	while(GetMessage(&msg, NULL, 0, 0))
	{
		if(!IsDialogMessage(cfg.hWnd, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	
	return msg.wParam;
}