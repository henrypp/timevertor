// TimeVertor
// Copyright (c) 2012-2023 Henry++

#include "routine.h"

#include <windows.h>

#include "app.h"
#include "rapp.h"
#include "main.h"

#include "resource.h"

VOID _app_timezone2string (
	_Out_writes_ (length) LPWSTR buffer,
	_In_ SIZE_T length,
	_In_ LONG bias,
	_In_ BOOLEAN divide,
	_In_ LPCWSTR utcname
)
{
	if (bias == 0)
	{
		_r_str_copy (buffer, length, utcname);
	}
	else
	{
		_r_str_printf (
			buffer,
			length,
			L"%c%02u%s%02u",
			((bias > 0) ? L'-' : L'+'),
			(UINT)floor ((long double)abs (bias) / 60.0),
			divide ? L":" : L"",
			(LONG)abs (bias) % 60
		);
	}
}

LONG _app_getdefaultbias ()
{
	RTL_TIME_ZONE_INFORMATION tzi = {0};
	NTSTATUS status;

	status = NtQuerySystemInformation (SystemCurrentTimeZoneInformation, &tzi, sizeof (RTL_TIME_ZONE_INFORMATION), NULL);

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

VOID _app_converttime (
	_In_ LPSYSTEMTIME current_time,
	_In_ LONG bias,
	_Out_ LPSYSTEMTIME system_time
)
{
	TIME_ZONE_INFORMATION tzi = {0};

	tzi.Bias = bias; // set timezone shift

	if (!SystemTimeToTzSpecificLocalTime (&tzi, current_time, system_time))
		RtlCopyMemory (system_time, current_time, sizeof (SYSTEMTIME));
}

VOID _app_timeconvert (
	_Out_writes_z_ (buffer_size) LPWSTR buffer,
	_In_ SIZE_T buffer_size,
	_In_ LONG64 unixtime,
	_In_ LONG bias,
	_In_ LPSYSTEMTIME lpst,
	_In_ PULARGE_INTEGER pul,
	_In_ ENUM_DATE_TYPE type
)
{
	WCHAR timezone[32] = {0};

	switch (type)
	{
		case TypeRfc2822:
		{
			_app_timezone2string (timezone, RTL_NUMBER_OF (timezone), bias, FALSE, L"GMT");

			_r_str_printf (
				buffer,
				buffer_size,
				L"%s, %02d %s %04d %02d:%02d:%02d %s",
				str_dayofweek[lpst->wDayOfWeek],
				lpst->wDay, str_month[lpst->wMonth - 1],
				lpst->wYear, lpst->wHour,
				lpst->wMinute,
				lpst->wSecond,
				timezone
			);

			break;
		}

		case TypeIso8601:
		{
			_app_timezone2string (timezone, RTL_NUMBER_OF (timezone), bias, TRUE, L"Z");

			_r_str_printf (
				buffer,
				buffer_size,
				L"%04d-%02d-%02dT%02d:%02d:%02d%s",
				lpst->wYear,
				lpst->wMonth,
				lpst->wDay,
				lpst->wHour,
				lpst->wMinute,
				lpst->wSecond,
				timezone
			);

			break;
		}

		case TypeUnixtime:
		{
			_r_str_printf (buffer, buffer_size, L"%" TEXT (PR_LONG64), unixtime);
			break;
		}

		case TypeMactime:
		{
			_r_str_printf (buffer, buffer_size, L"%" TEXT (PR_LONG64), unixtime + MAC_TIMESTAMP);
			break;
		}

		case TypeMicrosofttime:
		{
			_r_str_printf (
				buffer,
				buffer_size,
				L"%.09f",
				((DOUBLE)pul->QuadPart / (24.0 * (60.0 * (60.0 * 10000000.0)))) - MICROSOFT_TIMESTAMP
			);

			break;
		}

		case TypeFiletime:
		{
			_r_str_printf (buffer, buffer_size, L"%" TEXT (PR_ULONG64), pul->QuadPart);
			break;
		}

		default:
		{
			*buffer = UNICODE_NULL;
		}
	}
}

LPWSTR _app_gettimedescription (
	_In_ ENUM_DATE_TYPE type,
	_In_ BOOLEAN is_desc
)
{
	switch (type)
	{
		case TypeRfc2822:
		{
			return _r_locale_getstring (is_desc ? IDS_FMTDESC_RFC2822 : IDS_FMTNAME_RFC2822);
		}

		case TypeIso8601:
		{
			return _r_locale_getstring (is_desc ? IDS_FMTDESC_ISO8601 : IDS_FMTNAME_ISO8601);
		}

		case TypeUnixtime:
		{
			return _r_locale_getstring (is_desc ? IDS_FMTDESC_UNIXTIME : IDS_FMTNAME_UNIXTIME);
		}

		case TypeMactime:
		{
			return _r_locale_getstring (is_desc ? IDS_FMTDESC_MACTIME : IDS_FMTNAME_MACTIME);
		}

		case TypeMicrosofttime:
		{
			return _r_locale_getstring (is_desc ? IDS_FMTDESC_MICROSOFTTIME : IDS_FMTNAME_MICROSOFTTIME);
		}

		case TypeFiletime:
		{
			return _r_locale_getstring (is_desc ? IDS_FMTDESC_FILETIME : IDS_FMTNAME_FILETIME);
		}
	}

	return NULL;
}

BOOLEAN _app_getdate (
	_In_ HWND hwnd,
	_In_ INT ctrl_id,
	_Out_ LPSYSTEMTIME current_time
)
{
	LRESULT status;

	RtlZeroMemory (current_time, sizeof (SYSTEMTIME));

	status = SendDlgItemMessage (hwnd, IDC_INPUT, DTM_GETSYSTEMTIME, GDT_VALID, (LPARAM)current_time);

	return (status == GDT_VALID);
}

VOID _app_printdate (
	_In_ HWND hwnd,
	_In_ LPSYSTEMTIME system_time
)
{
	ULARGE_INTEGER ul = {0};
	FILETIME filetime;
	WCHAR time_string[128];
	LONG64 unixtime;
	LONG bias;

	unixtime = _r_unixtime_from_systemtime (system_time);
	bias = _app_getcurrentbias ();

	SystemTimeToFileTime (system_time, &filetime);

	ul.LowPart = filetime.dwLowDateTime;
	ul.HighPart = filetime.dwHighDateTime;

	SendDlgItemMessage (hwnd, IDC_INPUT, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)system_time);

	for (ENUM_DATE_TYPE i = 0; i < TypeMax; i++)
	{
		_app_timeconvert (time_string, RTL_NUMBER_OF (time_string), unixtime, bias, system_time, &ul, i);

		_r_listview_setitem (hwnd, IDC_LISTVIEW, i, 1, time_string);
	}
}

INT_PTR CALLBACK DlgProc (
	_In_ HWND hwnd,
	_In_ UINT msg,
	_In_ WPARAM wparam,
	_In_ LPARAM lparam
)
{
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			SYSTEMTIME current_time;
			SYSTEMTIME system_time;
			WCHAR date_format[128];
			WCHAR time_format[128];
			WCHAR menu_title[32];
			WCHAR timezone[32];
			HMENU hmenu;
			HMENU hsubmenu;
			HWND htip;
			LONG current_bias;
			LONG default_bias;
			LONG bias;

			// configure timezone
			hmenu = GetMenu (hwnd);

			if (hmenu)
			{
				hsubmenu = GetSubMenu (GetSubMenu (hmenu, 1), TIMEZONE_MENU);

				if (hsubmenu)
				{
					// clear menu
					_r_menu_clearitems (hsubmenu);

					current_bias = _app_getcurrentbias ();
					default_bias = _app_getdefaultbias ();

					for (SIZE_T i = 0; i < RTL_NUMBER_OF (int_timezones); i++)
					{
						bias = int_timezones[i];

						_app_timezone2string (timezone, RTL_NUMBER_OF (timezone), bias, TRUE, L"+00:00 (UTC)");

						_r_str_printf (menu_title, RTL_NUMBER_OF (menu_title), L"GMT %s", timezone);

						if (bias == default_bias)
							_r_str_append (menu_title, RTL_NUMBER_OF (menu_title), SYSTEM_BIAS);

						_r_menu_additem_ex (hsubmenu, IDX_TIMEZONE + (UINT)i, menu_title, (bias == current_bias) ? MF_CHECKED : MF_UNCHECKED);
					}
				}
			}

			// configure listview
			_r_listview_setstyle (
				hwnd,
				IDC_LISTVIEW,
				LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP,
				FALSE
			);

			_r_listview_addcolumn (hwnd, IDC_LISTVIEW, 1, NULL, -39, LVCFMT_LEFT);
			_r_listview_addcolumn (hwnd, IDC_LISTVIEW, 2, NULL, -61, LVCFMT_RIGHT);

			for (INT i = 0; i < TypeMax; i++)
				_r_listview_additem (hwnd, IDC_LISTVIEW, i, L"");

			// configure datetime format
			if (GetLocaleInfo (LOCALE_USER_DEFAULT, LOCALE_SLONGDATE, date_format, RTL_NUMBER_OF (date_format)) &&
				GetLocaleInfo (LOCALE_USER_DEFAULT, LOCALE_STIMEFORMAT, time_format, RTL_NUMBER_OF (time_format)))
			{
				_r_str_appendformat (date_format, RTL_NUMBER_OF (date_format), L" %s", time_format);

				SendDlgItemMessage (hwnd, IDC_INPUT, DTM_SETFORMAT, 0, (LPARAM)date_format);
			}

			// print latest timestamp
			_r_unixtime_to_systemtime (_app_getlastesttimestamp (), &current_time);

			_app_converttime (&current_time, 0, &system_time);

			_app_printdate (hwnd, &system_time);

			// set control tip
			htip = _r_ctrl_createtip (hwnd);

			if (htip)
				_r_ctrl_settiptext (htip, hwnd, IDC_CURRENT, LPSTR_TEXTCALLBACK);

			break;
		}

		case WM_DESTROY:
		{
			SYSTEMTIME system_time;
			SYSTEMTIME current_time;

			if (_app_getdate (hwnd, IDC_INPUT, &system_time))
			{
				_app_converttime (&system_time, 0, &current_time);

				_r_config_setlong64 (L"LatestTimestamp", _r_unixtime_from_systemtime (&current_time));
			}

			PostQuitMessage (0);

			break;
		}

		case RM_INITIALIZE:
		{
			// configure menu
			HMENU hmenu;

			hmenu = GetMenu (hwnd);

			if (!hmenu)
				break;

			_r_menu_checkitem (hmenu, IDM_ALWAYSONTOP_CHK, 0, MF_BYCOMMAND, _r_config_getboolean (L"AlwaysOnTop", FALSE));
			_r_menu_checkitem (hmenu, IDM_CHECKUPDATES_CHK, 0, MF_BYCOMMAND, _r_update_isenabled (FALSE));

			break;
		}

		case RM_LOCALIZE:
		{
			// configure menu
			HMENU hmenu;
			HMENU hsubmenu;

			hmenu = GetMenu (hwnd);

			if (hmenu)
			{
				_r_menu_setitemtext (hmenu, 0, TRUE, _r_locale_getstring (IDS_FILE));
				_r_menu_setitemtext (hmenu, 1, TRUE, _r_locale_getstring (IDS_SETTINGS));
				_r_menu_setitemtext (hmenu, 2, TRUE, _r_locale_getstring (IDS_HELP));
				_r_menu_setitemtextformat (hmenu, IDM_EXIT, FALSE, L"%s\tEsc", _r_locale_getstring (IDS_EXIT));
				_r_menu_setitemtext (hmenu, IDM_ALWAYSONTOP_CHK, FALSE, _r_locale_getstring (IDS_ALWAYSONTOP_CHK));
				_r_menu_setitemtext (hmenu, IDM_CHECKUPDATES_CHK, FALSE, _r_locale_getstring (IDS_CHECKUPDATES_CHK));
				_r_menu_setitemtext (hmenu, IDM_WEBSITE, FALSE, _r_locale_getstring (IDS_WEBSITE));
				_r_menu_setitemtext (hmenu, IDM_CHECKUPDATES, FALSE, _r_locale_getstring (IDS_CHECKUPDATES));
				_r_menu_setitemtextformat (hmenu, IDM_ABOUT, FALSE, L"%s\tF1", _r_locale_getstring (IDS_ABOUT));

				hsubmenu = GetSubMenu (hmenu, 1);

				if (hsubmenu)
				{
					_r_menu_setitemtext (hsubmenu, TIMEZONE_MENU, TRUE, _r_locale_getstring (IDS_TIMEZONE));
					_r_menu_setitemtextformat (hsubmenu, LANG_MENU, TRUE, L"%s (Language)", _r_locale_getstring (IDS_LANGUAGE));

					_r_locale_enum (hsubmenu, LANG_MENU, IDX_LANGUAGE); // enum localizations
				}
			}

			// configure listview
			for (ENUM_DATE_TYPE i = 0; i < TypeMax; i++)
				_r_listview_setitem (hwnd, IDC_LISTVIEW, i, 0, _app_gettimedescription (i, FALSE));

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
			HMENU hmenu;
			HMENU hsubmenu;

			if (GetDlgCtrlID ((HWND)wparam) != IDC_LISTVIEW)
				break;

			// localize
			hmenu = LoadMenu (NULL, MAKEINTRESOURCE (IDM_LISTVIEW));

			if (!hmenu)
				break;

			hsubmenu = GetSubMenu (hmenu, 0);

			if (hsubmenu)
			{
				_r_menu_setitemtextformat (hsubmenu, IDM_COPY, FALSE, L"%s\tCtrl+C", _r_locale_getstring (IDS_COPY));

				if (!_r_listview_getselectedcount (hwnd, IDC_LISTVIEW))
					_r_menu_enableitem (hsubmenu, IDM_COPY, MF_BYCOMMAND, FALSE);

				_r_menu_popup (hsubmenu, hwnd, NULL, TRUE);
			}

			DestroyMenu (hmenu);

			break;
		}

		case WM_NOTIFY:
		{
			LPNMHDR nmlp = (LPNMHDR)lparam;

			switch (nmlp->code)
			{
				case DTN_USERSTRING:
				{
					LPNMDATETIMESTRING lpds;
					R_STRINGREF user_string;

					lpds = (LPNMDATETIMESTRING)lparam;

					_r_obj_initializestringref (&user_string, (LPWSTR)lpds->pszUserString);

					if (_r_str_isnumeric (&user_string))
						_r_unixtime_to_systemtime (_r_str_tolong64 (&user_string), &lpds->st);

					break;
				}

				case DTN_DATETIMECHANGE:
				{
					LPNMDATETIMECHANGE lpnmdtc;
					SYSTEMTIME system_time;

					lpnmdtc = (LPNMDATETIMECHANGE)lparam;

					_app_converttime (&lpnmdtc->st, 0, &system_time);

					_app_printdate (hwnd, &system_time);

					break;
				}

				case LVN_GETINFOTIP:
				{
					LPNMLVGETINFOTIP lpnmlv;

					lpnmlv = (LPNMLVGETINFOTIP)lparam;

					_r_str_copy (lpnmlv->pszText, lpnmlv->cchTextMax, _app_gettimedescription (lpnmlv->iItem, TRUE));

					break;
				}

				case TTN_GETDISPINFO:
				{
					LPNMTTDISPINFO lpnmdi;
					INT ctrl_id;

					lpnmdi = (LPNMTTDISPINFO)lparam;

					if (!(lpnmdi->uFlags & TTF_IDISHWND) != 0)
						break;

					ctrl_id = GetDlgCtrlID ((HWND)lpnmdi->hdr.idFrom);

					if (ctrl_id == IDC_CURRENT)
						lpnmdi->lpszText = _r_locale_getstring (IDS_CURRENT);

					break;
				}
			}

			break;
		}

		case WM_COMMAND:
		{
			INT ctrl_id = LOWORD (wparam);

			if (HIWORD (wparam) == 0 && ctrl_id >= IDX_LANGUAGE && ctrl_id <= IDX_LANGUAGE + (INT)_r_locale_getcount () + 1)
			{
				_r_locale_apply (GetSubMenu (GetSubMenu (GetMenu (hwnd), 1), LANG_MENU), ctrl_id, IDX_LANGUAGE);

				return FALSE;
			}
			else if ((ctrl_id >= IDX_TIMEZONE && ctrl_id <= IDX_TIMEZONE + (RTL_NUMBER_OF (int_timezones) - 1)))
			{
				SYSTEMTIME current_time;
				SYSTEMTIME system_time;
				HMENU submenu_timezone;
				LONG bias;
				UINT idx;

				idx = ctrl_id - IDX_TIMEZONE;
				bias = int_timezones[idx];

				_r_config_setlong (L"TimezoneBias", bias);

				submenu_timezone = GetSubMenu (GetSubMenu (GetMenu (hwnd), 1), TIMEZONE_MENU);

				if (submenu_timezone)
					_r_menu_checkitem (submenu_timezone, IDX_TIMEZONE, IDX_TIMEZONE + (RTL_NUMBER_OF (int_timezones) - 1), MF_BYCOMMAND, ctrl_id);

				if (_app_getdate (hwnd, IDC_INPUT, &current_time))
				{
					_app_converttime (&current_time, bias, &system_time);

					_app_printdate (hwnd, &system_time);
				}

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
					BOOLEAN new_val;

					new_val = !_r_config_getboolean (L"AlwaysOnTop", FALSE);

					_r_menu_checkitem (GetMenu (hwnd), ctrl_id, 0, MF_BYCOMMAND, new_val);

					_r_config_setboolean (L"AlwaysOnTop", new_val);

					_r_wnd_top (hwnd, new_val);

					break;
				}

				case IDM_CHECKUPDATES_CHK:
				{
					BOOLEAN new_val;

					new_val = !_r_update_isenabled (FALSE);

					_r_update_enable (new_val);

					_r_menu_checkitem (GetMenu (hwnd), ctrl_id, 0, MF_BYCOMMAND, new_val);

					break;
				}

				case IDM_WEBSITE:
				{
					_r_shell_opendefault (_r_app_getwebsite_url ());
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
					R_STRINGBUILDER sb;
					PR_STRING string;
					INT item_id = -1;

					_r_obj_initializestringbuilder (&sb, 512);

					while ((item_id = _r_listview_getnextselected (hwnd, IDC_LISTVIEW, item_id)) != -1)
					{
						string = _r_listview_getitemtext (hwnd, IDC_LISTVIEW, item_id, 1);

						if (string)
						{
							_r_obj_appendstringbuilder2 (&sb, string);
							_r_obj_appendstringbuilder (&sb, L"\r\n");

							_r_obj_dereference (string);
						}
					}

					string = _r_obj_finalstringbuilder (&sb);

					_r_str_trimstring2 (string, L"\r\n ", 0);

					_r_clipboard_set (hwnd, &string->sr);

					_r_obj_dereference (string);

					break;
				}

				case IDC_CURRENT:
				{
					SYSTEMTIME current_time;
					SYSTEMTIME system_time;

					GetSystemTime (&current_time);

					_app_converttime (&current_time, _app_getcurrentbias (), &system_time);

					_app_printdate (hwnd, &system_time);

					break;
				}

				case IDM_SELECT_ALL:
				{
					if (GetFocus () != GetDlgItem (hwnd, IDC_LISTVIEW))
						break;

					_r_listview_setitemstate (hwnd, IDC_LISTVIEW, -1, LVIS_SELECTED, LVIS_SELECTED);

					break;
				}
			}
		}
	}

	return FALSE;
}

INT APIENTRY wWinMain (
	_In_ HINSTANCE hinst,
	_In_opt_ HINSTANCE prev_hinst,
	_In_ LPWSTR cmdline,
	_In_ INT show_cmd
)
{
	HWND hwnd;

	if (!_r_app_initialize (NULL))
		return ERROR_APP_INIT_FAILURE;

	hwnd = _r_app_createwindow (hinst, MAKEINTRESOURCE (IDD_MAIN), MAKEINTRESOURCE (IDI_MAIN), &DlgProc);

	if (!hwnd)
		return ERROR_APP_INIT_FAILURE;

	return _r_wnd_message_callback (hwnd, MAKEINTRESOURCE (IDA_MAIN));
}
