// TimeVertor
// Copyright (c) 2012-2021 Henry++

#include "routine.h"

#include <windows.h>

#include "app.h"
#include "rapp.h"
#include "main.h"

#include "resource.h"

VOID _app_timezone2string (LPWSTR buffer, SIZE_T length, LONG bias, BOOLEAN divide, LPCWSTR utcname)
{
	if (bias == 0)
	{
		_r_str_copy (buffer, length, utcname);
	}
	else
	{
		_r_str_printf (buffer, length, L"%c%02u%s%02u", ((bias > 0) ? L'-' : L'+'), (UINT)floor ((long double)abs (bias) / 60.0), divide ? L":" : L"", (LONG)abs (bias) % 60);
	}
}

FORCEINLINE LONG _app_getdefaultbias ()
{
	TIME_ZONE_INFORMATION tzi = {0};

	GetTimeZoneInformation (&tzi);

	return tzi.Bias;
}

FORCEINLINE LONG _app_getcurrentbias ()
{
	return _r_config_getlong (L"TimezoneBias", _app_getdefaultbias ());
}

FORCEINLINE LONG64 _app_getlastesttimestamp ()
{
	return _r_config_getlong64 (L"LatestTimestamp", _r_unixtime_now ());
}

FORCEINLINE VOID _app_converttime (LPSYSTEMTIME lpcurrent_time, LONG bias, LPSYSTEMTIME lpsystem_time)
{
	TIME_ZONE_INFORMATION tzi = {0};

	tzi.Bias = bias; // set timezone shift

	SystemTimeToTzSpecificLocalTime (&tzi, lpcurrent_time, lpsystem_time);
}

VOID _app_timeconvert (LPWSTR buffer, SIZE_T length, LONG64 unixtime, LONG bias, LPSYSTEMTIME lpst, PULARGE_INTEGER pul, ENUM_DATE_TYPE type)
{
	WCHAR timezone[32] = {0};

	if (type == TypeRfc2822)
	{
		_app_timezone2string (timezone, RTL_NUMBER_OF (timezone), bias, FALSE, L"GMT");
		_r_str_printf (buffer, length, L"%s, %02d %s %04d %02d:%02d:%02d %s", str_dayofweek[lpst->wDayOfWeek], lpst->wDay, str_month[lpst->wMonth - 1], lpst->wYear, lpst->wHour, lpst->wMinute, lpst->wSecond, timezone);
	}
	else if (type == TypeIso8601)
	{
		_app_timezone2string (timezone, RTL_NUMBER_OF (timezone), bias, TRUE, L"Z");
		_r_str_printf (buffer, length, L"%04d-%02d-%02dT%02d:%02d:%02d%s", lpst->wYear, lpst->wMonth, lpst->wDay, lpst->wHour, lpst->wMinute, lpst->wSecond, timezone);
	}
	else if (type == TypeUnixtime)
	{
		_r_str_printf (buffer, length, L"%" TEXT (PR_LONG64), unixtime);
	}
	else if (type == TypeMactime)
	{
		_r_str_printf (buffer, length, L"%" TEXT (PR_LONG64), unixtime + MAC_TIMESTAMP);
	}
	else if (type == TypeMicrosofttime)
	{
		_r_str_printf (buffer, length, L"%.09f", ((DOUBLE)pul->QuadPart / (24.0 * (60.0 * (60.0 * 10000000.0)))) - MICROSOFT_TIMESTAMP);
	}
	else if (type == TypeFiletime)
	{
		_r_str_printf (buffer, length, L"%" TEXT (PR_ULONG64), pul->QuadPart);
	}
}

LPCWSTR _app_gettimedescription (ENUM_DATE_TYPE type, BOOLEAN is_desc)
{
	if (type == TypeRfc2822)
	{
		return _r_locale_getstring (is_desc ? IDS_FMTDESC_RFC2822 : IDS_FMTNAME_RFC2822);
	}
	else if (type == TypeIso8601)
	{
		return _r_locale_getstring (is_desc ? IDS_FMTDESC_ISO8601 : IDS_FMTNAME_ISO8601);
	}
	else if (type == TypeUnixtime)
	{
		return _r_locale_getstring (is_desc ? IDS_FMTDESC_UNIXTIME : IDS_FMTNAME_UNIXTIME);
	}
	else if (type == TypeMactime)
	{
		return _r_locale_getstring (is_desc ? IDS_FMTDESC_MACTIME : IDS_FMTNAME_MACTIME);
	}
	else if (type == TypeMicrosofttime)
	{
		return _r_locale_getstring (is_desc ? IDS_FMTDESC_MICROSOFTTIME : IDS_FMTNAME_MICROSOFTTIME);
	}
	else if (type == TypeFiletime)
	{
		return _r_locale_getstring (is_desc ? IDS_FMTDESC_FILETIME : IDS_FMTNAME_FILETIME);
	}

	return NULL;
}

VOID _app_printdate (HWND hwnd, LPSYSTEMTIME lpst)
{
	ULARGE_INTEGER ul;
	FILETIME filetime;
	WCHAR time_string[128];
	LONG64 unixtime;
	LONG bias;

	unixtime = _r_unixtime_from_systemtime (lpst);
	bias = _app_getcurrentbias ();

	SystemTimeToFileTime (lpst, &filetime);

	ul.LowPart = filetime.dwLowDateTime;
	ul.HighPart = filetime.dwHighDateTime;

	SendDlgItemMessage (hwnd, IDC_INPUT, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)lpst);

	for (ENUM_DATE_TYPE i = 0; i < TypeMax; i++)
	{
		_app_timeconvert (time_string, RTL_NUMBER_OF (time_string), unixtime, bias, lpst, &ul, i);

		_r_listview_setitem (hwnd, IDC_LISTVIEW, i, 1, time_string);
	}
}

INT_PTR CALLBACK DlgProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			// configure timezone
			HMENU hmenu = GetMenu (hwnd);

			if (hmenu)
			{
				HMENU submenu_timezone = GetSubMenu (GetSubMenu (hmenu, 1), TIMEZONE_MENU);

				if (submenu_timezone)
				{
					// clear menu
					_r_menu_clearitems (submenu_timezone);

					MENUITEMINFO mii;
					WCHAR menu_title[32];
					WCHAR timezone[32];
					LONG bias;

					LONG current_bias = _app_getcurrentbias ();
					LONG default_bias = _app_getdefaultbias ();

					for (SIZE_T i = 0; i < RTL_NUMBER_OF (int_timezones); i++)
					{
						bias = int_timezones[i];

						_app_timezone2string (timezone, RTL_NUMBER_OF (timezone), bias, TRUE, L"+00:00 (UTC)");
						_r_str_printf (menu_title, RTL_NUMBER_OF (menu_title), L"GMT %s", timezone);

						if (bias == default_bias)
							_r_str_append (menu_title, RTL_NUMBER_OF (menu_title), SYSTEM_BIAS);

						memset (&mii, 0, sizeof (mii));

						mii.cbSize = sizeof (mii);
						mii.fMask = MIIM_ID | MIIM_STRING;
						mii.fType = MFT_STRING;
						mii.fState = MFS_DEFAULT;
						mii.dwTypeData = menu_title;
						mii.wID = IDX_TIMEZONE + (UINT)i;

						InsertMenuItem (submenu_timezone, mii.wID, FALSE, &mii);

						if (bias == current_bias)
						{
							_r_menu_checkitem (submenu_timezone, IDX_TIMEZONE, IDX_TIMEZONE + (RTL_NUMBER_OF (int_timezones) - 1), MF_BYCOMMAND, mii.wID);
						}
					}
				}
			}

			// configure listview
			_r_listview_setstyle (hwnd, IDC_LISTVIEW, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP, FALSE);

			_r_listview_addcolumn (hwnd, IDC_LISTVIEW, 1, NULL, -39, LVCFMT_LEFT);
			_r_listview_addcolumn (hwnd, IDC_LISTVIEW, 2, NULL, -61, LVCFMT_RIGHT);

			for (INT i = 0; i < TypeMax; i++)
				_r_listview_additem (hwnd, IDC_LISTVIEW, i, 0, L"");

			// configure datetime format
			WCHAR date_format[256] = {0};
			WCHAR time_format[128] = {0};

			if (
				GetLocaleInfo (LOCALE_USER_DEFAULT, LOCALE_SLONGDATE, date_format, RTL_NUMBER_OF (date_format)) &&
				GetLocaleInfo (LOCALE_USER_DEFAULT, LOCALE_STIMEFORMAT, time_format, RTL_NUMBER_OF (time_format))
				)
			{
				_r_str_appendformat (date_format, RTL_NUMBER_OF (date_format), L" %s", time_format);

				SendDlgItemMessage (hwnd, IDC_INPUT, DTM_SETFORMAT, 0, (LPARAM)date_format);
			}

			// print latest timestamp
			SYSTEMTIME current_time = {0};
			SYSTEMTIME system_time = {0};

			_r_unixtime_to_systemtime (_app_getlastesttimestamp (), &current_time);

			_app_converttime (&current_time, 0, &system_time);
			_app_printdate (hwnd, &system_time);

			// set control tip
			HWND htip = _r_ctrl_createtip (hwnd);

			if (htip)
				_r_ctrl_settiptext (htip, hwnd, IDC_CURRENT, LPSTR_TEXTCALLBACK);

			break;
		}

		case WM_DESTROY:
		{
			SYSTEMTIME system_time = {0};
			SYSTEMTIME current_time = {0};

			SendDlgItemMessage (hwnd, IDC_INPUT, DTM_GETSYSTEMTIME, GDT_VALID, (LPARAM)&system_time);
			_app_converttime (&system_time, 0, &current_time);

			_r_config_setlong64 (L"LatestTimestamp", _r_unixtime_from_systemtime (&current_time));

			PostQuitMessage (0);

			break;
		}

		case RM_INITIALIZE:
		{
			// configure menu
			HMENU hmenu = GetMenu (hwnd);

			if (hmenu)
			{
				CheckMenuItem (hmenu, IDM_ALWAYSONTOP_CHK, MF_BYCOMMAND | (_r_config_getboolean (L"AlwaysOnTop", APP_ALWAYSONTOP) ? MF_CHECKED : MF_UNCHECKED));
				CheckMenuItem (hmenu, IDM_CLASSICUI_CHK, MF_BYCOMMAND | (_r_config_getboolean (L"ClassicUI", APP_CLASSICUI) ? MF_CHECKED : MF_UNCHECKED));
				CheckMenuItem (hmenu, IDM_CHECKUPDATES_CHK, MF_BYCOMMAND | (_r_config_getboolean (L"CheckUpdates", TRUE) ? MF_CHECKED : MF_UNCHECKED));
			}

			break;
		}

		case RM_LOCALIZE:
		{
			// configure menu
			HMENU hmenu = GetMenu (hwnd);

			if (hmenu)
			{
				_r_menu_setitemtext (hmenu, 0, TRUE, _r_locale_getstring (IDS_FILE));
				_r_menu_setitemtext (hmenu, 1, TRUE, _r_locale_getstring (IDS_SETTINGS));
				_r_menu_setitemtext (hmenu, 2, TRUE, _r_locale_getstring (IDS_HELP));

				_r_menu_setitemtext (GetSubMenu (hmenu, 1), TIMEZONE_MENU, TRUE, _r_locale_getstring (IDS_TIMEZONE));
				_r_menu_setitemtextformat (GetSubMenu (hmenu, 1), LANG_MENU, TRUE, L"%s (Language)", _r_locale_getstring (IDS_LANGUAGE));

				_r_menu_setitemtextformat (hmenu, IDM_EXIT, FALSE, L"%s\tEsc", _r_locale_getstring (IDS_EXIT));
				_r_menu_setitemtext (hmenu, IDM_ALWAYSONTOP_CHK, FALSE, _r_locale_getstring (IDS_ALWAYSONTOP_CHK));
				_r_menu_setitemtextformat (hmenu, IDM_CLASSICUI_CHK, FALSE, L"%s*", _r_locale_getstring (IDS_CLASSICUI_CHK));
				_r_menu_setitemtext (hmenu, IDM_CHECKUPDATES_CHK, FALSE, _r_locale_getstring (IDS_CHECKUPDATES_CHK));

				_r_menu_setitemtext (hmenu, IDM_WEBSITE, FALSE, _r_locale_getstring (IDS_WEBSITE));
				_r_menu_setitemtext (hmenu, IDM_CHECKUPDATES, FALSE, _r_locale_getstring (IDS_CHECKUPDATES));
				_r_menu_setitemtextformat (hmenu, IDM_ABOUT, FALSE, L"%s\tF1", _r_locale_getstring (IDS_ABOUT));

				_r_locale_enum ((HWND)GetSubMenu (hmenu, 1), LANG_MENU, IDX_LANGUAGE); // enum localizations
			}

			// configure listview
			for (ENUM_DATE_TYPE i = 0; i < TypeMax; i++)
				_r_listview_setitem (hwnd, IDC_LISTVIEW, i, 0, _app_gettimedescription (i, FALSE));

			_r_wnd_addstyle (hwnd, IDC_CURRENT, _r_app_isclassicui () ? WS_EX_STATICEDGE : 0, WS_EX_STATICEDGE, GWL_EXSTYLE);

			break;
		}

		case WM_DPICHANGED:
		{
			_r_listview_setcolumn (hwnd, IDC_LISTVIEW, 0, NULL, -39);
			_r_listview_setcolumn (hwnd, IDC_LISTVIEW, 1, NULL, -61);

			break;
		}

		case WM_CONTEXTMENU:
		{
			if (GetDlgCtrlID ((HWND)wparam) == IDC_LISTVIEW)
			{
				HMENU hmenu = LoadMenu (NULL, MAKEINTRESOURCE (IDM_LISTVIEW));
				HMENU hsubmenu = GetSubMenu (hmenu, 0);

				// localize
				_r_menu_setitemtextformat (hsubmenu, IDM_COPY, FALSE, L"%s\tCtrl+C", _r_locale_getstring (IDS_COPY));

				if (!_r_listview_getselectedcount (hwnd, IDC_LISTVIEW))
					_r_menu_enableitem (hsubmenu, IDM_COPY, MF_BYCOMMAND, FALSE);

				_r_menu_popup (hsubmenu, hwnd, NULL, TRUE);

				DestroyMenu (hmenu);
			}

			break;
		}

		case WM_NOTIFY:
		{
			LPNMHDR nmlp = (LPNMHDR)lparam;

			switch (nmlp->code)
			{
				case DTN_USERSTRING:
				{
					LPNMDATETIMESTRING lpds = (LPNMDATETIMESTRING)lparam;

					if (_r_str_isnumeric (lpds->pszUserString))
						_r_unixtime_to_systemtime (_r_str_tolong64 (lpds->pszUserString), &lpds->st);

					break;
				}

				case DTN_DATETIMECHANGE:
				{
					LPNMDATETIMECHANGE lpnmdtc = (LPNMDATETIMECHANGE)lparam;
					SYSTEMTIME system_time = {0};

					_app_converttime (&lpnmdtc->st, 0, &system_time);
					_app_printdate (hwnd, &system_time);

					break;
				}

				case LVN_GETINFOTIP:
				{
					LPNMLVGETINFOTIP lpnmlv = (LPNMLVGETINFOTIP)lparam;

					_r_str_copy (lpnmlv->pszText, lpnmlv->cchTextMax, _app_gettimedescription (lpnmlv->iItem, TRUE));

					break;
				}

				case TTN_GETDISPINFO:
				{
					LPNMTTDISPINFO lpnmdi = (LPNMTTDISPINFO)lparam;

					if ((lpnmdi->uFlags & TTF_IDISHWND) != 0)
					{
						WCHAR buffer[256] = {0};
						INT ctrl_id = GetDlgCtrlID ((HWND)lpnmdi->hdr.idFrom);

						if (ctrl_id == IDC_CURRENT)
							_r_str_copy (buffer, RTL_NUMBER_OF (buffer), _r_locale_getstring (IDS_CURRENT));

						if (!_r_str_isempty (buffer))
							lpnmdi->lpszText = buffer;
					}

					break;
				}
			}

			break;
		}

		case WM_COMMAND:
		{
			INT ctrl_id = LOWORD (wparam);

			if (HIWORD (wparam) == 0 && ctrl_id >= IDX_LANGUAGE && ctrl_id <= IDX_LANGUAGE + (INT)_r_locale_getcount ())
			{
				_r_locale_applyfrommenu (GetSubMenu (GetSubMenu (GetMenu (hwnd), 1), LANG_MENU), ctrl_id);

				return FALSE;
			}
			else if ((ctrl_id >= IDX_TIMEZONE && ctrl_id <= IDX_TIMEZONE + (RTL_NUMBER_OF (int_timezones) - 1)))
			{
				UINT idx = ctrl_id - IDX_TIMEZONE;
				LONG bias = int_timezones[idx];

				_r_config_setlong (L"TimezoneBias", bias);

				HMENU submenu_timezone = GetSubMenu (GetSubMenu (GetMenu (hwnd), 1), TIMEZONE_MENU);

				if (submenu_timezone)
					_r_menu_checkitem (submenu_timezone, IDX_TIMEZONE, IDX_TIMEZONE + (RTL_NUMBER_OF (int_timezones) - 1), MF_BYCOMMAND, ctrl_id);

				SYSTEMTIME current_time = {0};
				SYSTEMTIME system_time = {0};

				SendDlgItemMessage (hwnd, IDC_INPUT, DTM_GETSYSTEMTIME, GDT_VALID, (LPARAM)&current_time);

				_app_converttime (&current_time, bias, &system_time);
				_app_printdate (hwnd, &system_time);

				return FALSE;
			}

			switch (ctrl_id)
			{
				case IDCANCEL: // process Esc key
				case IDM_EXIT:
				{
					DestroyWindow (hwnd);
					break;
				}

				case IDM_ALWAYSONTOP_CHK:
				{
					BOOLEAN new_val = !_r_config_getboolean (L"AlwaysOnTop", APP_ALWAYSONTOP);

					CheckMenuItem (GetMenu (hwnd), ctrl_id, MF_BYCOMMAND | (new_val ? MF_CHECKED : MF_UNCHECKED));
					_r_config_setboolean (L"AlwaysOnTop", new_val);

					_r_wnd_top (hwnd, new_val);

					break;
				}

				case IDM_CLASSICUI_CHK:
				{
					BOOLEAN new_val = !_r_config_getboolean (L"ClassicUI", APP_CLASSICUI);

					CheckMenuItem (GetMenu (hwnd), ctrl_id, MF_BYCOMMAND | (new_val ? MF_CHECKED : MF_UNCHECKED));
					_r_config_setboolean (L"ClassicUI", new_val);

					_r_app_restart (hwnd);

					break;
				}

				case IDM_CHECKUPDATES_CHK:
				{
					BOOLEAN new_val = !_r_config_getboolean (L"CheckUpdates", TRUE);

					CheckMenuItem (GetMenu (hwnd), ctrl_id, MF_BYCOMMAND | (new_val ? MF_CHECKED : MF_UNCHECKED));
					_r_config_setboolean (L"CheckUpdates", new_val);

					break;
				}

				case IDM_WEBSITE:
				{
					ShellExecute (hwnd, NULL, _r_app_getwebsite_url (), NULL, NULL, SW_SHOWDEFAULT);
					break;
				}

				case IDM_CHECKUPDATES:
				{
					_r_update_check (hwnd);
					break;
				}

				case IDM_ABOUT:
				{
					_r_show_aboutmessage (hwnd);
					break;
				}

				case IDM_COPY:
				{
					R_STRINGBUILDER buffer;
					PR_STRING string;

					_r_obj_initializestringbuilder (&buffer);

					INT item = -1;

					while ((item = (INT)SendDlgItemMessage (hwnd, IDC_LISTVIEW, LVM_GETNEXTITEM, (WPARAM)item, LVNI_SELECTED)) != -1)
					{
						string = _r_listview_getitemtext (hwnd, IDC_LISTVIEW, item, 1);

						if (string)
						{
							_r_obj_appendstringbuilderformat (&buffer, L"%s\r\n", string->buffer);

							_r_obj_dereference (string);
						}
					}

					string = _r_obj_finalstringbuilder (&buffer);

					if (!_r_obj_isstringempty (string))
					{
						_r_obj_trimstring (string, L"\r\n");

						_r_clipboard_set (hwnd, string->buffer, _r_obj_getstringlength (string));
					}

					_r_obj_deletestringbuilder (&buffer);

					break;
				}

				case IDC_CURRENT:
				{
					SYSTEMTIME current_time = {0};
					SYSTEMTIME system_time = {0};

					GetSystemTime (&current_time);

					_app_converttime (&current_time, _app_getcurrentbias (), &system_time);
					_app_printdate (hwnd, &system_time);

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

INT APIENTRY wWinMain (_In_ HINSTANCE hinst, _In_opt_ HINSTANCE prev_hinst, _In_ LPWSTR cmdline, _In_ INT show_cmd)
{
	MSG msg;

	if (_r_app_initialize ())
	{
		if (_r_app_createwindow (IDD_MAIN, IDI_MAIN, &DlgProc))
		{
			HACCEL haccel = LoadAccelerators (hinst, MAKEINTRESOURCE (IDA_MAIN));

			if (haccel)
			{
				while (GetMessage (&msg, NULL, 0, 0) > 0)
				{
					HWND hwnd = GetActiveWindow ();

					if (!TranslateAccelerator (hwnd, haccel, &msg) && !IsDialogMessage (hwnd, &msg))
					{
						TranslateMessage (&msg);
						DispatchMessage (&msg);
					}
				}

				DestroyAcceleratorTable (haccel);
			}
		}
	}

	return ERROR_SUCCESS;
}
