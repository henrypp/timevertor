// TimeVertor
// Copyright (c) 2012-2018 Henry++

#ifndef __MAIN_H__
#define __MAIN_H__

#include <windows.h>
#include <commctrl.h>
#include "resource.hpp"
#include "app.hpp"

#define TIMEZONE_MENU 4
#define LANG_MENU 5

#define SYSTEM_BIAS  L" (def.)"
#define MAC_TIMESTAMP 2082844800LL
#define MICROSOFT_TIMESTAMP 109206.000000

enum EnumDateType
{
	TypeRfc2822 = 0,
	TypeIso8601 = 1,
	TypeUnixtime = 2,
	TypeMactime = 3,
	TypeMicrosofttime = 4,
	TypeFiletime = 5,
	TypeMax = 6,
};

static const LONG int_timezones[] = {
	720, // -12:00
	660, // -11:00
	600, // -10:00
	570, // -09:30
	540, // -09:00
	480, // -08:00
	420, // -07:00
	360, // -06:00
	300, // -05:00
	240, // -04:00
	210, // -03:30
	180, // -03:00
	120, // -02:00
	60, // -01:00
	0, // +00:00
	-60, // +01:00
	-120, // +02:00
	-180, // +03:00
	-210, // +03:30
	-240, // +04:00
	-270, // +04:30
	-300, // +05:00
	-330, // +05:30
	-345, // +05:45
	-360, // +06:00
	-390, // +06:30
	-420, // +07:00
	-480, // +08:00
	-510, // +08:30
	-525, // +08:45
	-540, // +09:00
	-570, // +09:30
	-600, // +10:00
	-630, // +10:30
	-660, // +11:00
	-720, // +12:00
	-765, // +12:45
	-780, // +13:00
	-840, // +14:00
};

static LPCWSTR str_dayofweek[] = {
	L"Sun",
	L"Mon",
	L"Tue",
	L"Wed",
	L"Thu",
	L"Fri",
	L"Sat",
};

LPCWSTR str_month[] = {
	L"Jan",
	L"Feb",
	L"Mar",
	L"Apr",
	L"May",
	L"Jun",
	L"Jul",
	L"Aug",
	L"Sep",
	L"Oct",
	L"Nov",
	L"Dec",
};

#endif // __MAIN_H__
