#define ID_LISTBOX_PROJECTS 20
#define ID_LISTBOX_PAIRS_SOURCE 21
#define ID_LISTBOX_PAIRS_DEST 22
#define ID_LISTBOX_SYNC 23
#define ID_LISTBOX_ADD_SOURCE 24
#define ID_LISTBOX_ADD_DEST 25
#define ID_BUTTON_PREVIEW 30
#define ID_BUTTON_SYNC 31
#define ID_BUTTON_ADD_PROJECT 32
#define ID_BUTTON_ADD_FOLDER_PAIR 33
#define ID_BUTTON_ADD_PAIR 34
#define ID_BUTTON_PAIR_ADD 35
#define ID_BUTTON_OK 36
#define ID_BUTTON_CANCEL 37
#define ID_BUTTON_DELETE 38
#define ID_LABEL_ADD_PROJECT_NAME 40
#define ID_LABEL_PAIR_SOURCE 41
#define ID_LABEL_PAIR_DEST 42
#define ID_EDIT_ADD_PROJECT_NAME 43
#define ID_EDIT_PAIR_SOURCE 44
#define ID_EDIT_PAIR_DEST 45
#define ID_PROGRESS_BAR 50
#define ID_TIMER1 51
#define ID_SETTINGS_DESKTOP_LABEL 60
#define ID_SETTINGS_DESKTOP_CHECKBOX 61
#define ID_SETTINGS_SYMBOLIC_LABEL 62
#define ID_SETTINGS_SYMBOLIC_CHECKBOX 63
#define ID_SETTINGS_SMALL_DIFF_LABEL 64
#define ID_SETTINGS_SMALL_DIFF_CHECKBOX 65
#define TAB_TOP 5
#define TAB_LEFT 5
#define TAB_MARGIN 10
#define LISTBOX_TOP 80
#define LISTBOX_LEFT 10
#define BUTTON_WIDTH 80
#define BUTTON_HEIGHT 25
#define BIG_BUTTON_WIDTH 120
#define BIG_BUTTON_HEIGHT 30
#define PROGRESS_BAR_HEIGHT 25
#define RIGHT_MARGIN 20

#include "kitchen_sync.h"
#pragma comment(lib, "comctl32.lib")

static struct ProjectNode *projectsHead = NULL;
static struct Pair *pairIndex[MAX_PAIRS];

static LRESULT CALLBACK mainWndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK customListboxProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK projectNameWndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK customProjectNameProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK folderPairWndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK customSourceEditboxProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK customDestinationEditboxProc(HWND, UINT, WPARAM, LPARAM);
static WNDPROC originalListboxProc;
static WNDPROC originalProjectNameProc;
static WNDPROC originalSourceEditboxProc;
static WNDPROC originalDestinationEditboxProc;
static HWND mainHwnd;
static HWND projectNameHwnd;
static HWND folderPairHwnd;
static HWND lbProjectsHwnd;
static HWND lbSourceHwnd;
static HWND lbDestHwnd;
static HWND lbPairSourceHwnd;
static HWND lbPairDestHwnd;
static HWND lbSyncHwnd;
static HWND bProjectNameOK;
static HWND bDelete;
static HWND pbHwnd;
static HWND tabHwnd;
static HINSTANCE instance;
static HFONT hFont;

static void getProjectName(void);
static void addFolderPair(void);
static void resizeProjectTab();
static void resizePairsTab();
static void resizeSyncTab();
static void resizeSettingsTab();
static void resizeTab(int, int);

static bool showingFolderPair = false;
static bool showingProjectName = false;
static bool recentlySynced = false;
static bool editingFolderPair = false;
static wchar_t projectName[MAX_LINE] = {0};
static wchar_t folderPair[FOLDER_PAIR_SIZE] = {0};
static wchar_t sourceFolder[MAX_LINE] = {0};
static wchar_t destFolder[MAX_LINE] = {0};

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR pCmdLine, _In_ int nCmdShow)
{
	instance = hInstance;

	hFont = CreateFont(16, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_MODERN, L"Arial");

	WNDCLASSEX wc = {0};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	wc.hInstance = hInstance;
	wc.lpfnWndProc = mainWndProc;
	wc.lpszClassName = L"KitchenSync";
	wc.lpszMenuName = NULL;
	wc.style = CS_HREDRAW | CS_VREDRAW;

	if (!RegisterClassEx(&wc))
	{
		MessageBox(NULL, L"Window registration failed", L"Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	mainHwnd = CreateWindowEx(WS_EX_LEFT,
		wc.lpszClassName,
		L"KitchenSync v0.63",
		WS_OVERLAPPEDWINDOW,
		//WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT,
		NULL, NULL, hInstance, NULL);

	if (!mainHwnd)
	{
		MessageBox(NULL, L"Window creation failed", L"Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	pairCount = 0;
	memset(pairIndex, '\0', sizeof(struct Pair *) * MAX_PAIRS);
	startLoggingThread();
	ShowWindow(mainHwnd, nCmdShow);
	readSettings(mainHwnd, INI_FILE);
	loadProjects(lbProjectsHwnd, PRJ_FILE, &projectsHead);

	MSG msg = {0};

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

static LRESULT CALLBACK mainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static INITCOMMONCONTROLSEX icex = {0};
	static HWND bPreview, bSync, bAddProject, bAddFolders, bAddPair, lSkipDesktop, cbSkipDesktop, lSkipSymbolic, cbSkipSymbolic, lSkipSmallDiff, cbSkipSmallDiff;
	static bool listboxClicked = false;

	enum Tabs
	{
		TAB_PROJECTS,
		TAB_PAIRS,
		TAB_SYNC,
		TAB_SETTINGS
	};

	switch (msg)
	{
	case WM_CREATE:
		// initialize common controls
		icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
		icex.dwICC = ICC_TAB_CLASSES | ICC_PROGRESS_CLASS;
		InitCommonControlsEx(&icex);

		RECT rc = {0};
		GetClientRect(hwnd, &rc);

		tabHwnd = CreateWindow(WC_TABCONTROL, L"",
			WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS,
			TAB_TOP, TAB_LEFT, rc.right - TAB_MARGIN, rc.bottom - TAB_MARGIN, hwnd, NULL, instance, NULL);

		if (tabHwnd == NULL)
		{
			wchar_t buf[MAX_LINE] = L"Failed to create tab control";
			logger(buf);
			MessageBox(NULL, buf, L"Error", MB_ICONEXCLAMATION | MB_OK);
			break;
		}

		TCITEM tie = {0};
		tie.mask = TCIF_TEXT | TCIF_IMAGE;
		tie.iImage = -1;
		tie.pszText = L"Projects";
		TabCtrl_InsertItem(tabHwnd, 0, &tie);
		tie.pszText = L"Folder Pairs";
		TabCtrl_InsertItem(tabHwnd, 1, &tie);
		tie.pszText = L"Sync";
		TabCtrl_InsertItem(tabHwnd, 2, &tie);
		tie.pszText = L"Settings";
		TabCtrl_InsertItem(tabHwnd, 3, &tie);

		// projects

			// listbox
		lbProjectsHwnd = CreateWindowEx(WS_EX_LEFT, L"ListBox", NULL,
			WS_VISIBLE | WS_CHILD | LBS_NOTIFY | WS_VSCROLL | WS_BORDER,
			LISTBOX_LEFT, LISTBOX_TOP, rc.right - RIGHT_MARGIN, rc.bottom - LISTBOX_TOP - TAB_MARGIN, hwnd, (HMENU)ID_LISTBOX_PROJECTS, NULL, NULL);
		originalListboxProc = (WNDPROC)SetWindowLongPtr(lbProjectsHwnd, GWLP_WNDPROC, (LONG_PTR)customListboxProc);
		SendMessage(lbProjectsHwnd, WM_SETFONT, (WPARAM)hFont, 0);

		bAddProject = CreateWindowEx(WS_EX_LEFT, L"Button", L"Add Project",
			WS_VISIBLE | WS_CHILD | WS_TABSTOP,
			LISTBOX_LEFT, 40, BIG_BUTTON_WIDTH, BIG_BUTTON_HEIGHT, hwnd, (HMENU)ID_BUTTON_ADD_PROJECT, NULL, NULL);

		bAddFolders = CreateWindowEx(WS_EX_LEFT, L"Button", L"Add Folder Pair",
			WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_DISABLED,
			140, 40, BIG_BUTTON_WIDTH, BIG_BUTTON_HEIGHT, hwnd, (HMENU)ID_BUTTON_ADD_FOLDER_PAIR, NULL, NULL);

		bDelete = CreateWindowEx(WS_EX_LEFT, L"Button", L"Delete",
			WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_DISABLED,
			270, 40, BIG_BUTTON_WIDTH, BIG_BUTTON_HEIGHT, hwnd, (HMENU)ID_BUTTON_DELETE, NULL, NULL);

		bPreview = CreateWindowEx(WS_EX_LEFT, L"Button", L"Preview",
			WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_DISABLED,
			400, 40, BIG_BUTTON_WIDTH, BIG_BUTTON_HEIGHT, hwnd, (HMENU)ID_BUTTON_PREVIEW, NULL, NULL);

		// folder pairs

			// source listbox
		lbSourceHwnd = CreateWindowEx(WS_EX_LEFT, L"ListBox", NULL,
			WS_CHILD | LBS_NOTIFY | WS_VSCROLL | WS_BORDER,
			LISTBOX_LEFT, LISTBOX_TOP, (rc.right / 2) - TAB_MARGIN, rc.bottom - LISTBOX_TOP - TAB_MARGIN, hwnd, (HMENU)ID_LISTBOX_PAIRS_SOURCE, NULL, NULL);
		originalListboxProc = (WNDPROC)SetWindowLongPtr(lbSourceHwnd, GWLP_WNDPROC, (LONG_PTR)customListboxProc);
		SendMessage(lbSourceHwnd, WM_SETFONT, (WPARAM)hFont, 0);

		// destination listbox
		lbDestHwnd = CreateWindowEx(WS_EX_LEFT, L"ListBox", NULL,
			WS_CHILD | LBS_NOTIFY | WS_VSCROLL | WS_BORDER,
			(rc.right / 2) + 5, LISTBOX_TOP, (rc.right / 2) - TAB_MARGIN - 5, rc.bottom - LISTBOX_TOP - TAB_MARGIN, hwnd, (HMENU)ID_LISTBOX_PAIRS_DEST, NULL, NULL);
		originalListboxProc = (WNDPROC)SetWindowLongPtr(lbDestHwnd, GWLP_WNDPROC, (LONG_PTR)customListboxProc);
		SendMessage(lbDestHwnd, WM_SETFONT, (WPARAM)hFont, 0);

		bAddPair = CreateWindowEx(WS_EX_LEFT, L"Button", L"Add Pair",
			WS_CHILD | WS_TABSTOP,
			LISTBOX_LEFT, 40, BIG_BUTTON_WIDTH, BIG_BUTTON_HEIGHT, hwnd, (HMENU)ID_BUTTON_ADD_PAIR, NULL, NULL);

		// sync

			// listbox
		lbSyncHwnd = CreateWindowEx(WS_EX_LEFT, L"ListBox", NULL,
			WS_CHILD | LBS_NOTIFY | WS_VSCROLL | WS_BORDER,// | LBS_EXTENDEDSEL,
			LISTBOX_LEFT, LISTBOX_TOP, rc.right - RIGHT_MARGIN, rc.bottom - LISTBOX_TOP - PROGRESS_BAR_HEIGHT - 5, hwnd, (HMENU)ID_LISTBOX_SYNC, NULL, NULL);
		originalListboxProc = (WNDPROC)SetWindowLongPtr(lbSyncHwnd, GWLP_WNDPROC, (LONG_PTR)customListboxProc);
		SendMessage(lbSyncHwnd, WM_SETFONT, (WPARAM)hFont, 0);

		bSync = CreateWindowEx(WS_EX_LEFT, L"Button", L"Sync",
			WS_CHILD | WS_TABSTOP | WS_DISABLED,
			LISTBOX_LEFT, 40, BIG_BUTTON_WIDTH, BIG_BUTTON_HEIGHT, hwnd, (HMENU)ID_BUTTON_SYNC, NULL, NULL);

		// progress bar
		pbHwnd = CreateWindowEx(WS_EX_LEFT, PROGRESS_CLASS, NULL,
			WS_CHILD | PBS_SMOOTH,
			LISTBOX_LEFT, rc.bottom - PROGRESS_BAR_HEIGHT - TAB_MARGIN, rc.right - RIGHT_MARGIN, PROGRESS_BAR_HEIGHT, hwnd, (HMENU)ID_PROGRESS_BAR, NULL, NULL);

		SendMessage(pbHwnd, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
		SendMessage(pbHwnd, PBM_SETSTEP, (WPARAM)1, 0);

		// settings

		lSkipDesktop = CreateWindowEx(WS_EX_LEFT, L"Static", L"Skip desktop.ini files",
			WS_CHILD,
			LISTBOX_LEFT, 50, 150, 25, hwnd, (HMENU)ID_SETTINGS_DESKTOP_LABEL, NULL, NULL);

		cbSkipDesktop = CreateWindowEx(WS_EX_LEFT, L"Button", NULL,
			BS_CHECKBOX | WS_CHILD,
			240, 43, 30, 30, hwnd, (HMENU)ID_SETTINGS_DESKTOP_CHECKBOX, NULL, NULL);

		lSkipSymbolic = CreateWindowEx(WS_EX_LEFT, L"Static", L"Skip symbolic links",
			WS_CHILD,
			LISTBOX_LEFT, 80, 150, 25, hwnd, (HMENU)ID_SETTINGS_SYMBOLIC_LABEL, NULL, NULL);

		cbSkipSymbolic = CreateWindowEx(WS_EX_LEFT, L"Button", NULL,
			BS_CHECKBOX | WS_CHILD,
			240, 73, 30, 30, hwnd, (HMENU)ID_SETTINGS_SYMBOLIC_CHECKBOX, NULL, NULL);

		lSkipSmallDiff = CreateWindowEx(WS_EX_LEFT, L"Static", L"Skip timestamp differences < 3 secs",
			WS_CHILD,
			LISTBOX_LEFT, 110, 250, 25, hwnd, (HMENU)ID_SETTINGS_SMALL_DIFF_LABEL, NULL, NULL);

		cbSkipSmallDiff = CreateWindowEx(WS_EX_LEFT, L"Button", NULL,
			BS_CHECKBOX | WS_CHILD,
			240, 103, 30, 30, hwnd, (HMENU)ID_SETTINGS_SMALL_DIFF_CHECKBOX, NULL, NULL);
		break;
	case WM_NOTIFY:
	{
		switch (((LPNMHDR)lParam)->code)
		{
		case TCN_SELCHANGING:
		{
			// save any page status here before the change
			return false;
		}
		case TCN_SELCHANGE:
		{
			int page = TabCtrl_GetCurSel(tabHwnd);
			switch (page)
			{
			case 0: // projects
			{
				memset(projectName, '\0', MAX_LINE);
				memset(folderPair, '\0', FOLDER_PAIR_SIZE);
				memset(sourceFolder, '\0', MAX_LINE);
				memset(destFolder, '\0', MAX_LINE);

				SendMessage(lbSourceHwnd, LB_RESETCONTENT, 0, 0);
				SendMessage(lbDestHwnd, LB_RESETCONTENT, 0, 0);
				SendMessage(lbProjectsHwnd, LB_RESETCONTENT, 0, 0);
				SendMessage(lbSyncHwnd, LB_RESETCONTENT, 0, 0);
				SendMessage(pbHwnd, PBM_SETPOS, 0, 0);

				deleteAllPairs(pairIndex);
				fillProjectListbox(lbProjectsHwnd, &projectsHead);

				EnableWindow(bAddFolders, false);
				EnableWindow(bPreview, false);
				EnableWindow(bDelete, false);

				ShowWindow(lbProjectsHwnd, SW_SHOW);
				ShowWindow(lbSyncHwnd, SW_HIDE);
				ShowWindow(lbSourceHwnd, SW_HIDE);
				ShowWindow(lbDestHwnd, SW_HIDE);
				ShowWindow(bAddProject, SW_SHOW);
				ShowWindow(bAddFolders, SW_SHOW);
				ShowWindow(bAddPair, SW_HIDE);
				ShowWindow(bPreview, SW_SHOW);
				ShowWindow(bSync, SW_HIDE);
				ShowWindow(bDelete, SW_SHOW);
				ShowWindow(pbHwnd, SW_HIDE);
				ShowWindow(lSkipDesktop, SW_HIDE);
				ShowWindow(cbSkipDesktop, SW_HIDE);
				ShowWindow(lSkipSymbolic, SW_HIDE);
				ShowWindow(cbSkipSymbolic, SW_HIDE);
				ShowWindow(lSkipSmallDiff, SW_HIDE);
				ShowWindow(cbSkipSmallDiff, SW_HIDE);

				resizeProjectTab();
				break;
			}
			case 1: // folder pairs
			{
				EnableWindow(bDelete, false);

				ShowWindow(lbProjectsHwnd, SW_HIDE);
				ShowWindow(lbSyncHwnd, SW_HIDE);
				ShowWindow(lbSourceHwnd, SW_SHOW);
				ShowWindow(lbDestHwnd, SW_SHOW);
				ShowWindow(bAddProject, SW_HIDE);
				ShowWindow(bAddFolders, SW_HIDE);
				ShowWindow(bAddPair, SW_SHOW);
				ShowWindow(bPreview, SW_HIDE);
				ShowWindow(bSync, SW_HIDE);
				ShowWindow(bDelete, SW_SHOW);
				ShowWindow(pbHwnd, SW_HIDE);
				ShowWindow(lSkipDesktop, SW_HIDE);
				ShowWindow(cbSkipDesktop, SW_HIDE);
				ShowWindow(lSkipSymbolic, SW_HIDE);
				ShowWindow(cbSkipSymbolic, SW_HIDE);
				ShowWindow(lSkipSmallDiff, SW_HIDE);
				ShowWindow(cbSkipSmallDiff, SW_HIDE);

				// if a project name is detected load the pairs
				if (wcslen(projectName) > 0)
				{
					SendMessage(lbSourceHwnd, LB_RESETCONTENT, 0, 0);
					SendMessage(lbDestHwnd, LB_RESETCONTENT, 0, 0);
					reloadFolderPairs(lbSourceHwnd, lbDestHwnd, projectsHead, projectName);
					EnableWindow(bAddPair, true);
				}
				else
					EnableWindow(bAddPair, false);

				resizePairsTab();
				break;
			}
			case 2: // sync
			{
				if (pairCount > 0)
					EnableWindow(bSync, true);
				else
					EnableWindow(bSync, false);
				EnableWindow(bDelete, false);

				ShowWindow(lbProjectsHwnd, SW_HIDE);
				ShowWindow(lbSyncHwnd, SW_SHOW);
				ShowWindow(lbSourceHwnd, SW_HIDE);
				ShowWindow(lbDestHwnd, SW_HIDE);
				ShowWindow(bAddProject, SW_HIDE);
				ShowWindow(bAddFolders, SW_HIDE);
				ShowWindow(bAddPair, SW_HIDE);
				ShowWindow(bPreview, SW_HIDE);
				ShowWindow(bSync, SW_SHOW);
				ShowWindow(bDelete, SW_SHOW);
				ShowWindow(pbHwnd, SW_SHOW);
				ShowWindow(lSkipDesktop, SW_HIDE);
				ShowWindow(cbSkipDesktop, SW_HIDE);
				ShowWindow(lSkipSymbolic, SW_HIDE);
				ShowWindow(cbSkipSymbolic, SW_HIDE);
				ShowWindow(lSkipSmallDiff, SW_HIDE);
				ShowWindow(cbSkipSmallDiff, SW_HIDE);

				resizeSyncTab();
				break;
			}
			case 3: // settings
			{
				ShowWindow(lbProjectsHwnd, SW_HIDE);
				ShowWindow(lbSyncHwnd, SW_HIDE);
				ShowWindow(lbSourceHwnd, SW_HIDE);
				ShowWindow(lbDestHwnd, SW_HIDE);
				ShowWindow(bAddProject, SW_HIDE);
				ShowWindow(bAddFolders, SW_HIDE);
				ShowWindow(bAddPair, SW_HIDE);
				ShowWindow(bPreview, SW_HIDE);
				ShowWindow(bSync, SW_HIDE);
				ShowWindow(bDelete, SW_HIDE);
				ShowWindow(pbHwnd, SW_HIDE);
				ShowWindow(lSkipDesktop, SW_SHOW);
				ShowWindow(cbSkipDesktop, SW_SHOW);
				ShowWindow(lSkipSymbolic, SW_SHOW);
				ShowWindow(cbSkipSymbolic, SW_SHOW);
				ShowWindow(lSkipSmallDiff, SW_SHOW);
				ShowWindow(cbSkipSmallDiff, SW_SHOW);

				SendMessage(cbSkipDesktop, BM_SETCHECK, settings.skipDesktopIni, 0);
				SendMessage(cbSkipSymbolic, BM_SETCHECK, settings.skipSymbolicLinks, 0);
				SendMessage(cbSkipSmallDiff, BM_SETCHECK, settings.skipSmallDifferences, 0);
				break;
			}
			}
		}
		}
	}
	break;
	case WM_COMMAND:
		if (LOWORD(wParam) == ID_LISTBOX_PROJECTS)
		{
			// a row was selected
			if (HIWORD(wParam) == LBN_SELCHANGE)
			{
				EnableWindow(bAddFolders, false);
				EnableWindow(bPreview, false);
				EnableWindow(bDelete, false);

				// get row index
				LRESULT selectedRow = SendMessage(lbProjectsHwnd, LB_GETCURSEL, 0, 0);
				if (selectedRow != LB_ERR)
				{
					wchar_t selectedRowText[MAX_LINE] = {0};
					int textLen = (int)SendMessage(lbProjectsHwnd, LB_GETTEXT, selectedRow, (LPARAM)selectedRowText);

					if (textLen > 0)
					{
						EnableWindow(bPreview, true);
						EnableWindow(bDelete, true);

						if (isProjectName(selectedRowText, textLen))
						{
							wcscpy_s(projectName, MAX_LINE, selectedRowText);
							EnableWindow(bAddFolders, true);
						}
						else
						{
							wcscpy_s(folderPair, FOLDER_PAIR_SIZE, selectedRowText);
							findProjectName(lbProjectsHwnd, selectedRow, projectName);
						}
					}
				}
			}

			// a row was double-clicked
			if (HIWORD(wParam) == LBN_DBLCLK)
			{
				// get row index
				LRESULT selectedRow = SendMessage(lbProjectsHwnd, LB_GETCURSEL, 0, 0);
				if (selectedRow != LB_ERR)
				{
					wchar_t selectedRowText[MAX_LINE] = {0};
					int textLen = (int)SendMessage(lbProjectsHwnd, LB_GETTEXT, selectedRow, (LPARAM)selectedRowText);

					if (textLen > 0)
					{
						if (isProjectName(selectedRowText, textLen))
						{
							wcscpy_s(projectName, MAX_LINE, selectedRowText);
							getProjectName();
						}
						else
						{
							wcscpy_s(folderPair, FOLDER_PAIR_SIZE, selectedRowText);
							findProjectName(lbProjectsHwnd, selectedRow, projectName);
							SendMessage(tabHwnd, TCM_SETCURFOCUS, TAB_PAIRS, 0);
							editingFolderPair = true;
							addFolderPair();
						}
					}
				}
			}
		}

		if (LOWORD(wParam) == ID_LISTBOX_PAIRS_SOURCE)
		{
			// a row was selected
			if (HIWORD(wParam) == LBN_SELCHANGE)
			{
				EnableWindow(bDelete, false);

				// get row index
				LRESULT selectedRow = SendMessage(lbSourceHwnd, LB_GETCURSEL, 0, 0);
				if (selectedRow != LB_ERR)
				{
					SendMessage(lbDestHwnd, LB_SETCURSEL, selectedRow, 0);
					wchar_t selectedRowText[MAX_LINE] = {0};
					int textLen = (int)SendMessage(lbSourceHwnd, LB_GETTEXT, selectedRow, (LPARAM)selectedRowText);

					if (textLen > 0)
						EnableWindow(bDelete, true);
				}
			}

			// a row was double-clicked
			if (HIWORD(wParam) == LBN_DBLCLK)
			{
				// get row index
				LRESULT selectedRow = SendMessage(lbSourceHwnd, LB_GETCURSEL, 0, 0);
				if (selectedRow != LB_ERR)
				{
					wchar_t selectedRowText[MAX_LINE] = {0};
					int textLen = (int)SendMessage(lbSourceHwnd, LB_GETTEXT, selectedRow, (LPARAM)selectedRowText);

					if (textLen > 0)
					{
						// edit folder pair
						SendMessage(lbDestHwnd, LB_GETTEXT, selectedRow, (LPARAM)destFolder);
						wcscpy_s(folderPair, FOLDER_PAIR_SIZE, selectedRowText);
						wcscat(folderPair, L" -> ");
						wcscat(folderPair, destFolder);
						editingFolderPair = true;
						addFolderPair();
					}
				}
			}
		}

		if (LOWORD(wParam) == ID_LISTBOX_PAIRS_DEST)
		{
			// a row was selected
			if (HIWORD(wParam) == LBN_SELCHANGE)
			{
				EnableWindow(bDelete, false);

				// get row index
				LRESULT selectedRow = SendMessage(lbDestHwnd, LB_GETCURSEL, 0, 0);
				if (selectedRow != LB_ERR)
				{
					SendMessage(lbSourceHwnd, LB_SETCURSEL, selectedRow, 0);
					SendMessage(lbSourceHwnd, LB_SETCARETINDEX, selectedRow, 0);
					wchar_t selectedRowText[MAX_LINE] = {0};
					int textLen = (int)SendMessage(lbDestHwnd, LB_GETTEXT, selectedRow, (LPARAM)selectedRowText);

					if (textLen > 0)
						EnableWindow(bDelete, true);
				}
			}

			// a row was double-clicked
			if (HIWORD(wParam) == LBN_DBLCLK)
			{
				// get row index
				LRESULT selectedRow = SendMessage(lbDestHwnd, LB_GETCURSEL, 0, 0);
				if (selectedRow != LB_ERR)
				{
					wchar_t selectedRowText[MAX_LINE] = {0};
					int textLen = (int)SendMessage(lbDestHwnd, LB_GETTEXT, selectedRow, (LPARAM)selectedRowText);

					if (textLen > 0)
					{
						// edit folder pair
						SendMessage(lbSourceHwnd, LB_GETTEXT, selectedRow, (LPARAM)sourceFolder);
						wcscpy_s(folderPair, FOLDER_PAIR_SIZE, sourceFolder);
						wcscat(folderPair, L" -> ");
						wcscat(folderPair, selectedRowText);
						editingFolderPair = true;
						addFolderPair();
					}
				}
			}
		}

		if (LOWORD(wParam) == ID_LISTBOX_SYNC)
		{
			// a row was selected
			if (!recentlySynced && HIWORD(wParam) == LBN_SELCHANGE)
			{
				// get row index
				LRESULT selectedRow = SendMessage(lbSyncHwnd, LB_GETCURSEL, 0, 0);
				if (selectedRow != LB_ERR)
				{
					wchar_t selectedRowText[MAX_LINE] = {0};
					int textLen = (int)SendMessage(lbSyncHwnd, LB_GETTEXT, selectedRow, (LPARAM)selectedRowText);

					if (textLen > 0)
						EnableWindow(bDelete, true);
				}
			}

			// a row was double-clicked
			//if (HIWORD(wParam) == LBN_DBLCLK)
			//{
			//	// get row index
			//	LRESULT selectedRow = SendMessage(lbSyncHwnd, LB_GETCURSEL, 0, 0);
			//	if (selectedRow != LB_ERR)
			//	{
			//		wchar_t selectedRowText[MAX_LINE] = { 0 };
			//		int textLen = (int)SendMessage(lbSyncHwnd, LB_GETTEXT, selectedRow, (LPARAM)selectedRowText);

			//		if (textLen > 0)
			//		{
			//		}
			//	}
			//}
		}

		if (LOWORD(wParam) == ID_BUTTON_ADD_PROJECT)
		{
			memset(projectName, '\0', MAX_LINE);
			getProjectName();
		}

		if (LOWORD(wParam) == ID_BUTTON_ADD_FOLDER_PAIR)
		{
#if DEBUG_MODE
			logger(L"Add folder pair button");
#endif
			// get row index
			LRESULT selectedRow = SendMessage(lbProjectsHwnd, LB_GETCURSEL, 0, 0);
			if (selectedRow != LB_ERR)
			{
				wchar_t selectedRowText[MAX_LINE] = {0};
				int textLen = (int)SendMessage(lbProjectsHwnd, LB_GETTEXT, selectedRow, (LPARAM)selectedRowText);

				if (textLen > 0 && isProjectName(selectedRowText, textLen))
				{
					wcscpy_s(projectName, MAX_LINE, selectedRowText);
#if DEBUG_MODE
					wchar_t buf[100] = {0};
					swprintf(buf, 100, L"Selected project name: %s", projectName);
					logger(buf);
#endif
					// change to folder pair tab and populate listboxes
					wcscpy_s(folderPair, FOLDER_PAIR_SIZE, L"C: -> C:");
					SendMessage(tabHwnd, TCM_SETCURFOCUS, TAB_PAIRS, 0);
					editingFolderPair = false;
					addFolderPair();
				}
			}
		}

		if (LOWORD(wParam) == ID_BUTTON_ADD_PAIR)
		{
#if DEBUG_MODE
			logger(L"Add pair button");
#endif
			wcscpy_s(folderPair, FOLDER_PAIR_SIZE, L"C: -> C:");
			editingFolderPair = false;
			addFolderPair();
		}

		if (LOWORD(wParam) == ID_BUTTON_DELETE)
		{
			LRESULT tab = SendMessage(tabHwnd, TCM_GETCURFOCUS, 0, 0);

			if (tab == TAB_PROJECTS)
			{
#if DEBUG_MODE
				logger(L"Delete button, projects");
#endif
				EnableWindow(bDelete, false);
				EnableWindow(bPreview, false);

				// get row index
				LRESULT selectedRow = SendMessage(lbProjectsHwnd, LB_GETCURSEL, 0, 0);
				if (selectedRow != LB_ERR)
				{
					wchar_t selectedRowText[MAX_LINE] = {0};
					int textLen = (int)SendMessage(lbProjectsHwnd, LB_GETTEXT, selectedRow, (LPARAM)selectedRowText);

					if (textLen > 0)
					{
						if (isProjectName(selectedRowText, textLen))
						{
							// delete whole project
							deleteProject(&projectsHead, selectedRowText);
						}
						else
						{
							// delete folder pair
							wcscpy_s(folderPair, FOLDER_PAIR_SIZE, selectedRowText);
							findProjectName(lbProjectsHwnd, selectedRow, projectName);
							deleteFolderPair(&projectsHead, folderPair, projectName);
						}

						SendMessage(lbProjectsHwnd, LB_RESETCONTENT, 0, 0);
						fillProjectListbox(lbProjectsHwnd, &projectsHead);
						SetFocus(lbProjectsHwnd);
					}
				}
			}

			if (tab == TAB_PAIRS)
			{
#if DEBUG_MODE
				logger(L"Delete button, pairs");
#endif
				EnableWindow(bDelete, false);

				// get row index
				LRESULT selectedRow = SendMessage(lbSourceHwnd, LB_GETCURSEL, 0, 0);
				if (selectedRow != LB_ERR)
				{
					wchar_t selectedRowText[MAX_LINE] = {0};
					int textLen = (int)SendMessage(lbSourceHwnd, LB_GETTEXT, selectedRow, (LPARAM)selectedRowText);

					if (textLen > 0)
					{
						// when selecting Project name there is no folder pair selected so create it from selected rows
						if (wcslen(folderPair) == 0)
						{
							SendMessage(lbDestHwnd, LB_GETTEXT, selectedRow, (LPARAM)destFolder);
							wcscpy_s(folderPair, FOLDER_PAIR_SIZE, selectedRowText);
							wcscat(folderPair, L" -> ");
							wcscat(folderPair, destFolder);
						}

						// delete folder pair and reload listboxes
						deleteFolderPair(&projectsHead, folderPair, projectName);
						SendMessage(lbSourceHwnd, LB_RESETCONTENT, 0, 0);
						SendMessage(lbDestHwnd, LB_RESETCONTENT, 0, 0);
						reloadFolderPairs(lbSourceHwnd, lbDestHwnd, projectsHead, projectName);
						memset(folderPair, '\0', FOLDER_PAIR_SIZE);
					}
				}
			}

			if (tab == TAB_SYNC)
			{
#if DEBUG_MODE
				logger(L"Delete button, sync");
#endif
				EnableWindow(bDelete, false);

				// get row index
				LRESULT selectedRow = SendMessage(lbSyncHwnd, LB_GETCURSEL, 0, 0);
				if (selectedRow != LB_ERR)
				{
					wchar_t selectedRowText[MAX_LINE] = {0};
					int textLen = (int)SendMessage(lbSyncHwnd, LB_GETTEXT, selectedRow, (LPARAM)selectedRowText);

					if (textLen > 0)
					{
						// delete folder pair from list and listbox
						deleteFilePair(pairIndex, selectedRowText);
						SendMessage(lbSyncHwnd, LB_RESETCONTENT, 0, 0);
						fillSyncListbox(lbSyncHwnd, pairIndex);

						if (pairCount > 0)
							EnableWindow(bSync, true);
						else
							EnableWindow(bSync, false);
					}
				}
			}
		}

		if (LOWORD(wParam) == ID_BUTTON_PREVIEW)
		{
#if DEBUG_MODE
			logger(L"Preview button");
#endif
			recentlySynced = false;
			deleteAllPairs(pairIndex);

			// get row index
			LRESULT selectedRow = SendMessage(lbProjectsHwnd, LB_GETCURSEL, 0, 0);
			if (selectedRow != LB_ERR)
			{
				wchar_t selectedRowText[MAX_LINE] = {0};
				int textLen = (int)SendMessage(lbProjectsHwnd, LB_GETTEXT, selectedRow, (LPARAM)selectedRowText);

				if (textLen > 0)
				{
					SendMessage(tabHwnd, TCM_SETCURFOCUS, TAB_SYNC, 0);
					EnableWindow(tabHwnd, false);
					startPreviewScanThread(pbHwnd, lbSyncHwnd, lbProjectsHwnd, bSync, tabHwnd, &projectsHead, pairIndex, selectedRowText, selectedRow);
				}
			}
		}

		if (LOWORD(wParam) == ID_BUTTON_SYNC)
		{
#if DEBUG_MODE
			logger(L"Sync button");
#endif
			EnableWindow(bSync, false);
			EnableWindow(bDelete, false);
			EnableWindow(tabHwnd, false);
			synchronizeFiles(pbHwnd, lbSyncHwnd, bSync, tabHwnd, pairIndex);
			recentlySynced = true;
		}

		if (LOWORD(wParam) == ID_SETTINGS_DESKTOP_CHECKBOX)
		{
#if DEBUG_MODE
			logger(L"Desktop checkbox");
#endif
			SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)!SendMessage((HWND)lParam, BM_GETCHECK, 0, 0), 0);
			settings.skipDesktopIni = !settings.skipDesktopIni;
		}

		if (LOWORD(wParam) == ID_SETTINGS_SYMBOLIC_CHECKBOX)
		{
#if DEBUG_MODE
			logger(L"Document checkbox");
#endif
			SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)!SendMessage((HWND)lParam, BM_GETCHECK, 0, 0), 0);
			settings.skipSymbolicLinks = !settings.skipSymbolicLinks;
		}

		if (LOWORD(wParam) == ID_SETTINGS_SMALL_DIFF_CHECKBOX)
		{
#if DEBUG_MODE
			logger(L"Small diff checkbox");
#endif
			SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)!SendMessage((HWND)lParam, BM_GETCHECK, 0, 0), 0);
			settings.skipSmallDifferences = !settings.skipSmallDifferences;
		}

		break;
	case WM_SIZE:
	{
		// resize listboxes to fit window
		int tab = TabCtrl_GetCurSel(tabHwnd);
		switch (tab)
		{
		case 0:
			resizeProjectTab();
			break;
		case 1:
			resizePairsTab();
			break;
		case 2:
			resizeSyncTab();
			break;
		case 3:
			resizeSettingsTab();
			break;
		}
	}
	break;
	case WM_KEYUP:
		switch (wParam)
		{
		case VK_ESCAPE:
			shutDown(mainHwnd, &projectsHead);
			break;
		}
		break;
	case WM_DESTROY:
		shutDown(mainHwnd, &projectsHead);
		break;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK customListboxProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		//case WM_LBUTTONDBLCLK:
		//{
		//	listboxDoubleClicked = true;
		//	logger(L"Listbox double clicked");
		//}
		//	break;
	case WM_KEYUP:
		switch (wParam)
		{
		case VK_DELETE:
#if DEBUG_MODE
			logger(L"Del key");
#endif
			SendMessage(bDelete, BM_CLICK, 0, 0);
			break;
		case VK_ESCAPE:
			shutDown(mainHwnd, &projectsHead);
			break;
		}
		break;
	}
	return CallWindowProc(originalListboxProc, hwnd, msg, wParam, lParam);
}

static void getProjectName(void)
{
	static WNDCLASSEX wcProjectName = {0};
	static bool projectNameClassRegistered = false;

	if (showingProjectName)
		return;
	showingProjectName = true;

	if (!projectNameClassRegistered)
	{
		wcProjectName.cbSize = sizeof(WNDCLASSEX);
		wcProjectName.cbClsExtra = 0;
		wcProjectName.cbWndExtra = 0;
		wcProjectName.hbrBackground = (HBRUSH)COLOR_WINDOW;
		wcProjectName.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcProjectName.hIcon = NULL;
		wcProjectName.hIconSm = NULL;
		wcProjectName.hInstance = instance;
		wcProjectName.lpfnWndProc = projectNameWndProc;
		wcProjectName.lpszClassName = L"getProjectName";
		wcProjectName.lpszMenuName = NULL;
		wcProjectName.style = CS_HREDRAW | CS_VREDRAW;

		if (!RegisterClassEx(&wcProjectName))
		{
			MessageBox(NULL, L"Get Project Name window registration failed", L"Error", MB_ICONEXCLAMATION | MB_OK);
			return;
		}
		else
			projectNameClassRegistered = true;
	}

	projectNameHwnd = CreateWindowEx(WS_EX_LEFT,
		wcProjectName.lpszClassName,
		L"Get Project Name",
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT,
		435, 130,
		NULL, NULL,
		instance, NULL);

	if (!projectNameHwnd)
	{
		MessageBox(NULL, L"Get Project Name window creation failed", L"Error", MB_ICONEXCLAMATION | MB_OK);
		return;
	}

	ShowWindow(projectNameHwnd, SW_SHOW);
}

static LRESULT CALLBACK projectNameWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND bProjectNameCancel, lProjectName, eProjectName;

	switch (msg)
	{
	case WM_CREATE:
		SetTimer(hwnd, ID_TIMER1, 100, NULL);

		lProjectName = CreateWindowEx(WS_EX_LEFT, L"Static", L"Enter project name",
			WS_VISIBLE | WS_CHILD,
			10, 15, 150, BUTTON_HEIGHT, hwnd, (HMENU)ID_LABEL_ADD_PROJECT_NAME, NULL, NULL);

		eProjectName = CreateWindowEx(WS_EX_LEFT, L"Edit", NULL,
			WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_BORDER,
			140, 10, 170, BUTTON_HEIGHT, hwnd, (HMENU)ID_EDIT_ADD_PROJECT_NAME, NULL, NULL);
		originalProjectNameProc = (WNDPROC)SetWindowLongPtr(eProjectName, GWLP_WNDPROC, (LONG_PTR)customProjectNameProc);

		bProjectNameOK = CreateWindowEx(WS_EX_LEFT, L"Button", L"OK",
			WS_VISIBLE | WS_CHILD | WS_TABSTOP,
			320, 10, BUTTON_WIDTH, BUTTON_HEIGHT, hwnd, (HMENU)ID_BUTTON_OK, NULL, NULL);

		bProjectNameCancel = CreateWindowEx(WS_EX_LEFT, L"Button", L"Cancel",
			WS_VISIBLE | WS_CHILD | WS_TABSTOP,
			320, 45, BUTTON_WIDTH, BUTTON_HEIGHT, hwnd, (HMENU)ID_BUTTON_CANCEL, NULL, NULL);

		SendMessage(eProjectName, EM_LIMITTEXT, MAX_LINE, 0);
		if (wcslen(projectName) > 0)
			SetWindowText(eProjectName, projectName);
		centerWindow(hwnd);
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == ID_BUTTON_OK)
		{
			wchar_t newProjectName[MAX_LINE];
			GetWindowText(eProjectName, newProjectName, MAX_LINE);

			// blank project name entered
			if (wcslen(newProjectName) == 0)
			{
				logger(L"Blank name used");
				DestroyWindow(hwnd);
				break;
			}

#if DEBUG_MODE
			wchar_t buf[100] = {0};
			swprintf(buf, 100, L"New project name: %s", newProjectName);
			logger(buf);
#endif
			// new project being added
			if (wcslen(newProjectName) > 0 && wcslen(projectName) == 0)
			{
				// check if pre-existing name used
				bool existing = false;
				struct ProjectNode *node = projectsHead;
				while (node != NULL)
				{
					if (wcscmp(node->project.name, newProjectName) == 0)
					{
						wchar_t buff[MAX_LINE] = L"Project name already exists";
						logger(buff);
						MessageBox(NULL, buff, L"Error", MB_ICONEXCLAMATION | MB_OK);
						existing = true;
						break;
					}
					node = node->next;
				}

				if (!existing)
				{
#if DEBUG_MODE
					logger(L"New project added");
#endif
					appendProjectNode(&projectsHead, newProjectName, L"C:", L"C:");
					memset(projectName, '\0', MAX_LINE);
				}
			}

			// existing project being renamed
			if (wcslen(newProjectName) > 0 && wcslen(projectName) > 0)
				renameProject(&projectsHead, projectName, newProjectName);

			SendMessage(lbProjectsHwnd, LB_RESETCONTENT, 0, 0);
			sortProjectNodes(&projectsHead);
			fillProjectListbox(lbProjectsHwnd, &projectsHead);
			memset(projectName, '\0', MAX_LINE);
			showingProjectName = false;
			DestroyWindow(hwnd);
		}

		if (LOWORD(wParam) == ID_BUTTON_CANCEL)
		{
			showingProjectName = false;
			DestroyWindow(hwnd);
		}
		break;
	case WM_TIMER:
		if (wParam == ID_TIMER1)
		{
			SetFocus(eProjectName);
			KillTimer(hwnd, ID_TIMER1);
		}
		break;
	case WM_DESTROY:
		showingProjectName = false;
		DestroyWindow(hwnd);
		break;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK customProjectNameProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_KEYUP:
		switch (wParam)
		{
		case VK_ESCAPE:
			DestroyWindow(projectNameHwnd);
			break;
		case VK_RETURN:
			SendMessage(bProjectNameOK, BM_CLICK, 0, 0);
			break;
		case 'A': // CTRL A
			if (GetAsyncKeyState(VK_CONTROL))
				SendMessage(hwnd, EM_SETSEL, 0, -1);
			break;
		}
		break;
	}
	return CallWindowProc(originalProjectNameProc, hwnd, msg, wParam, lParam);
}

static void addFolderPair(void)
{
	static WNDCLASSEX wcFolderPair = {0};
	static bool folderPairClassRegistered = false;

	if (showingFolderPair)
		return;
	showingFolderPair = true;

	if (!folderPairClassRegistered)
	{
		wcFolderPair.cbSize = sizeof(WNDCLASSEX);
		wcFolderPair.cbClsExtra = 0;
		wcFolderPair.cbWndExtra = 0;
		wcFolderPair.hbrBackground = (HBRUSH)COLOR_WINDOW;
		wcFolderPair.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcFolderPair.hIcon = NULL;
		wcFolderPair.hIconSm = NULL;
		wcFolderPair.hInstance = instance;
		wcFolderPair.lpfnWndProc = folderPairWndProc;
		wcFolderPair.lpszClassName = L"addFolderPair";
		wcFolderPair.lpszMenuName = NULL;
		wcFolderPair.style = CS_HREDRAW | CS_VREDRAW;

		if (!RegisterClassEx(&wcFolderPair))
		{
			MessageBox(NULL, L"Add Folder Pair window registration failed", L"Error", MB_ICONEXCLAMATION | MB_OK);
			return;
		}
		else
			folderPairClassRegistered = true;
	}

	folderPairHwnd = CreateWindowEx(WS_EX_LEFT,
		wcFolderPair.lpszClassName,
		L"Add Folder Pair",
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT,
		800, 600,
		NULL, NULL,
		instance, NULL);

	if (!folderPairHwnd)
	{
		MessageBox(NULL, L"Add Folder Pair window creation failed", L"Error", MB_ICONEXCLAMATION | MB_OK);
		return;
	}

	ShowWindow(folderPairHwnd, SW_SHOW);
}

static LRESULT CALLBACK folderPairWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND bFolderPairAdd, bFolderPairCancel, lSource, lDestination, eSource, eDestination;
	static wchar_t previousFolderPair[FOLDER_PAIR_SIZE];

	switch (msg)
	{
	case WM_CREATE:
		lSource = CreateWindowEx(WS_EX_LEFT, L"Static", L"Source",
			WS_VISIBLE | WS_CHILD,
			LISTBOX_LEFT, 10, BUTTON_WIDTH, BUTTON_HEIGHT, hwnd, (HMENU)ID_LABEL_PAIR_SOURCE, NULL, NULL);

		eSource = CreateWindowEx(WS_EX_LEFT, L"Edit", NULL,
			WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL,
			LISTBOX_LEFT, 30, 370, 25, hwnd, (HMENU)ID_EDIT_PAIR_SOURCE, NULL, NULL);
		originalSourceEditboxProc = (WNDPROC)SetWindowLongPtr(eSource, GWLP_WNDPROC, (LONG_PTR)customSourceEditboxProc);

		lbPairSourceHwnd = CreateWindowEx(WS_EX_LEFT, L"ListBox", NULL,
			WS_VISIBLE | WS_CHILD | LBS_NOTIFY | WS_VSCROLL | WS_BORDER | LBS_EXTENDEDSEL,
			LISTBOX_LEFT, 60, 370, 450, hwnd, (HMENU)ID_LISTBOX_ADD_SOURCE, NULL, NULL);
		//			originalSourceListboxProc = (WNDPROC)SetWindowLongPtr(lbPairSourceHwnd, GWLP_WNDPROC, (LONG_PTR)customSourceListboxProc);
		originalListboxProc = (WNDPROC)SetWindowLongPtr(lbPairSourceHwnd, GWLP_WNDPROC, (LONG_PTR)customListboxProc);

		lDestination = CreateWindowEx(WS_EX_LEFT, L"Static", L"Destination",
			WS_VISIBLE | WS_CHILD,
			400, 10, BUTTON_WIDTH, BUTTON_HEIGHT, hwnd, (HMENU)ID_LABEL_PAIR_DEST, NULL, NULL);

		eDestination = CreateWindowEx(WS_EX_LEFT, L"Edit", NULL,
			WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL,
			400, 30, 370, 25, hwnd, (HMENU)ID_EDIT_PAIR_DEST, NULL, NULL);
		originalDestinationEditboxProc = (WNDPROC)SetWindowLongPtr(eDestination, GWLP_WNDPROC, (LONG_PTR)customDestinationEditboxProc);

		lbPairDestHwnd = CreateWindowEx(WS_EX_LEFT, L"ListBox", NULL,
			WS_VISIBLE | WS_CHILD | LBS_NOTIFY | WS_VSCROLL | WS_BORDER | LBS_EXTENDEDSEL,
			400, 60, 370, 450, hwnd, (HMENU)ID_LISTBOX_ADD_DEST, NULL, NULL);
		//			originalDestinationListboxProc = (WNDPROC)SetWindowLongPtr(lbPairDestHwnd, GWLP_WNDPROC, (LONG_PTR)customDestinationListboxProc);
		originalListboxProc = (WNDPROC)SetWindowLongPtr(lbPairDestHwnd, GWLP_WNDPROC, (LONG_PTR)customListboxProc);

		bFolderPairAdd = CreateWindowEx(WS_EX_LEFT, L"Button", L"Add",
			WS_VISIBLE | WS_CHILD | WS_TABSTOP,
			290, 520, BUTTON_WIDTH, BUTTON_HEIGHT, hwnd, (HMENU)ID_BUTTON_PAIR_ADD, NULL, NULL);

		bFolderPairCancel = CreateWindowEx(WS_EX_LEFT, L"Button", L"Cancel",
			WS_VISIBLE | WS_CHILD | WS_TABSTOP,
			410, 520, BUTTON_WIDTH, BUTTON_HEIGHT, hwnd, (HMENU)ID_BUTTON_CANCEL, NULL, NULL);

		SendMessage(eSource, EM_LIMITTEXT, MAX_LINE, 0);
		SendMessage(eDestination, EM_LIMITTEXT, MAX_LINE, 0);
		centerWindow(hwnd);

		// store folderPair in case pair is being edited
		memset(previousFolderPair, '\0', FOLDER_PAIR_SIZE);
		wcscpy_s(previousFolderPair, FOLDER_PAIR_SIZE, folderPair);

		// load current folder pair into listboxes
		size_t length = wcslen(folderPair);
		assert(length > 0);

		if (length > 0)
		{
			splitPair(folderPair, sourceFolder, destFolder, length);
			SetWindowText(eSource, sourceFolder);
			SetWindowText(eDestination, destFolder);
			listSubFolders(lbPairSourceHwnd, sourceFolder);
			listSubFolders(lbPairDestHwnd, destFolder);
		}
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == ID_LISTBOX_ADD_SOURCE)
		{
			// a row was selected
			// do nothing when a row is only selected?
			//if (HIWORD(wParam) == LBN_SELCHANGE)
			//{
			//	// get row index
			//	LRESULT selectedRow = SendMessage(lbPairSourceHwnd, LB_GETCURSEL, 0, 0);
			//	if (selectedRow != LB_ERR)
			//	{
			//		wchar_t selectedRowText[MAX_LINE] = { 0 };
			//		int textLen = (int)SendMessage(lbPairSourceHwnd, LB_GETTEXT, selectedRow, (LPARAM)selectedRowText);

			//		if (textLen > 0)
			//		{
			//
			//		}
			//	}
			//}

			// a row was double-clicked
			if (HIWORD(wParam) == LBN_DBLCLK)
			{
				// get row index
				LRESULT selectedRow = SendMessage(lbPairSourceHwnd, LB_GETCURSEL, 0, 0);
				if (selectedRow != LB_ERR)
				{
					// change to selected folder and load contents
					wchar_t selectedRowText[MAX_LINE] = {0};
					int textLen = (int)SendMessage(lbPairSourceHwnd, LB_GETTEXT, selectedRow, (LPARAM)selectedRowText);

					if (textLen > 0)
					{
						wchar_t currentFolder[MAX_LINE] = {0};
						wcscpy_s(currentFolder, MAX_LINE, sourceFolder);

						// handle '..' or get new folder name
						if (wcscmp(selectedRowText, L"..") == 0)
							cutOffLastFolder(sourceFolder);
						else
						{
							GetWindowText(eSource, sourceFolder, MAX_LINE);

							// handle drive change
							if (selectedRowText[1] == ':' && selectedRowText[2] == '\\')
							{
								wcscpy_s(sourceFolder, MAX_LINE, selectedRowText);
								sourceFolder[2] = '\0'; // prevent double slash
							}
							else
							{
								if (wcslen(sourceFolder) + wcslen(selectedRowText) + 2 > MAX_LINE)
									MessageBox(NULL, L"Path is at the limit, can't add new folder", L"Error", MB_ICONEXCLAMATION | MB_OK);
								else
									addPath(sourceFolder, sourceFolder, selectedRowText);
							}
						}

						SendMessage(lbPairSourceHwnd, LB_RESETCONTENT, 0, 0);
						if (listSubFolders(lbPairSourceHwnd, sourceFolder))
							SetWindowText(eSource, sourceFolder);
						else
						{
							//TODO handle errors properly
							//TODO what happens if folder is read-only? hidden? no access?
							// if error, redisplay folder contents
							SendMessage(lbPairSourceHwnd, LB_RESETCONTENT, 0, 0);
							listSubFolders(lbPairSourceHwnd, currentFolder);
						}
					}
				}
			}
		}

		if (LOWORD(wParam) == ID_LISTBOX_ADD_DEST)
		{
			// a row was selected
			// do nothing when a row is only selected?
			//if (HIWORD(wParam) == LBN_SELCHANGE)
			//{
			//	// get row index
			//	LRESULT selectedRow = SendMessage(lbPairDestHwnd, LB_GETCURSEL, 0, 0);
			//	if (selectedRow != LB_ERR)
			//	{
			//		wchar_t selectedRowText[MAX_LINE] = { 0 };
			//		int textLen = (int)SendMessage(lbPairDestHwnd, LB_GETTEXT, selectedRow, (LPARAM)selectedRowText);

			//		if (textLen > 0)
			//		{
			//
			//		}
			//	}
			//}

			// a row was double-clicked
			if (HIWORD(wParam) == LBN_DBLCLK)
			{
				// get row index
				LRESULT selectedRow = SendMessage(lbPairDestHwnd, LB_GETCURSEL, 0, 0);
				if (selectedRow != LB_ERR)
				{
					// change to selected folder and load contents
					wchar_t selectedRowText[MAX_LINE] = {0};
					int textLen = (int)SendMessage(lbPairDestHwnd, LB_GETTEXT, selectedRow, (LPARAM)selectedRowText);

					if (textLen > 0)
					{
						wchar_t currentFolder[MAX_LINE] = {0};
						wcscpy_s(currentFolder, MAX_LINE, destFolder);

						// handle .. or get new folder name
						if (wcscmp(selectedRowText, L"..") == 0)
							cutOffLastFolder(destFolder);
						else
						{
							GetWindowText(eDestination, destFolder, MAX_LINE);

							// handle drive change
							if (selectedRowText[1] == ':' && selectedRowText[2] == '\\')
							{
								wcscpy_s(destFolder, MAX_LINE, selectedRowText);
								destFolder[2] = '\0'; // prevent double slash
							}
							else
							{
								if (wcslen(destFolder) + wcslen(selectedRowText) + 2 > MAX_LINE)
									MessageBox(NULL, L"Path would be over the limit, can't add new folder", L"Error", MB_ICONEXCLAMATION | MB_OK);
								else
									addPath(destFolder, destFolder, selectedRowText);
							}
						}

						SendMessage(lbPairDestHwnd, LB_RESETCONTENT, 0, 0);
						if (listSubFolders(lbPairDestHwnd, destFolder))
							SetWindowText(eDestination, destFolder);
						else
						{
							//TODO handle errors properly
							//TODO what happens if folder is read-only? hidden? no access?
							// if error, redisplay folder contents
							SendMessage(lbPairDestHwnd, LB_RESETCONTENT, 0, 0);
							listSubFolders(lbPairDestHwnd, currentFolder);
						}
					}
				}
			}
		}

		if (LOWORD(wParam) == ID_BUTTON_PAIR_ADD)
		{
			GetWindowText(eSource, sourceFolder, MAX_LINE);
			GetWindowText(eDestination, destFolder, MAX_LINE);

			if (wcslen(sourceFolder) == 0 || wcslen(destFolder) == 0)
				break;

			if (editingFolderPair)
			{
				replaceFolderPair(&projectsHead, previousFolderPair, projectName, sourceFolder, destFolder);
				editingFolderPair = false;
			}
			else
				appendProjectNode(&projectsHead, projectName, sourceFolder, destFolder);

			// reload pairs listboxes
			SendMessage(lbSourceHwnd, LB_RESETCONTENT, 0, 0);
			SendMessage(lbDestHwnd, LB_RESETCONTENT, 0, 0);
			sortProjectNodes(&projectsHead);
			reloadFolderPairs(lbSourceHwnd, lbDestHwnd, projectsHead, projectName);
			showingFolderPair = false;
			DestroyWindow(hwnd);
		}

		if (LOWORD(wParam) == ID_BUTTON_CANCEL)
		{
			showingFolderPair = false;
			DestroyWindow(hwnd);
		}
		break;
	case WM_DESTROY:
		showingFolderPair = false;
		DestroyWindow(hwnd);
		break;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK customSourceEditboxProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_KEYUP:
		switch (wParam)
		{
		case VK_ESCAPE:
			DestroyWindow(folderPairHwnd);
			break;
		case VK_RETURN:
			// load folder contents
			GetWindowText(hwnd, sourceFolder, MAX_LINE);
			if (destFolder[1] == ':')
				destFolder[0] = toupper(destFolder[0]);
			SendMessage(lbPairSourceHwnd, LB_RESETCONTENT, 0, 0);
			if (listSubFolders(lbPairSourceHwnd, sourceFolder))
				SetWindowText(hwnd, sourceFolder);
			break;
		case 'A': // CTRL A
			if (GetAsyncKeyState(VK_CONTROL))
				SendMessage(hwnd, EM_SETSEL, 0, -1);
			break;
		}
		break;
	}
	return CallWindowProc(originalSourceEditboxProc, hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK customDestinationEditboxProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_KEYUP:
		switch (wParam)
		{
		case VK_ESCAPE:
			DestroyWindow(folderPairHwnd);
			break;
		case VK_RETURN:
			// load folder contents
			GetWindowText(hwnd, destFolder, MAX_LINE);
			if (destFolder[1] == ':')
				destFolder[0] = toupper(destFolder[0]);
			SendMessage(lbPairDestHwnd, LB_RESETCONTENT, 0, 0);
			if (listSubFolders(lbPairDestHwnd, destFolder))
				SetWindowText(hwnd, destFolder);
			break;
		case 'A': // CTRL A
			if (GetAsyncKeyState(VK_CONTROL))
				SendMessage(hwnd, EM_SETSEL, 0, -1);
			break;
		}
		break;
	}
	return CallWindowProc(originalDestinationEditboxProc, hwnd, msg, wParam, lParam);
}

//static LRESULT CALLBACK customSourceListboxProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
//{
//	switch (msg)
//	{
//		//case WM_LBUTTONDBLCLK:
//		//{
//		//	logger(L"Listbox double clicked");
//		//}
//		//	break;
//		case WM_KEYUP:
//			switch (wParam)
//			{
//				case VK_ESCAPE:
//					DestroyWindow(folderPairHwnd);
//					break;
//			}
//			break;
//	}
//	return CallWindowProc(originalSourceListboxProc, hwnd, msg, wParam, lParam);
//}
//
//static LRESULT CALLBACK customDestinationListboxProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
//{
//	switch (msg)
//	{
//		//case WM_LBUTTONDBLCLK:
//		//{
//		//	logger(L"Listbox double clicked");
//		//}
//		//	break;
//		case WM_KEYUP:
//			switch (wParam)
//			{
//				case VK_ESCAPE:
//					DestroyWindow(folderPairHwnd);
//					break;
//			}
//			break;
//	}
//	return CallWindowProc(originalDestinationListboxProc, hwnd, msg, wParam, lParam);
//}

static void forceMinimumSize(int windowWidth, int windowHeight, int left, int top)
{
	// force minimum height
	if (windowHeight < WINDOW_HEIGHT_MINIMUM)
		SetWindowPos(mainHwnd, HWND_TOP, left, top, windowWidth, WINDOW_HEIGHT_MINIMUM, SWP_SHOWWINDOW);

	// force minimum width
	if (windowWidth < WINDOW_WIDTH_MINIMUM)
		SetWindowPos(mainHwnd, HWND_TOP, left, top, WINDOW_WIDTH_MINIMUM, windowHeight, SWP_SHOWWINDOW);
}

static void resizeTab(int windowWidth, int windowHeight)
{
	SetWindowPos(tabHwnd, HWND_TOP, TAB_TOP, TAB_LEFT, windowWidth - 25, windowHeight - 50, SWP_SHOWWINDOW);
}

static void resizeProjectTab()
{
	RECT rc = {0};
	GetWindowRect(mainHwnd, &rc);
	int windowHeight = rc.bottom - rc.top;
	int windowWidth = rc.right - rc.left;

	resizeTab(windowWidth, windowHeight);
	SetWindowPos(lbProjectsHwnd, HWND_TOP, LISTBOX_LEFT, LISTBOX_TOP, windowWidth - RIGHT_MARGIN - 15, windowHeight - LISTBOX_TOP - TAB_MARGIN - 40, SWP_SHOWWINDOW);
	forceMinimumSize(windowWidth, windowHeight, rc.left, rc.top);
}

static void resizePairsTab()
{
	RECT rc = {0};
	GetWindowRect(mainHwnd, &rc);
	int windowHeight = rc.bottom - rc.top;
	int windowWidth = rc.right - rc.left;

	resizeTab(windowWidth, windowHeight);
	SetWindowPos(lbSourceHwnd, HWND_TOP, LISTBOX_LEFT, LISTBOX_TOP, (windowWidth / 2) - TAB_MARGIN, windowHeight - LISTBOX_TOP - TAB_MARGIN - 40, SWP_SHOWWINDOW);
	SetWindowPos(lbDestHwnd, HWND_TOP, (windowWidth / 2) + 5, LISTBOX_TOP, (windowWidth / 2) - TAB_MARGIN - 20, windowHeight - LISTBOX_TOP - TAB_MARGIN - 40, SWP_SHOWWINDOW);
	forceMinimumSize(windowWidth, windowHeight, rc.left, rc.top);
}

static void resizeSyncTab()
{
	RECT rc = {0};
	GetWindowRect(mainHwnd, &rc);
	int windowHeight = rc.bottom - rc.top;
	int windowWidth = rc.right - rc.left;

	resizeTab(windowWidth, windowHeight);
	SetWindowPos(lbSyncHwnd, HWND_TOP, LISTBOX_LEFT, LISTBOX_TOP, windowWidth - RIGHT_MARGIN - 15, windowHeight - LISTBOX_TOP - PROGRESS_BAR_HEIGHT - 50, SWP_SHOWWINDOW);
	SetWindowPos(pbHwnd, HWND_TOP, LISTBOX_LEFT, windowHeight - PROGRESS_BAR_HEIGHT - TAB_MARGIN - 40, windowWidth - RIGHT_MARGIN - 15, PROGRESS_BAR_HEIGHT, SWP_SHOWWINDOW);
	forceMinimumSize(windowWidth, windowHeight, rc.left, rc.top);
}

static void resizeSettingsTab()
{
	RECT rc = {0};
	GetWindowRect(mainHwnd, &rc);
	int windowHeight = rc.bottom - rc.top;
	int windowWidth = rc.right - rc.left;

	resizeTab(windowWidth, windowHeight);
	forceMinimumSize(windowWidth, windowHeight, rc.left, rc.top);
}
