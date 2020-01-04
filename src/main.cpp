// TimeVertor
// Copyright (c) 2012-2020 Henry++

#include <windows.h>

#include "main.hpp"
#include "rapp.hpp"
#include "routine.hpp"

#include "resource.hpp"

rapp app;

size_t current_bias_idx;
time_t current_timestamp_utc;

rstring _app_timezone2string (LONG bias, bool divide, LPCWSTR utcname)
{
	WCHAR result[32] = {0};

	if (bias == 0)
	{
		_r_str_copy (result, _countof (result), utcname);
	}
	else
	{
		const WCHAR symbol = ((bias > 0) ? L'-' : L'+');
		const u_int tzHours = (u_int)floor (long double ((abs (bias)) / 60.0));
		const u_int tzMinutes = (abs (bias) % 60L);

		_r_str_printf (result, _countof (result), L"%c%02u%s%02u", symbol, tzHours, divide ? L":" : L"", tzMinutes);
	}

	return result;
}

LONG _app_getdefaultbias ()
{
	TIME_ZONE_INFORMATION tz = {0};
	GetTimeZoneInformation (&tz);

	return tz.Bias;
}

LONG _app_getcurrentbias ()
{
	return app.ConfigGet (L"TimezoneBias", _app_getdefaultbias ()).AsLong ();
}

void _app_gettime (time_t unixtime, LONG bias, LPSYSTEMTIME lpst)
{
	SYSTEMTIME st = {0};
	_r_unixtime_to_systemtime (unixtime, &st);

	current_timestamp_utc = unixtime; // store position of current time (utc)

	TIME_ZONE_INFORMATION tz = {0};
	tz.Bias = bias; // set timezone shift

	SystemTimeToTzSpecificLocalTime (&tz, &st, lpst);
}

rstring _app_timeconvert (time_t ut, LONG bias, LPSYSTEMTIME lpst, PULARGE_INTEGER pul, EnumDateType type)
{
	rstring result;

	if (type == TypeRfc2822)
		result.Format (L"%s, %02d %s %04d %02d:%02d:%02d %s", str_dayofweek[lpst->wDayOfWeek], lpst->wDay, str_month[lpst->wMonth - 1], lpst->wYear, lpst->wHour, lpst->wMinute, lpst->wSecond, _app_timezone2string (bias, false, L"GMT").GetString ());

	else if (type == TypeIso8601)
		result.Format (L"%04d-%02d-%02dT%02d:%02d:%02d%s", lpst->wYear, lpst->wMonth, lpst->wDay, lpst->wHour, lpst->wMinute, lpst->wSecond, _app_timezone2string (bias, true, L"Z").GetString ());

	else if (type == TypeUnixtime)
		result.Format (L"%" PRId64, (std::max) (ut, 0LL));

	else if (type == TypeMactime)
		result.Format (L"%" PRId64, (std::max) (ut + MAC_TIMESTAMP, 0LL));

	else if (type == TypeMicrosofttime)
		result.Format (L"%.09f", (std::max) ((double (pul->QuadPart) / (24.0 * (60.0 * (60.0 * 10000000.0)))) - MICROSOFT_TIMESTAMP, 0.0));

	else if (type == TypeFiletime)
		result.Format (L"%" PRIu64, (std::max) (pul->QuadPart, 0ULL));

	return result;
}

rstring _app_gettimedescription (EnumDateType type, bool is_desc)
{
	rstring result;

	if (type == TypeRfc2822)
		result = app.LocaleString (is_desc ? IDS_FMTDESC_RFC2822 : IDS_FMTNAME_RFC2822, nullptr);

	else if (type == TypeIso8601)
		result = app.LocaleString (is_desc ? IDS_FMTDESC_ISO8601 : IDS_FMTNAME_ISO8601, nullptr);

	else if (type == TypeUnixtime)
		result = app.LocaleString (is_desc ? IDS_FMTDESC_UNIXTIME : IDS_FMTNAME_UNIXTIME, nullptr);

	else if (type == TypeMactime)
		result = app.LocaleString (is_desc ? IDS_FMTDESC_MACTIME : IDS_FMTNAME_MACTIME, nullptr);

	else if (type == TypeMicrosofttime)
		result = app.LocaleString (is_desc ? IDS_FMTDESC_MICROSOFTTIME : IDS_FMTNAME_MICROSOFTTIME, nullptr);

	else if (type == TypeFiletime)
		result = app.LocaleString (is_desc ? IDS_FMTDESC_FILETIME : IDS_FMTNAME_FILETIME, nullptr);

	return result;
}

void _app_printdate (HWND hwnd, LPSYSTEMTIME lpst)
{
	SendDlgItemMessage (hwnd, IDC_INPUT, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)lpst);

	const time_t unixtime = _r_unixtime_from_systemtime (lpst);
	const LONG bias = _app_getcurrentbias ();

	FILETIME filetime = {0};
	SystemTimeToFileTime (lpst, &filetime);

	ULARGE_INTEGER ul = {0};
	ul.LowPart = filetime.dwLowDateTime;
	ul.HighPart = filetime.dwHighDateTime;

	for (INT i = 0; i < TypeMax; i++)
		_r_listview_setitem (hwnd, IDC_LISTVIEW, i, 1, _app_timeconvert (unixtime, bias, lpst, &ul, (EnumDateType)i));
}

INT_PTR CALLBACK DlgProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
		case WM_INITDIALOG:
		{
#ifndef _APP_NO_DARKTHEME
			_r_wnd_setdarktheme (hwnd);
#endif // _APP_NO_DARKTHEME

			// configure listview
			_r_listview_setstyle (hwnd, IDC_LISTVIEW, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP);

			_r_listview_addcolumn (hwnd, IDC_LISTVIEW, 1, nullptr, -39, LVCFMT_LEFT);
			_r_listview_addcolumn (hwnd, IDC_LISTVIEW, 2, nullptr, -61, LVCFMT_RIGHT);

			for (INT i = 0; i < TypeMax; i++)
				_r_listview_additem (hwnd, IDC_LISTVIEW, i, 0, nullptr);

			// configure datetime format
			{
				WCHAR date_format[MAX_PATH] = {0};
				WCHAR time_format[MAX_PATH] = {0};

				if (
					GetLocaleInfo (LOCALE_SYSTEM_DEFAULT, LOCALE_SLONGDATE, date_format, _countof (date_format)) &&
					GetLocaleInfo (LOCALE_SYSTEM_DEFAULT, LOCALE_STIMEFORMAT, time_format, _countof (time_format))
					)
				{
					WCHAR buffer[MAX_PATH] = {0};
					_r_str_printf (buffer, _countof (buffer), L"%s %s", date_format, time_format);

					SendDlgItemMessage (hwnd, IDC_INPUT, DTM_SETFORMAT, 0, (LPARAM)buffer);
				}
			}

			// print latest timestamp
			{
				SYSTEMTIME st = {0};

				current_timestamp_utc = app.ConfigGet (L"LatestTimestamp", _r_unixtime_now ()).AsLonglong ();

				//_app_gettime (current_timestamp_utc, 0, &st);
				//_app_printdate (hwnd, &st);
			}

			const HWND htip = _r_ctrl_createtip (hwnd);

			if (htip)
				_r_ctrl_settip (htip, hwnd, IDC_CURRENT, LPSTR_TEXTCALLBACK);

			break;
		}

		case WM_NCCREATE:
		{
			_r_wnd_enablenonclientscaling (hwnd);
			break;
		}

		case WM_DESTROY:
		{
			// save latest timestamp
			{
				SYSTEMTIME st = {0};
				//SendDlgItemMessage (hwnd, IDC_INPUT, DTM_GETSYSTEMTIME, 0, (LPARAM)&st);

				_app_gettime (current_timestamp_utc, 0, &st);

				app.ConfigSet (L"LatestTimestamp", _r_unixtime_from_systemtime (&st));
			}

			PostQuitMessage (0);

			break;
		}

		case RM_INITIALIZE:
		{
			// configure menu
			CheckMenuItem (GetMenu (hwnd), IDM_ALWAYSONTOP_CHK, MF_BYCOMMAND | (app.ConfigGet (L"AlwaysOnTop", false).AsBool () ? MF_CHECKED : MF_UNCHECKED));
			CheckMenuItem (GetMenu (hwnd), IDM_CHECKUPDATES_CHK, MF_BYCOMMAND | (app.ConfigGet (L"CheckUpdates", true).AsBool () ? MF_CHECKED : MF_UNCHECKED));
			CheckMenuItem (GetMenu (hwnd), IDM_CLASSICUI_CHK, MF_BYCOMMAND | (app.ConfigGet (L"ClassicUI", _APP_CLASSICUI).AsBool () ? MF_CHECKED : MF_UNCHECKED));

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

				const LONG current_bias = _app_getcurrentbias ();
				const LONG default_bias = _app_getdefaultbias ();

				for (size_t i = 0; i < _countof (int_timezones); i++)
				{
					const LONG bias = int_timezones[i];

					MENUITEMINFO mii = {0};

					WCHAR menu_title[32] = {0};
					_r_str_printf (menu_title, _countof (menu_title), L"GMT %s", _app_timezone2string (bias, true, L"+00:00 (UTC)").GetString ());

					if (bias == default_bias)
						_r_str_cat (menu_title, _countof (menu_title), SYSTEM_BIAS);

					mii.cbSize = sizeof (mii);
					mii.fMask = MIIM_ID | MIIM_STRING;
					mii.fType = MFT_STRING;
					mii.fState = MFS_DEFAULT;
					mii.dwTypeData = menu_title;
					mii.wID = IDX_TIMEZONE + UINT (i);

					InsertMenuItem (submenu_timezone, mii.wID, FALSE, &mii);

					if (bias == current_bias)
					{
						current_bias_idx = i;

						CheckMenuRadioItem (submenu_timezone, IDX_TIMEZONE, IDX_TIMEZONE + UINT (_countof (int_timezones) - 1), mii.wID, MF_BYCOMMAND);
					}
				}

				SYSTEMTIME st = {0};

				_app_gettime (current_timestamp_utc, current_bias, &st);
				_app_printdate (hwnd, &st);
			}

			break;
		}

		case RM_LOCALIZE:
		{
			// configure menu
			const HMENU menu = GetMenu (hwnd);

			app.LocaleMenu (menu, IDS_FILE, 0, true, nullptr);
			app.LocaleMenu (menu, IDS_EXIT, IDM_EXIT, false, L"\tEsc");
			app.LocaleMenu (menu, IDS_SETTINGS, 1, true, nullptr);
			app.LocaleMenu (menu, IDS_ALWAYSONTOP_CHK, IDM_ALWAYSONTOP_CHK, false, nullptr);
			app.LocaleMenu (menu, IDS_CHECKUPDATES_CHK, IDM_CHECKUPDATES_CHK, false, nullptr);
			app.LocaleMenu (menu, IDS_CLASSICUI_CHK, IDM_CLASSICUI_CHK, false, nullptr);
			app.LocaleMenu (GetSubMenu (menu, 1), IDS_TIMEZONE, TIMEZONE_MENU, true, nullptr);
			app.LocaleMenu (GetSubMenu (menu, 1), IDS_LANGUAGE, LANG_MENU, true, L" (Language)");
			app.LocaleMenu (menu, IDS_HELP, 2, true, nullptr);
			app.LocaleMenu (menu, IDS_WEBSITE, IDM_WEBSITE, false, nullptr);
			app.LocaleMenu (menu, IDS_CHECKUPDATES, IDM_CHECKUPDATES, false, nullptr);
			app.LocaleMenu (menu, IDS_ABOUT, IDM_ABOUT, false, L"\tF1");

			app.LocaleEnum ((HWND)GetSubMenu (menu, 1), LANG_MENU, true, IDX_LANGUAGE); // enum localizations

			// configure listview
			for (INT i = 0; i < TypeMax; i++)
				_r_listview_setitem (hwnd, IDC_LISTVIEW, i, 0, _app_gettimedescription ((EnumDateType)i, false));

			_r_wnd_addstyle (hwnd, IDC_CURRENT, app.IsClassicUI () ? WS_EX_STATICEDGE : 0, WS_EX_STATICEDGE, GWL_EXSTYLE);

			RedrawWindow (hwnd, nullptr, nullptr, RDW_ERASENOW | RDW_INVALIDATE);

			break;
		}

		case RM_DPICHANGED:
		{
			_r_listview_setcolumn (hwnd, IDC_LISTVIEW, 0, nullptr, -39);
			_r_listview_setcolumn (hwnd, IDC_LISTVIEW, 1, nullptr, -61);

			break;
		}

		case WM_CONTEXTMENU:
		{
			if (GetDlgCtrlID ((HWND)wparam) == IDC_LISTVIEW)
			{
				const HMENU hmenu = LoadMenu (nullptr, MAKEINTRESOURCE (IDM_LISTVIEW));
				const HMENU hsubmenu = GetSubMenu (hmenu, 0);

				// localize
				app.LocaleMenu (hsubmenu, IDS_COPY, IDM_COPY, false, L"\tCtrl+C");

				if (!SendDlgItemMessage (hwnd, IDC_LISTVIEW, LVM_GETSELECTEDCOUNT, 0, 0))
					EnableMenuItem (hsubmenu, IDM_COPY, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

				POINT pt = {0};
				GetCursorPos (&pt);

				TrackPopupMenuEx (hsubmenu, TPM_RIGHTBUTTON | TPM_LEFTBUTTON, pt.x, pt.y, hwnd, nullptr);

				DestroyMenu (hmenu);
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

					if (lpds)
					{
						const rstring datetime = lpds->pszUserString;

						if (_r_str_isnumeric (datetime))
							_r_unixtime_to_systemtime (datetime.AsLonglong (), &lpds->st);
					}

					break;
				}

				case DTN_DATETIMECHANGE:
				{
					LPNMDATETIMECHANGE lpnmdtc = (LPNMDATETIMECHANGE)lparam;

					current_timestamp_utc = _r_unixtime_from_systemtime (&lpnmdtc->st); // store position of current time (utc)

					_app_printdate (hwnd, &lpnmdtc->st);

					break;
				}

				case LVN_GETINFOTIP:
				{
					LPNMLVGETINFOTIP lpnmlv = (LPNMLVGETINFOTIP)lparam;

					_r_str_copy (lpnmlv->pszText, lpnmlv->cchTextMax, _app_gettimedescription ((EnumDateType)lpnmlv->iItem, true));

					break;
				}

				case TTN_GETDISPINFO:
				{
					LPNMTTDISPINFO lpnmdi = (LPNMTTDISPINFO)lparam;

					if ((lpnmdi->uFlags & TTF_IDISHWND) != 0)
					{
						WCHAR buffer[1024] = {0};
						const INT ctrl_id = GetDlgCtrlID ((HWND)lpnmdi->hdr.idFrom);

						if (ctrl_id == IDC_CURRENT)
							_r_str_copy (buffer, _countof (buffer), app.LocaleString (IDS_CURRENT, nullptr));

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
			if (HIWORD (wparam) == 0 && LOWORD (wparam) >= IDX_LANGUAGE && LOWORD (wparam) <= IDX_LANGUAGE + app.LocaleGetCount ())
			{
				app.LocaleApplyFromMenu (GetSubMenu (GetSubMenu (GetMenu (hwnd), 1), LANG_MENU), LOWORD (wparam), IDX_LANGUAGE);

				return FALSE;
			}
			else if ((LOWORD (wparam) >= IDX_TIMEZONE && LOWORD (wparam) <= IDX_TIMEZONE + (_countof (int_timezones) - 1)))
			{
				const UINT idx = LOWORD (wparam) - IDX_TIMEZONE;
				const LONG bias = int_timezones[idx];

				current_bias_idx = idx;
				app.ConfigSet (L"TimezoneBias", bias);

				const HMENU submenu_timezone = GetSubMenu (GetSubMenu (GetMenu (hwnd), 1), TIMEZONE_MENU);
				CheckMenuRadioItem (submenu_timezone, IDX_TIMEZONE, IDX_TIMEZONE + UINT (_countof (int_timezones) - 1), LOWORD (wparam), MF_BYCOMMAND);

				SYSTEMTIME st = {0};

				_app_gettime (current_timestamp_utc, bias, &st);
				_app_printdate (hwnd, &st);

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

					CheckMenuItem (GetMenu (hwnd), LOWORD (wparam), MF_BYCOMMAND | (new_val ? MF_CHECKED : MF_UNCHECKED));
					app.ConfigSet (L"AlwaysOnTop", new_val);

					_r_wnd_top (hwnd, new_val);

					break;
				}

				case IDM_CHECKUPDATES_CHK:
				{
					const bool new_val = !app.ConfigGet (L"CheckUpdates", true).AsBool ();

					CheckMenuItem (GetMenu (hwnd), LOWORD (wparam), MF_BYCOMMAND | (new_val ? MF_CHECKED : MF_UNCHECKED));
					app.ConfigSet (L"CheckUpdates", new_val);

					break;
				}

				case IDM_CLASSICUI_CHK:
				{
					const bool new_val = !app.ConfigGet (L"ClassicUI", _APP_CLASSICUI).AsBool ();

					CheckMenuItem (GetMenu (hwnd), LOWORD (wparam), MF_BYCOMMAND | (new_val ? MF_CHECKED : MF_UNCHECKED));
					app.ConfigSet (L"ClassicUI", new_val);

					break;
				}

				case IDM_WEBSITE:
				{
					ShellExecute (hwnd, nullptr, _APP_WEBSITE_URL, nullptr, nullptr, SW_SHOWDEFAULT);
					break;
				}

				case IDM_CHECKUPDATES:
				{
					app.UpdateCheck (hwnd);
					break;
				}

				case IDM_ABOUT:
				{
					app.CreateAboutWindow (hwnd);
					break;
				}

				case IDM_COPY:
				{
					rstring buffer;

					INT item = INVALID_INT;

					while ((item = (INT)SendDlgItemMessage (hwnd, IDC_LISTVIEW, LVM_GETNEXTITEM, (WPARAM)item, LVNI_SELECTED)) != INVALID_INT)
						buffer.AppendFormat (L"%s\r\n", _r_listview_getitemtext (hwnd, IDC_LISTVIEW, item, 1).GetString ());

					if (!buffer.IsEmpty ())
					{
						_r_str_trim (buffer, L"\r\n");

						_r_clipboard_set (hwnd, buffer, buffer.GetLength ());
					}

					break;
				}

				case IDC_CURRENT:
				{
					SYSTEMTIME st = {0};

					_app_gettime (_r_unixtime_now (), _app_getcurrentbias (), &st);
					_app_printdate (hwnd, &st);

					break;
				}

				case IDM_SELECT_ALL:
				{
					ListView_SetItemState (GetDlgItem (hwnd, IDC_LISTVIEW), INVALID_INT, LVIS_SELECTED, LVIS_SELECTED);
					break;
				}

				case IDM_TIMEZONE_NEXT:
				case IDM_TIMEZONE_PREV:
				{
					if (LOWORD (wparam) == IDM_TIMEZONE_NEXT)
						current_bias_idx = ((current_bias_idx == (_countof (int_timezones) - 1)) ? 0 : ++current_bias_idx);

					else
						current_bias_idx = (!current_bias_idx ? (_countof (int_timezones) - 1) : --current_bias_idx);

					SendMessage (hwnd, WM_COMMAND, MAKEWPARAM (IDX_TIMEZONE + current_bias_idx, 0), 0);

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

	if (app.Initialize (APP_NAME, APP_NAME_SHORT, APP_VERSION, APP_COPYRIGHT))
	{
		if (app.CreateMainWindow (IDD_MAIN, IDI_MAIN, &DlgProc))
		{
			const HACCEL haccel = LoadAccelerators (app.GetHINSTANCE (), MAKEINTRESOURCE (IDA_MAIN));

			if (haccel)
			{
				while (GetMessage (&msg, nullptr, 0, 0) > 0)
				{
					TranslateAccelerator (app.GetHWND (), haccel, &msg);

					if (!IsDialogMessage (app.GetHWND (), &msg))
					{
						TranslateMessage (&msg);
						DispatchMessage (&msg);
					}
				}

				DestroyAcceleratorTable (haccel);
			}
		}
	}

	return (INT)msg.wParam;
}
