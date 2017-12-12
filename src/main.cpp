// TimeVertor
// Copyright (c) 2012-2018 Henry++

#include <windows.h>
#include <algorithm>

#include "main.hpp"
#include "rapp.hpp"
#include "routine.hpp"

#include "resource.hpp"

rapp app (APP_NAME, APP_NAME_SHORT, APP_VERSION, APP_COPYRIGHT);

rstring _app_timezone2string (LONG bias, bool divide, LPCWSTR utcname)
{
	WCHAR result[32] = {0};

	if (bias == 0)
	{
		StringCchCopy (result, _countof (result), utcname);
	}
	else
	{
		const WCHAR symbol = ((bias > 0) ? L'-' : L'+');
		const u_int tzHours = (u_int)floor (long double ((abs (bias)) / 60.0));
		const u_int tzMinutes = (abs (bias) % 60);

		StringCchPrintf (result, _countof (result), L"%c%02u%s%02u", symbol, tzHours, divide ? L":" : L"", tzMinutes);
	}

	return result;
}

LONG _app_timegetdefaultbias ()
{
	DYNAMIC_TIME_ZONE_INFORMATION tz = {0};
	GetDynamicTimeZoneInformation (&tz);

	return tz.Bias;
}

LONG _app_timegetcurrentbias ()
{
	return app.ConfigGet (L"TimezoneBias", _app_timegetdefaultbias ()).AsLong ();
}

bool _app_timegetcurrent (LPSYSTEMTIME lpst)
{
	TIME_ZONE_INFORMATION tz = {0};
	tz.Bias = _app_timegetcurrentbias (); // set timezone shift

	SYSTEMTIME st = {0};
	GetSystemTime (&st);

	SystemTimeToTzSpecificLocalTime (&tz, &st, lpst);

	return true;
}

rstring _app_timeconvert (__time64_t ut, LPSYSTEMTIME lpst, PULARGE_INTEGER pul, EnumDateType type)
{
	rstring result;

	const LONG bias = _app_timegetcurrentbias ();

	if (type == TypeRfc2822)
		result.Format (L"%s, %02d %s %04d %02d:%02d:%02d %s", str_dayofweek[lpst->wDayOfWeek], lpst->wDay, str_month[lpst->wMonth - 1], lpst->wYear, lpst->wHour, lpst->wMinute, lpst->wSecond, _app_timezone2string (bias, false, L"GMT").GetString ());

	else if (type == TypeIso8601)
		result.Format (L"%04d-%02d-%02dT%02d:%02d:%02d%s", lpst->wYear, lpst->wMonth, lpst->wDay, lpst->wHour, lpst->wMinute, lpst->wSecond, _app_timezone2string (bias, true, L"Z").GetString ());

	else if (type == TypeUnixtime)
		result.Format (L"%llu", max (ut, 0));

	else if (type == TypeMactime)
		result.Format (L"%llu", max (ut + MAC_TIMESTAMP, 0));

	else if (type == TypeMicrosofttime)
		result.Format (L"%.09f", max ((double (pul->QuadPart) / (24.0 * (60.0 * (60.0 * 10000000.0)))) - MICROSOFT_TIMESTAMP, 0.0));

	else if (type == TypeFiletime)
		result.Format (L"%llu", max (pul->QuadPart, 0));

	return result;
}

rstring _app_timedescription (EnumDateType type, bool is_desc)
{
	rstring result;

	if (type == TypeRfc2822)
		result = is_desc ? I18N (&app, IDS_FMTDESC_RFC2822, 0) : I18N (&app, IDS_FMTNAME_RFC2822, 0);

	else if (type == TypeIso8601)
		result = is_desc ? I18N (&app, IDS_FMTDESC_ISO8601, 0) : I18N (&app, IDS_FMTNAME_ISO8601, 0);

	else if (type == TypeUnixtime)
		result = is_desc ? I18N (&app, IDS_FMTDESC_UNIXTIME, 0) : I18N (&app, IDS_FMTNAME_UNIXTIME, 0);

	else if (type == TypeMactime)
		result = is_desc ? I18N (&app, IDS_FMTDESC_MACTIME, 0) : I18N (&app, IDS_FMTNAME_MACTIME, 0);

	else if (type == TypeMicrosofttime)
		result = is_desc ? I18N (&app, IDS_FMTDESC_MICROSOFTTIME, 0) : I18N (&app, IDS_FMTNAME_MICROSOFTTIME, 0);

	else if (type == TypeFiletime)
		result = is_desc ? I18N (&app, IDS_FMTDESC_FILETIME, 0) : I18N (&app, IDS_FMTNAME_FILETIME, 0);

	return result;
}

void _app_print (HWND hwnd)
{
	SYSTEMTIME st = {0};
	SendDlgItemMessage (hwnd, IDC_INPUT, DTM_GETSYSTEMTIME, 0, (LPARAM)&st);

	const __time64_t ut = _r_unixtime_from_systemtime (&st);

	FILETIME ft = {0};
	SystemTimeToFileTime (&st, &ft);

	ULARGE_INTEGER ul = {0};
	ul.LowPart = ft.dwLowDateTime;
	ul.HighPart = ft.dwHighDateTime;

	for (UINT i = 0; i < TypeMax; i++)
		_r_listview_setitem (hwnd, IDC_LISTVIEW, i, 1, _app_timeconvert (ut, &st, &ul, (EnumDateType)i));
}

BOOL initializer_callback (HWND hwnd, DWORD msg, LPVOID, LPVOID)
{
	switch (msg)
	{
		case _RM_INITIALIZE:
		{
			// configure menu
			CheckMenuItem (GetMenu (hwnd), IDM_ALWAYSONTOP_CHK, MF_BYCOMMAND | (app.ConfigGet (L"AlwaysOnTop", false).AsBool () ? MF_CHECKED : MF_UNCHECKED));
			CheckMenuItem (GetMenu (hwnd), IDM_CHECKUPDATES_CHK, MF_BYCOMMAND | (app.ConfigGet (L"CheckUpdates", true).AsBool () ? MF_CHECKED : MF_UNCHECKED));

			// configure timezone
			{
				const HMENU submenu_timezone = GetSubMenu (GetSubMenu (GetMenu (hwnd), 1), TIMEZONE_MENU);

				// clear menu
				for (UINT i = 0;; i++)
				{
					if (!DeleteMenu (submenu_timezone, TIMEZONE_MENU + i, MF_BYCOMMAND))
					{
						DeleteMenu (submenu_timezone, 0, MF_BYPOSITION); // delete separator
						break;
					}
				}

				const LONG def_bias = _app_timegetdefaultbias ();
				const LONG current_bias = _app_timegetcurrentbias ();

				for (size_t i = 0; i < _countof (int_timezones); i++)
				{
					MENUITEMINFO mii = {0};

					WCHAR name[32] = {0};
					StringCchPrintf (name, _countof (name), L"GMT %s", _app_timezone2string (int_timezones[i], true, L"+00:00 (UTC)").GetString ());

					if (int_timezones[i] == def_bias)
						StringCchCat (name, _countof (name), L" (Def.)");

					mii.cbSize = sizeof (mii);
					mii.fMask = MIIM_ID | MIIM_STRING;
					mii.fType = MFT_STRING;
					mii.fState = MFS_DEFAULT;
					mii.dwTypeData = name;
					mii.wID = IDM_TIMEZONE + UINT (i);

					InsertMenuItem (submenu_timezone, IDM_TIMEZONE + UINT (i), FALSE, &mii);

					if (int_timezones[i] == current_bias)
						CheckMenuRadioItem (submenu_timezone, IDM_TIMEZONE, IDM_TIMEZONE + UINT (_countof (int_timezones)), IDM_TIMEZONE + UINT (i), MF_BYCOMMAND);
				}
			}

			break;
		}

		case _RM_LOCALIZE:
		{
			// configure menu
			const HMENU menu = GetMenu (hwnd);

			app.LocaleMenu (menu, I18N (&app, IDS_FILE, 0), 0, true, nullptr);
			app.LocaleMenu (menu, I18N (&app, IDS_EXIT, 0), IDM_EXIT, false, L"\tEsc");
			app.LocaleMenu (menu, I18N (&app, IDS_SETTINGS, 0), 1, true, nullptr);
			app.LocaleMenu (menu, I18N (&app, IDS_ALWAYSONTOP_CHK, 0), IDM_ALWAYSONTOP_CHK, false, nullptr);
			app.LocaleMenu (menu, I18N (&app, IDS_CHECKUPDATES_CHK, 0), IDM_CHECKUPDATES_CHK, false, nullptr);
			app.LocaleMenu (GetSubMenu (menu, 1), I18N (&app, IDS_TIMEZONE, 0), TIMEZONE_MENU, true, nullptr);
			app.LocaleMenu (GetSubMenu (menu, 1), I18N (&app, IDS_LANGUAGE, 0), LANG_MENU, true, L" (Language)");
			app.LocaleMenu (menu, I18N (&app, IDS_HELP, 0), 2, true, nullptr);
			app.LocaleMenu (menu, I18N (&app, IDS_WEBSITE, 0), IDM_WEBSITE, false, nullptr);
			app.LocaleMenu (menu, I18N (&app, IDS_CHECKUPDATES, 0), IDM_CHECKUPDATES, false, nullptr);
			app.LocaleMenu (menu, I18N (&app, IDS_ABOUT, 0), IDM_ABOUT, false, L"\tF1");

			app.LocaleEnum ((HWND)GetSubMenu (menu, 1), LANG_MENU, true, IDM_LANGUAGE); // enum localizations

			// configure listview
			for (UINT i = 0; i < TypeMax; i++)
				_r_listview_setitem (hwnd, IDC_LISTVIEW, i, 0, _app_timedescription ((EnumDateType)i, false));

			_r_wnd_addstyle (hwnd, IDC_CURRENT, app.IsClassicUI () ? WS_EX_STATICEDGE : 0, WS_EX_STATICEDGE, GWL_EXSTYLE);

			RedrawWindow (hwnd, nullptr, nullptr, RDW_ERASENOW | RDW_INVALIDATE);

			break;
		}
	}

	return FALSE;
}

INT_PTR CALLBACK DlgProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			// configure listview
			_r_listview_setstyle (hwnd, IDC_LISTVIEW, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP);

			_r_listview_addcolumn (hwnd, IDC_LISTVIEW, 1, nullptr, 39, LVCFMT_LEFT);
			_r_listview_addcolumn (hwnd, IDC_LISTVIEW, 2, nullptr, 61, LVCFMT_RIGHT);

			for (UINT i = 0; i < TypeMax; i++)
				_r_listview_additem (hwnd, IDC_LISTVIEW, i, 0, nullptr);

			// configure controls
			rstring dt_format;
			WCHAR buffer[MAX_PATH] = {0};

			GetLocaleInfo (LOCALE_SYSTEM_DEFAULT, LOCALE_SSHORTDATE, buffer, _countof (buffer));
			dt_format.Append (buffer);
			dt_format.Append (L", ");

			GetLocaleInfo (LOCALE_SYSTEM_DEFAULT, LOCALE_STIMEFORMAT, buffer, _countof (buffer));
			dt_format.Append (buffer);

			SendDlgItemMessage (hwnd, IDC_INPUT, DTM_SETFORMAT, 0, (LPARAM)dt_format.GetBuffer ());
			dt_format.Clear ();

			// print latest timestamp
			{
				SYSTEMTIME st = {0};
				__time64_t ut = app.ConfigGet (L"LatestTimestamp", 0).AsLonglong ();

				if (!ut)
					ut = _r_unixtime_now ();

				_r_unixtime_to_systemtime (ut, &st);

				SendDlgItemMessage (hwnd, IDC_INPUT, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
			}

			_r_ctrl_settip (hwnd, IDC_CURRENT, LPSTR_TEXTCALLBACK);

			_app_print (hwnd);

			break;
		}

		case WM_DESTROY:
		{
			SYSTEMTIME st = {0};
			SendDlgItemMessage (hwnd, IDC_INPUT, DTM_GETSYSTEMTIME, 0, (LPARAM)&st);

			app.ConfigSet (L"LatestTimestamp", _r_unixtime_from_systemtime (&st));

			PostQuitMessage (0);

			break;
		}

		case WM_QUERYENDSESSION:
		{
			SetWindowLongPtr (hwnd, DWLP_MSGRESULT, TRUE);
			return TRUE;
		}

		case WM_CONTEXTMENU:
		{
			if (GetDlgCtrlID ((HWND)wparam) == IDC_LISTVIEW)
			{
				const HMENU menu = LoadMenu (nullptr, MAKEINTRESOURCE (IDM_LISTVIEW));
				const HMENU submenu = GetSubMenu (menu, 0);

				// localize
				app.LocaleMenu (submenu, I18N (&app, IDS_COPY, 0), IDM_COPY, false, L"\tCtrl+C");

				if (!SendDlgItemMessage (hwnd, IDC_LISTVIEW, LVM_GETSELECTEDCOUNT, 0, 0))
					EnableMenuItem (submenu, IDM_COPY, MF_BYCOMMAND | MF_DISABLED);

				POINT pt = {0};
				GetCursorPos (&pt);

				TrackPopupMenuEx (submenu, TPM_RIGHTBUTTON | TPM_LEFTBUTTON, pt.x, pt.y, hwnd, nullptr);

				DestroyMenu (menu);
			}

			break;
		}

		case WM_NOTIFY:
		{
			switch (LPNMHDR (lparam)->code)
			{
				case DTN_USERSTRING:
				{
					LPNMDATETIMESTRING lpds = (LPNMDATETIMESTRING)lparam;

					const rstring datetime = lpds->pszUserString;

					if (datetime.IsNumeric ())
						_r_unixtime_to_systemtime (datetime.AsLonglong (), &lpds->st);

					break;
				}

				case DTN_DATETIMECHANGE:
				case DTN_CLOSEUP:
				{
					_app_print (hwnd);
					break;
				}

				case LVN_GETINFOTIP:
				{
					LPNMLVGETINFOTIP lpnmlv = (LPNMLVGETINFOTIP)lparam;

					StringCchCopy (lpnmlv->pszText, lpnmlv->cchTextMax, _app_timedescription ((EnumDateType)lpnmlv->iItem, true));

					break;
				}

				case TTN_GETDISPINFO:
				{
					LPNMTTDISPINFO lpnmdi = (LPNMTTDISPINFO)lparam;

					if ((lpnmdi->uFlags & TTF_IDISHWND) != 0)
					{
						WCHAR buffer[1024] = {0};
						const UINT ctrl_id = GetDlgCtrlID ((HWND)lpnmdi->hdr.idFrom);

						if (ctrl_id == IDC_CURRENT)
							StringCchCopy (buffer, _countof (buffer), I18N (&app, IDS_CURRENT, 0));

						if (buffer[0])
							lpnmdi->lpszText = buffer;
					}

					break;
				}
			}

			break;
		}

		case WM_COMMAND:
		{
			if (HIWORD (wparam) == 0 && LOWORD (wparam) >= IDM_LANGUAGE && LOWORD (wparam) <= IDM_LANGUAGE + app.LocaleGetCount ())
			{
				app.LocaleApplyFromMenu (GetSubMenu (GetSubMenu (GetMenu (hwnd), 1), LANG_MENU), LOWORD (wparam), IDM_LANGUAGE);

				return FALSE;
			}
			else if ((LOWORD (wparam) >= IDM_TIMEZONE && LOWORD (wparam) <= IDM_TIMEZONE + _countof (int_timezones)))
			{
				const UINT idx = LOWORD (wparam) - IDM_TIMEZONE;

				app.ConfigSet (L"TimezoneBias", (LONGLONG)int_timezones[idx]);

				const HMENU submenu_timezone = GetSubMenu (GetSubMenu (GetMenu (hwnd), 1), TIMEZONE_MENU);
				CheckMenuRadioItem (submenu_timezone, IDM_TIMEZONE, IDM_TIMEZONE + UINT (_countof (int_timezones)), LOWORD (wparam), MF_BYCOMMAND);

				_app_print (hwnd);

				return FALSE;
			}

			switch (LOWORD (wparam))
			{
				case IDCANCEL: // process Esc key
				case IDM_EXIT:
				{
					DestroyWindow (hwnd);
					break;
				}

				case IDM_ALWAYSONTOP_CHK:
				{
					const bool new_val = !app.ConfigGet (L"AlwaysOnTop", false).AsBool ();

					CheckMenuItem (GetMenu (hwnd), IDM_ALWAYSONTOP_CHK, MF_BYCOMMAND | (new_val ? MF_CHECKED : MF_UNCHECKED));
					app.ConfigSet (L"AlwaysOnTop", new_val);

					_r_wnd_top (hwnd, new_val);

					break;
				}

				case IDM_CHECKUPDATES_CHK:
				{
					const bool new_val = !app.ConfigGet (L"CheckUpdates", true).AsBool ();

					CheckMenuItem (GetMenu (hwnd), IDM_ALWAYSONTOP_CHK, MF_BYCOMMAND | (new_val ? MF_CHECKED : MF_UNCHECKED));
					app.ConfigSet (L"CheckUpdates", new_val);

					break;
				}

				case IDM_WEBSITE:
				{
					ShellExecute (hwnd, nullptr, _APP_WEBSITE_URL, nullptr, nullptr, SW_SHOWDEFAULT);
					break;
				}

				case IDM_CHECKUPDATES:
				{
					app.CheckForUpdates (false);
					break;
				}

				case IDM_ABOUT:
				{
					app.CreateAboutWindow (hwnd, I18N (&app, IDS_DONATE, 0));
					break;
				}

				case IDM_COPY:
				{
					rstring buffer;

					size_t item = LAST_VALUE;

					while ((item = (size_t)SendDlgItemMessage (hwnd, IDC_LISTVIEW, LVM_GETNEXTITEM, item, LVNI_SELECTED)) != -1)
					{
						buffer.Append (_r_listview_getitemtext (hwnd, IDC_LISTVIEW, item, 1));
						buffer.Append (L"\r\n");
					}

					if (!buffer.IsEmpty ())
					{
						buffer.Trim (L"\r\n");

						_r_clipboard_set (hwnd, buffer, buffer.GetLength ());
					}

					break;
				}

				case IDC_CURRENT:
				{
					SYSTEMTIME st = {0};
					_app_timegetcurrent (&st);

					SendDlgItemMessage (hwnd, IDC_INPUT, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);

					_app_print (hwnd);

					break;
				}

				case IDM_SELECT_ALL:
				{
					ListView_SetItemState (GetDlgItem (hwnd, IDC_LISTVIEW), -1, LVIS_SELECTED, LVIS_SELECTED);
					break;
				}
			}
		}
	}

	return FALSE;
}

INT APIENTRY wWinMain (HINSTANCE, HINSTANCE, LPWSTR, INT)
{
	MSG msg = {0};

	if (app.CreateMainWindow (IDD_MAIN, IDI_MAIN, &DlgProc, &initializer_callback))
	{
		const HACCEL haccel = LoadAccelerators (app.GetHINSTANCE (), MAKEINTRESOURCE (IDA_MAIN));

		while (GetMessage (&msg, nullptr, 0, 0) > 0)
		{
			if (haccel)
				TranslateAccelerator (app.GetHWND (), haccel, &msg);

			if (!IsDialogMessage (app.GetHWND (), &msg))
			{
				TranslateMessage (&msg);
				DispatchMessage (&msg);
			}
		}

		if (haccel)
			DestroyAcceleratorTable (haccel);
	}

	return (INT)msg.wParam;
}
