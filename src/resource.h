#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#ifndef IDC_STATIC
#define IDC_STATIC (-1)
#endif

// Dialogs
#define IDD_MAIN	                            100
#define IDD_SETTINGS                            101

// Menus
#define IDM_MAIN		                        100

// Main Dlg
#define IDC_WINDOWS_RB							100
#define IDC_WINDOWS_TIME	                    101
#define IDC_WINDOWS_CURRENT_BTN	                102

#define IDC_UNIX_RB								103
#define IDC_UNIX_TIME							104
#define IDC_UNIX_CURRENT_BTN					105

#define IDC_CONVERT								106

// Settings Dlg
#define IDC_CHECK_UPDATE_AT_STARTUP_CHK			100
#define IDC_ALWAYS_ON_TOP_CHK					101

#define IDC_ENABLE_WINDOWS_EDIT_CHK				102
#define IDC_ENABLE_FORMAT_CHK					103
#define IDC_WINDOWS_FORMAT_EDIT					104
#define IDC_WINDOWS_FORMAT_STATIC				105

#define IDC_LANGUAGE_CB							106
#define IDC_LANGUAGE_INFO						107

// Common Controls
#define IDC_OK									200
#define IDC_CANCEL								201

#define IDC_LABEL_1								203
#define IDC_LABEL_2								204
#define IDC_LABEL_3								205

// Main Menu
#define IDM_SETTINGS							40000
#define IDM_EXIT                                40001
#define IDM_COPY_WINDOWS						40002
#define IDM_COPY_UNIX			                40003
#define IDM_WEBSITE                             40004
#define IDM_CHECK_UPDATES		                40005
#define IDM_ABOUT                               40006

// Strings
#define IDS_TRANSLATION_INFO					1000

#define IDS_UPDATE_NO                           10000
#define IDS_UPDATE_YES                          10001
#define IDS_UPDATE_ERROR                        10002

#define IDS_ABOUT								10003
#define IDS_COPYRIGHT							10004

#define IDS_WINDOWS_FORMAT						10005

#define IDS_W2U									10006
#define IDS_U2W									10007

#define IDS_CURRENT_TIME						10008
#define IDS_REQUIRED_FIELD						10009

// Icons
#define IDI_MAIN	                            100

#endif
