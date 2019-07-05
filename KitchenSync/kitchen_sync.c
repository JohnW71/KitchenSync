#define _CRT_SECURE_NO_WARNINGS
#define UNICODE
#define _UNICODE
#define DEV_MODE 1

#define LOG_FILE "kitchen_sync.log"
#define INI_FILE "kitchen_sync.ini"
#define PROJECTS "kitchen_sync.prj"
#define MAX_LINE 512
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define WINDOW_HEIGHT_MINIMUM 200
#define ID_LISTBOX_PROJECTS 20
#define ID_LISTBOX_PAIRS 21
#define ID_PROGRESS_BAR 22
#define ID_BUTTON_PREVIEW 23
#define ID_BUTTON_SYNC 24
#define ID_BUTTON_ADD_PROJECT 25
#define ID_BUTTON_ADD_FOLDERS 26
#define ID_BUTTON_OK 27
#define ID_BUTTON_CANCEL 28
#define ID_ADD_PROJECT_NAME_LABEL 29
#define ID_ADD_PROJECT_NAME_TEXT 30
#define ID_TIMER1 31

#define arrayCount(array) (sizeof(array) / sizeof((array)[0]))

#if DEV_MODE
#define assert(expression) if(!(expression)) {*(int *)0 = 0;}
#else
#define assert(expression)
#endif

#include <stdio.h>
//#include <stdlib.h> // rand()
//#include <time.h> // time()
//#include <stdint.h>
#include <stdbool.h>
#include <windows.h>
#include <windowsx.h>

#include "kitchen_sync.h"

struct Pair
{
	wchar_t source[MAX_LINE];
	wchar_t destination[MAX_LINE];
};

struct PairNode
{
	struct Pair pair;
	struct PairNode *next;
};

struct Project
{
	wchar_t name[MAX_LINE];
	struct Pair pair;
};

struct ProjectNode
{
	struct Project project;
	struct ProjectNode *next;
};

// create empty lists
static struct PairNode *filesHead = NULL;
static struct ProjectNode *projectsHead = NULL;

static LRESULT CALLBACK mainWndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK customListboxProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK addProjectNameWndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK customAddProjectNameProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK editProjectNameWndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK customEditProjectNameProc(HWND, UINT, WPARAM, LPARAM);
static WNDPROC originalListboxProc;
static WNDPROC originalAddProjectNameProc;
static WNDPROC originalEditProjectNameProc;
static HWND mainHwnd;
static HWND addProjectNameHwnd;
static HWND editProjectNameHwnd;
static HWND lbProjectsHwnd;
static HWND lbPairsHwnd;
static HWND bAddProjectNameOK;
static HWND bEditProjectNameOK;
static HINSTANCE instance;

static void shutDown(void);
static void centerWindow(HWND);
static void clearArray(char *, int);
static void clearArrayW(wchar_t *, int);
static void clearNewlines(char *, int);
static void clearNewlinesW(wchar_t *, int);
static void writeFile(char *, char *);
static void writeFileW(char *, wchar_t *);
static void writeSettings(char *, HWND);
static void readSettings(char *, HWND);
static void loadProjects(char *, struct ProjectNode **);
static void saveProjects(char *, struct ProjectNode **);
static void appendPairNode(struct PairNode **, struct Pair);
static void appendProjectNode(struct ProjectNode **, struct Project);
static void deletePairNode(struct PairNode **, int);
static void deleteProjectNode(struct ProjectNode **, int);
static void deletePairList(struct PairNode **);
static void deleteProjectList(struct ProjectNode **);
static void addProjectName(void);
static void editProjectName(void);
static void fillListbox(struct ProjectNode **);
static int countPairNodes(struct PairNode *);
static int countProjectNodes(struct ProjectNode *);
static bool isProjectName(wchar_t *, int);

static bool showingAddProject = false;
static bool showingEditProject = false;
wchar_t projectName[MAX_LINE] = {0};

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR pCmdLine, _In_ int nCmdShow)
{
	//srand((unsigned int)time(NULL));
	instance = hInstance;

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
		L"KitchenSync v0.1",
		//WS_OVERLAPPEDWINDOW,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT,
		NULL, NULL, hInstance, NULL);

	if (!mainHwnd)
	{
		MessageBox(NULL, L"Window creation failed", L"Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	// reset log file
	FILE *lf = fopen(LOG_FILE, "wt, ccs=UNICODE");
	if (lf == NULL)
		MessageBox(NULL, L"Can't open log file", L"Error", MB_ICONEXCLAMATION | MB_OK);
	else
	{
		fwprintf(lf, L"%s\n", L"Log file reset");
		fclose(lf);
	}

	ShowWindow(mainHwnd, nCmdShow);
	readSettings(INI_FILE, mainHwnd);
	loadProjects(PROJECTS, &projectsHead);

	MSG msg = {0};

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

LRESULT CALLBACK mainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND bPreview, bSync, bAddProject, bAddFolders;
	static bool listboxClicked = false;

	switch (msg)
	{
		case WM_CREATE:
			// listbox
			lbProjectsHwnd = CreateWindowEx(WS_EX_LEFT, L"ListBox", NULL,
				WS_VISIBLE | WS_CHILD | LBS_NOTIFY | WS_VSCROLL | WS_BORDER | LBS_EXTENDEDSEL,
				10, 10, 500, 400, hwnd, (HMENU)ID_LISTBOX_PROJECTS, NULL, NULL);
			originalListboxProc = (WNDPROC)SetWindowLongPtr(lbProjectsHwnd, GWLP_WNDPROC, (LONG_PTR)customListboxProc);

			lbPairsHwnd = CreateWindowEx(WS_EX_LEFT, L"ListBox", NULL,
				WS_VISIBLE | WS_CHILD | LBS_NOTIFY | WS_VSCROLL | WS_BORDER | LBS_EXTENDEDSEL,
				520, 10, WINDOW_WIDTH-500-45, WINDOW_HEIGHT - 160, hwnd, (HMENU)ID_LISTBOX_PAIRS, NULL, NULL);
			originalListboxProc = (WNDPROC)SetWindowLongPtr(lbPairsHwnd, GWLP_WNDPROC, (LONG_PTR)customListboxProc);

			bPreview = CreateWindowEx(WS_EX_LEFT, L"Button", L"Preview",
				WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_DISABLED,
				390, 420, 120, 50, hwnd, (HMENU)ID_BUTTON_PREVIEW, NULL, NULL);

			bSync = CreateWindowEx(WS_EX_LEFT, L"Button", L"Sync",
				WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_DISABLED,
				390, 480, 120, 50, hwnd, (HMENU)ID_BUTTON_SYNC, NULL, NULL);

			bAddProject = CreateWindowEx(WS_EX_LEFT, L"Button", L"Add Project",
				WS_VISIBLE | WS_CHILD | WS_TABSTOP,
				10, 420, 120, 30, hwnd, (HMENU)ID_BUTTON_ADD_PROJECT, NULL, NULL);

			bAddFolders = CreateWindowEx(WS_EX_LEFT, L"Button", L"Add Folder Pair",
				WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_DISABLED,
				140, 420, 120, 30, hwnd, (HMENU)ID_BUTTON_ADD_FOLDERS, NULL, NULL);

			break;
		case WM_COMMAND:
			if (LOWORD(wParam) == ID_LISTBOX_PROJECTS)
			{

				// a row was selected
				if (HIWORD(wParam) == LBN_SELCHANGE)
				{
					EnableWindow(bAddFolders, FALSE);

					// get row index
					LRESULT selectedRow = SendMessage(lbProjectsHwnd, LB_GETCURSEL, 0, 0);

					if (selectedRow != LB_ERR)
					{
						wchar_t selectedRowText[MAX_LINE] = {0};
						int textLen = (int)SendMessage(lbProjectsHwnd, LB_GETTEXT, selectedRow, (LPARAM)selectedRowText);

						if (textLen > 0)
							if (isProjectName(selectedRowText, textLen))
								EnableWindow(bAddFolders, TRUE);
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
								editProjectName();
							}
							//else
								//editFolderPair();
						}
					}
				}
			}

			if (LOWORD(wParam) == ID_LISTBOX_PAIRS)
			{
				// a row was selected
				if (HIWORD(wParam) == LBN_SELCHANGE)
				{
					// get row index
					LRESULT selectedRow = SendMessage(lbPairsHwnd, LB_GETCURSEL, 0, 0);

					if (selectedRow != LB_ERR)
					{
						// store selected row in case of Del key?
					}
				}

				// a row was double-clicked
				if (HIWORD(wParam) == LBN_DBLCLK)
				{
					// get row index
					LRESULT selectedRow = SendMessage(lbPairsHwnd, LB_GETCURSEL, 0, 0);

					if (selectedRow != LB_ERR)
					{
						// edit file pair
					}
				}
			}

			if (LOWORD(wParam) == ID_BUTTON_ADD_PROJECT)
			{
				addProjectName();
			}
			if (LOWORD(wParam) == ID_BUTTON_ADD_FOLDERS)
			{
				writeFileW(LOG_FILE, L"add folder pair");
			}
			break;
		case WM_SIZE:
		{
			//RECT rc = {0};
			//GetWindowRect(hwnd, &rc);
			//int windowHeight = rc.bottom - rc.top;

			// resize listbox & textbox to fit window
			//SetWindowPos(listboxProjectsHwnd, HWND_TOP, 10, 10, 500, windowHeight - 320, SWP_SHOWWINDOW);
			//SetWindowPos(listboxPairsHwnd, HWND_TOP, 520, 10, WINDOW_WIDTH-500-45, 25, SWP_SHOWWINDOW);

			// maintain main window width
			//SetWindowPos(hwnd, HWND_TOP, rc.left, rc.top, WINDOW_WIDTH, windowHeight, SWP_SHOWWINDOW);

			// force minimum height
			//if (windowHeight < WINDOW_HEIGHT_MINIMUM)
			//	SetWindowPos(hwnd, HWND_TOP, rc.left, rc.top, WINDOW_WIDTH, WINDOW_HEIGHT_MINIMUM, SWP_SHOWWINDOW);
		}
			break;
		case WM_KEYUP:
			switch (wParam)
			{
				case VK_ESCAPE:
					shutDown();
					break;
			}
			break;
		case WM_DESTROY:
			shutDown();
			break;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK customListboxProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		//case WM_LBUTTONDBLCLK:
		//{
		//	listboxDoubleClicked = true;
		//	writeFileW(LOG_FILE, L"Listbox double clicked");

			//int xPos = GET_X_LPARAM(lParam);
			//int yPos = GET_Y_LPARAM(lParam);

			//RECT r;
			//HWND h = GetDlgItem(lbProjectsHwnd, ID_LISTBOX_PROJECTS);
			//GetWindowRect(h, &r); //get window rect of control relative to screen
			//POINT pt = {r.left, r.top}; //new point object using rect x, y
			//ScreenToClient(lbProjectsHwnd, &pt);

			//wchar_t buf[100] = {0};
			//swprintf(buf, 100, L"xPos:%d yPos:%d dlg left:%d dlg top:%d", xPos, yPos, pt.x, pt.y);
			//writeFileW(LOG_FILE, buf);


			//addProjectName();

		//}
		//	break;
		case WM_KEYUP:
			switch (wParam)
			{
				case VK_ESCAPE:
					shutDown();
					break;
			}
			break;
	}
	return CallWindowProc(originalListboxProc, hwnd, msg, wParam, lParam);
}

static void addProjectName(void)
{
	static WNDCLASSEX wcaddProjectName = {0};
	static bool addProjectNameClassRegistered = false;

	if (showingAddProject)
		return;
	showingAddProject = true;

	if (!addProjectNameClassRegistered)
	{
		wcaddProjectName.cbSize = sizeof(WNDCLASSEX);
		wcaddProjectName.cbClsExtra = 0;
		wcaddProjectName.cbWndExtra = 0;
		wcaddProjectName.hbrBackground = (HBRUSH)COLOR_WINDOW;
		wcaddProjectName.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcaddProjectName.hIcon = NULL;
		wcaddProjectName.hIconSm = NULL;
		wcaddProjectName.hInstance = instance;
		wcaddProjectName.lpfnWndProc = addProjectNameWndProc;
		wcaddProjectName.lpszClassName = L"addProjectName";
		wcaddProjectName.lpszMenuName = NULL;
		wcaddProjectName.style = CS_HREDRAW | CS_VREDRAW;

		if (!RegisterClassEx(&wcaddProjectName))
		{
			MessageBox(NULL, L"Add Project Name window registration failed", L"Error", MB_ICONEXCLAMATION | MB_OK);
			return;
		}
		else
			addProjectNameClassRegistered = true;
	}

	addProjectNameHwnd = CreateWindowEx(WS_EX_LEFT,
		wcaddProjectName.lpszClassName,
		L"Add new project",
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT,
		435, 130,
		NULL, NULL,
		instance, NULL);

	if (!addProjectNameHwnd)
	{
		MessageBox(NULL, L"Get Project Name window creation failed", L"Error", MB_ICONEXCLAMATION | MB_OK);
		return;
	}

	ShowWindow(addProjectNameHwnd, SW_SHOW);
}

static LRESULT CALLBACK addProjectNameWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND bAddProjectNameCancel, lAddProjectName, eAddProjectName;

	switch (msg)
	{
		case WM_CREATE:
			SetTimer(hwnd, ID_TIMER1, 100, NULL);

			lAddProjectName = CreateWindowEx(WS_EX_LEFT, L"Static", L"Enter project name",
				WS_VISIBLE | WS_CHILD,
				10, 15, 150, 25, hwnd, (HMENU)ID_ADD_PROJECT_NAME_LABEL, NULL, NULL);

			eAddProjectName = CreateWindowEx(WS_EX_LEFT, L"Edit", NULL,
				WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_BORDER,
				140, 10, 170, 25, hwnd, (HMENU)ID_ADD_PROJECT_NAME_TEXT, NULL, NULL);
			originalAddProjectNameProc = (WNDPROC)SetWindowLongPtr(eAddProjectName, GWLP_WNDPROC, (LONG_PTR)customAddProjectNameProc);

			bAddProjectNameOK = CreateWindowEx(WS_EX_LEFT, L"Button", L"OK",
				WS_VISIBLE | WS_CHILD | WS_TABSTOP,
				320, 10, 80, 25, hwnd, (HMENU)ID_BUTTON_OK, NULL, NULL);

			bAddProjectNameCancel = CreateWindowEx(WS_EX_LEFT, L"Button", L"Cancel",
				WS_VISIBLE | WS_CHILD | WS_TABSTOP,
				320, 45, 80, 25, hwnd, (HMENU)ID_BUTTON_CANCEL, NULL, NULL);

			SendMessage(eAddProjectName, EM_LIMITTEXT, MAX_LINE, 0);
			centerWindow(hwnd);
			break;
		case WM_COMMAND:
			if (LOWORD(wParam) == ID_BUTTON_OK)
			{
				wchar_t name[MAX_LINE];
				GetWindowText(eAddProjectName, name, MAX_LINE);
				wchar_t buf[100] = {0};
				swprintf(buf, 100, L"New project name: %s", name);
				writeFileW(LOG_FILE, buf);
				DestroyWindow(hwnd);
			}

			if (LOWORD(wParam) == ID_BUTTON_CANCEL)
			{
				DestroyWindow(hwnd);
			}
			break;
		case WM_TIMER:
			if (wParam == ID_TIMER1)
			{
				SetFocus(eAddProjectName);
				KillTimer(hwnd, ID_TIMER1);
			}
			break;
		case WM_DESTROY:
			showingAddProject = false;
			DestroyWindow(hwnd);
			break;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK customAddProjectNameProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_KEYUP:
			switch (wParam)
			{
				case VK_ESCAPE:
					DestroyWindow(addProjectNameHwnd);
					break;
				case VK_RETURN:
					SendMessage(bAddProjectNameOK, BM_CLICK, 0, 0);
					break;
				case 'A': // CTRL A
					if (GetAsyncKeyState(VK_CONTROL))
						SendMessage(hwnd, EM_SETSEL, 0, -1);
					break;
			}
			break;
	}

	return CallWindowProc(originalAddProjectNameProc, hwnd, msg, wParam, lParam);
}

static void editProjectName(void)
{
	static WNDCLASSEX wcEditProjectName = {0};
	static bool editProjectNameClassRegistered = false;

	if (showingEditProject)
		return;
	showingEditProject = true;

	if (!editProjectNameClassRegistered)
	{
		wcEditProjectName.cbSize = sizeof(WNDCLASSEX);
		wcEditProjectName.cbClsExtra = 0;
		wcEditProjectName.cbWndExtra = 0;
		wcEditProjectName.hbrBackground = (HBRUSH)COLOR_WINDOW;
		wcEditProjectName.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcEditProjectName.hIcon = NULL;
		wcEditProjectName.hIconSm = NULL;
		wcEditProjectName.hInstance = instance;
		wcEditProjectName.lpfnWndProc = editProjectNameWndProc;
		wcEditProjectName.lpszClassName = L"EditProjectName";
		wcEditProjectName.lpszMenuName = NULL;
		wcEditProjectName.style = CS_HREDRAW | CS_VREDRAW;

		if (!RegisterClassEx(&wcEditProjectName))
		{
			MessageBox(NULL, L"Edit Project Name window registration failed", L"Error", MB_ICONEXCLAMATION | MB_OK);
			return;
		}
		else
			editProjectNameClassRegistered = true;
	}

	editProjectNameHwnd = CreateWindowEx(WS_EX_LEFT,
		wcEditProjectName.lpszClassName,
		L"Edit project",
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT,
		435, 130,
		NULL, NULL,
		instance, NULL);

	if (!editProjectNameHwnd)
	{
		MessageBox(NULL, L"Edit Project Name window creation failed", L"Error", MB_ICONEXCLAMATION | MB_OK);
		return;
	}

	ShowWindow(editProjectNameHwnd, SW_SHOW);
}

static LRESULT CALLBACK editProjectNameWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND bEditProjectNameCancel, lEditProjectName, eEditProjectName;

	switch (msg)
	{
		case WM_CREATE:
			SetTimer(hwnd, ID_TIMER1, 100, NULL);

			lEditProjectName = CreateWindowEx(WS_EX_LEFT, L"Static", L"Enter project name",
				WS_VISIBLE | WS_CHILD,
				10, 15, 150, 25, hwnd, (HMENU)ID_ADD_PROJECT_NAME_LABEL, NULL, NULL);

			eEditProjectName = CreateWindowEx(WS_EX_LEFT, L"Edit", NULL,
				WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_BORDER,
				140, 10, 170, 25, hwnd, (HMENU)ID_ADD_PROJECT_NAME_TEXT, NULL, NULL);
			originalEditProjectNameProc = (WNDPROC)SetWindowLongPtr(eEditProjectName, GWLP_WNDPROC, (LONG_PTR)customEditProjectNameProc);

			bEditProjectNameOK = CreateWindowEx(WS_EX_LEFT, L"Button", L"OK",
				WS_VISIBLE | WS_CHILD | WS_TABSTOP,
				320, 10, 80, 25, hwnd, (HMENU)ID_BUTTON_OK, NULL, NULL);

			bEditProjectNameCancel = CreateWindowEx(WS_EX_LEFT, L"Button", L"Cancel",
				WS_VISIBLE | WS_CHILD | WS_TABSTOP,
				320, 45, 80, 25, hwnd, (HMENU)ID_BUTTON_CANCEL, NULL, NULL);

			SendMessage(eEditProjectName, EM_LIMITTEXT, MAX_LINE, 0);
			if (wcslen(projectName) > 0)
				SetWindowText(eEditProjectName, projectName);
			centerWindow(hwnd);
			break;
		case WM_COMMAND:
			if (LOWORD(wParam) == ID_BUTTON_OK)
			{
				wchar_t newProjectName[MAX_LINE];
				GetWindowText(eEditProjectName, newProjectName, MAX_LINE);
				wchar_t buf[100] = {0};
				swprintf(buf, 100, L"New project name: %s", newProjectName);
				writeFileW(LOG_FILE, buf);

				if (wcscmp(projectName, newProjectName) != 0)
				{
					// replace all occurrences of old project name with new name
					struct ProjectNode *current = projectsHead;
					while (current != NULL)
					{
						if (wcscmp(current->project.name, projectName) == 0)
							wcscpy_s(current->project.name, MAX_LINE, newProjectName);

						current = current->next;
					}
				}

				SendMessage(lbProjectsHwnd, LB_RESETCONTENT, 0, 0);
				fillListbox(&projectsHead);
				DestroyWindow(hwnd);
			}

			if (LOWORD(wParam) == ID_BUTTON_CANCEL)
			{
				DestroyWindow(hwnd);
			}
			break;
		case WM_TIMER:
			if (wParam == ID_TIMER1)
			{
				SetFocus(eEditProjectName);
				KillTimer(hwnd, ID_TIMER1);
			}
			break;
		case WM_DESTROY:
			clearArrayW(projectName, MAX_LINE);
			showingEditProject = false;
			DestroyWindow(hwnd);
			break;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK customEditProjectNameProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_KEYUP:
			switch (wParam)
			{
				case VK_ESCAPE:
					DestroyWindow(editProjectNameHwnd);
					break;
				case VK_RETURN:
					SendMessage(bEditProjectNameOK, BM_CLICK, 0, 0);
					break;
				case 'A': // CTRL A
					if (GetAsyncKeyState(VK_CONTROL))
						SendMessage(hwnd, EM_SETSEL, 0, -1);
					break;
			}
			break;
	}

	return CallWindowProc(originalEditProjectNameProc, hwnd, msg, wParam, lParam);
}

static void shutDown(void)
{
	writeSettings(INI_FILE, mainHwnd);
	saveProjects(PROJECTS, &projectsHead);
	PostQuitMessage(0);
}

static void centerWindow(HWND hwnd)
{
	RECT rc = {0};

	GetWindowRect(hwnd, &rc);
	int windowWidth = rc.right - rc.left;
	int windowHeight = rc.bottom - rc.top;
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	SetWindowPos(hwnd, HWND_TOP, (screenWidth - windowWidth) / 2,
		(screenHeight - windowHeight) / 2, 0, 0, SWP_NOSIZE);
}

static void clearArray(char *array, int length)
{
	for (int i = 0; i < length; ++i)
		array[i] = '\0';
}

static void clearArrayW(wchar_t *array, int length)
{
	for (int i = 0; i < length; ++i)
		array[i] = '\0';
}

static void clearNewlines(char *array, int length)
{
	for (int i = 0; i < length; ++i)
		if (array[i] == '\n')
			array[i] = '\0';
}

static void clearNewlinesW(wchar_t *array, int length)
{
	for (int i = 0; i < length; ++i)
		if (array[i] == '\n')
			array[i] = '\0';
}

static void writeFile(char *filename, char *text)
{
	//while (state.writing)
	//	Sleep(100);

	//state.writing = true;

	FILE *f = fopen(filename, "a");
	if (f == NULL)
	{
		MessageBox(NULL, L"Can't open file", L"Error", MB_ICONEXCLAMATION | MB_OK);
		return;
	}

	fprintf(f, "%s\n", text);
	fclose(f);
	//state.writing = false;
}

static void writeFileW(char *filename, wchar_t *text)
{
	//while (state.writing)
	//	Sleep(100);

	//state.writing = true;

	FILE *f = fopen(filename, "at, ccs=UNICODE");
	if (f == NULL)
	{
		MessageBox(NULL, L"Can't open file", L"Error", MB_ICONEXCLAMATION | MB_OK);
		return;
	}

	fwprintf(f, L"%s\n", text);
	fclose(f);
	//state.writing = false;
}

static void writeSettings(char *filename, HWND hwnd)
{
	writeFileW(LOG_FILE, L"writeSettings()");

	FILE *f = fopen(filename, "w");
	if (f == NULL)
	{
		writeFileW(LOG_FILE, L"Error saving settings!");
		return;
	}

	RECT rc = {0};
	GetWindowRect(hwnd, &rc);
	int windowHeight = rc.bottom - rc.top;
	int windowCol = rc.left;
	int windowRow = rc.top;

	fprintf(f, "window_row=%d\n", windowRow);
	fprintf(f, "window_col=%d\n", windowCol);
	fprintf(f, "window_height=%d\n", windowHeight);
	fclose(f);
}

static void readSettings(char *filename, HWND hwnd)
{
	writeFileW(LOG_FILE, L"readSettings()");

	FILE *f = fopen(filename, "r");
	if (f == NULL)
	{
		int screenWidth = GetSystemMetrics(SM_CXSCREEN);
		int screenHeight = GetSystemMetrics(SM_CYSCREEN);
		SetWindowPos(hwnd, HWND_TOP, (screenWidth - WINDOW_WIDTH) / 2, (screenHeight - WINDOW_HEIGHT) / 2, WINDOW_WIDTH, WINDOW_HEIGHT, SWP_SHOWWINDOW);
		return;
	}

	int windowHeight = 0;
	int windowCol = 0;
	int windowRow = 0;
	char *line = (char *)malloc(MAX_LINE);
	if (!line)
	{
		writeFileW(LOG_FILE, L"Failed to allocate memory for line from settings file");
		fclose(f);
		return;
	}

	while (fgets(line, MAX_LINE, f) != NULL)
	{
		if (line[0] == '#')
			continue;

		char *setting = (char *)calloc(MAX_LINE, sizeof(char));
		if (!setting)
		{
			writeFileW(LOG_FILE, L"Failed to allocate memory for setting from settings file");
			fclose(f);
			return;
		}
		char *value = (char *)calloc(MAX_LINE, sizeof(char));
		if (!value)
		{
			writeFileW(LOG_FILE, L"Failed to allocate memory for value from settings file");
			fclose(f);
			return;
		}
		char *l = line;
		char *s = setting;
		char *v = value;

		// read setting
		while (*l && *l != '=')
			*s++ = *l++;
		*s = '\0';

		// read value
		++l;
		while (*l)
			*v++ = *l++;
		*v = '\0';

		if (strcmp(setting, "window_row") == 0)		windowRow = atoi(value);
		if (strcmp(setting, "window_col") == 0)		windowCol = atoi(value);
		if (strcmp(setting, "window_height") == 0)	windowHeight = atoi(value);
	}

	fclose(f);

	if (windowHeight == 0)
		windowHeight = WINDOW_HEIGHT;
	if (windowRow == 0)
	{
	 	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
		windowRow = (screenHeight - windowHeight) / 2;
	}
	if (windowCol == 0)
	{
	 	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
		windowCol = (screenWidth - WINDOW_WIDTH) / 2;
	}

	SetWindowPos(hwnd, HWND_TOP, windowCol, windowRow, WINDOW_WIDTH, windowHeight, SWP_SHOWWINDOW);
}

static void loadProjects(char *filename, struct ProjectNode **head_ref)
{
	writeFileW(LOG_FILE, L"loadProjects()");

	FILE *f = fopen(filename, "rt, ccs=UNICODE");
	if (f == NULL)
	{
		writeFileW(LOG_FILE, L"No project history found");
		return;
	}

	wchar_t *line = (wchar_t *)malloc(MAX_LINE * 4);
	if (!line)
	{
		writeFileW(LOG_FILE, L"Failed to allocate memory for line from projects file");
		fclose(f);
		return;
	}

	while (fgetws(line, MAX_LINE * 2, f) != NULL)
	{
		if (line[0] == '#' || line[0] == '\n' || line[0] == '\0')
			continue;

		wchar_t *name = (wchar_t *)calloc(MAX_LINE, sizeof(wchar_t));
		if (!name)
		{
			writeFileW(LOG_FILE, L"Failed to allocate memory for name from projects file");
			fclose(f);
			return;
		}
		wchar_t *source = (wchar_t *)calloc(MAX_LINE, sizeof(wchar_t));
		if (!source)
		{
			writeFileW(LOG_FILE, L"Failed to allocate memory for source from projects file");
			fclose(f);
			return;
		}
		wchar_t *destination = (wchar_t *)calloc(MAX_LINE, sizeof(wchar_t));
		if (!destination)
		{
			writeFileW(LOG_FILE, L"Failed to allocate memory for destination from projects file");
			fclose(f);
			return;
		}
		wchar_t *l = line;
		wchar_t *n = name;
		wchar_t *s = source;
		wchar_t *d = destination;

		// read name
		while (*l && *l != ',')
			*n++ = *l++;
		*n = '\0';

		// read source
		++l;
		while (*l && *l != ',')
			*s++ = *l++;
		*s = '\0';

		// read destination
		++l;
		while (*l && *l != '\n' && *l != '\0')
			*d++ = *l++;
		*d = '\0';

		wchar_t *buf = (wchar_t *)calloc(MAX_LINE * 4, sizeof(wchar_t));
		if (!buf)
		{
			writeFileW(LOG_FILE, L"Failed to allocate memory for logging buf");
			fclose(f);
			return;
		}
		swprintf(buf, MAX_LINE * 4, L"Name: %s, source: %s, dest: %s", name, source, destination);
		writeFileW(LOG_FILE, buf);

		struct Project project = {0};
		wcscpy_s(project.name, MAX_LINE, name);
		wcscpy_s(project.pair.source, MAX_LINE, source);
		wcscpy_s(project.pair.destination, MAX_LINE, destination);

		appendProjectNode(head_ref, project);
	}

	wchar_t buf[100] = {0};
	swprintf(buf, 100, L"Loaded %d projects", countProjectNodes(*head_ref));
	writeFileW(LOG_FILE, buf);

	fclose(f);

	// add test data
	//struct Project project = {0};
	//wchar_t name[100] = L"name";
	//wchar_t source[100] = L"source";
	//wchar_t destination[100] = L"destination";

	//int rn = rand() % 1024;
	//rn = rand() % 1024;

	//clearArrayW(buf, 100);

	//swprintf(buf, 100, L"%s%d", name, rn);
	//wcscpy_s(project.name, 100, buf);

	//swprintf(buf, 100, L"%s%d", source, rn);
	//wcscpy_s(project.pair.source, 100, buf);

	//swprintf(buf, 100, L"%s%d", destination, rn);
	//wcscpy_s(project.pair.destination, 100, buf);

	//appendProjectNode(head_ref, project);

	fillListbox(head_ref);
}

static void fillListbox(struct ProjectNode **head_ref)
{
	// populate listbox
	wchar_t *currentProjectName = (wchar_t *)calloc(MAX_LINE, sizeof(wchar_t));
	if (!currentProjectName)
	{
		writeFileW(LOG_FILE, L"Failed to allocate memory for currentProjectName from projects file");
		return;
	}

	int position = 0;
	struct ProjectNode *current = projectsHead;

	while (current != NULL)
	{
		// add folder pair, into a new or existing project section
		if (wcscmp(currentProjectName, current->project.name) != 0) // new project section
		{
			// add blank line between each project
			if (position > 0)
				SendMessage(lbProjectsHwnd, LB_ADDSTRING, position++, (LPARAM)"");

			SendMessage(lbProjectsHwnd, LB_ADDSTRING, position++, (LPARAM)current->project.name);
		}

		wchar_t *buffer = (wchar_t *)calloc(MAX_LINE*4, sizeof(wchar_t));
		if (!buffer)
		{
			writeFileW(LOG_FILE, L"Failed to allocate memory for folder pair buffer from projects file");
			return;
		}

		swprintf(buffer, MAX_LINE*4, L"%s -> %s", current->project.pair.source, current->project.pair.destination);
		SendMessage(lbProjectsHwnd, LB_ADDSTRING, position++, (LPARAM)buffer);

		wcscpy_s(currentProjectName, MAX_LINE, current->project.name);
		current = current->next;
	}
}

static void saveProjects(char *filename, struct ProjectNode **head_ref)
{
	writeFileW(LOG_FILE, L"saveProjects()");

	FILE *f = fopen(filename, "wt, ccs=UNICODE");
	if (f == NULL)
	{
		writeFileW(LOG_FILE, L"Unable to save project history");
		return;
	}

	struct ProjectNode *current = projectsHead;
	int count = 0;

	while (current != NULL)
	{
		wchar_t *buf = (wchar_t *)calloc(MAX_LINE*4, sizeof(wchar_t));
		if (!buf)
		{
			writeFileW(LOG_FILE, L"Failed to allocate memory for buf for projects file");
			return;
		}

		swprintf(buf, MAX_LINE * 4, L"%s,%s,%s", current->project.name, current->project.pair.source, current->project.pair.destination);
		writeFileW(PROJECTS, buf);

		swprintf(buf, MAX_LINE * 4, L"Name: %s, source: %s, dest: %s", current->project.name, current->project.pair.source, current->project.pair.destination);
		writeFileW(LOG_FILE, buf);

		++count;
		current = current->next;
	}

	wchar_t buf[100] = {0};
	swprintf(buf, 100, L"Saved %d projects", count);
	writeFileW(LOG_FILE, buf);

	fclose(f);
}

// append a new node at the end
static void appendPairNode(struct PairNode **head_ref, struct Pair pair)
{
	writeFileW(LOG_FILE, L"appendPairNode()");

	struct PairNode *newPairNode = (struct PairNode *)malloc(sizeof(struct PairNode));

	if (!newPairNode)
	{
		writeFileW(LOG_FILE, L"Failed to allocate memory for new pair");
		return;
	}

	if (wcscpy_s(newPairNode->pair.source, MAX_LINE, pair.source))
	{
		writeFileW(LOG_FILE, L"Failed copying new source");
		writeFileW(LOG_FILE, pair.source);
		free(newPairNode);
		return;
	}
	if (wcscpy_s(newPairNode->pair.destination, MAX_LINE, pair.destination))
	{
		writeFileW(LOG_FILE, L"Failed copying new destination");
		writeFileW(LOG_FILE, pair.source);
		free(newPairNode);
		return;
	}

	newPairNode->next = NULL;

	// if list is empty set newPairNode as head
	if (*head_ref == NULL)
	{
		*head_ref = newPairNode;
		return;
	}

	// divert current last node to newPairNode
	struct PairNode *last = *head_ref;
	while (last->next != NULL)
		last = last->next;
	last->next = newPairNode;
}

// append a new node at the end
static void appendProjectNode(struct ProjectNode **head_ref, struct Project project)
{
	writeFileW(LOG_FILE, L"appendProjectNode()");

	struct ProjectNode *newProjectNode = (struct ProjectNode *)malloc(sizeof(struct ProjectNode));

	if (!newProjectNode)
	{
		writeFileW(LOG_FILE, L"Failed to allocate memory for new project");
		return;
	}

	if (wcscpy_s(newProjectNode->project.name, MAX_LINE, project.name))
	{
		writeFileW(LOG_FILE, L"Failed copying new project name");
		writeFileW(LOG_FILE, project.name);
		free(newProjectNode);
		return;
	}
	if (wcscpy_s(newProjectNode->project.pair.source, MAX_LINE, project.pair.source))
	{
		writeFileW(LOG_FILE, L"Failed copying new project source");
		writeFileW(LOG_FILE, project.pair.source);
		free(newProjectNode);
		return;
	}
	if (wcscpy_s(newProjectNode->project.pair.destination, MAX_LINE, project.pair.destination))
	{
		writeFileW(LOG_FILE, L"Failed copying new project destination");
		writeFileW(LOG_FILE, project.pair.destination);
		free(newProjectNode);
		return;
	}

	newProjectNode->next = NULL;

	// if list is empty set newProjectNode as head
	if (*head_ref == NULL)
	{
		*head_ref = newProjectNode;
		return;
	}

	// divert current last node to newProjectNode
	struct ProjectNode *last = *head_ref;
	while (last->next != NULL)
		last = last->next;
	last->next = newProjectNode;
}

#if 0
// Given a reference (pointer to pointer) to the head of a list
// and a position, deletes the node at the given position
static void deletePairNode(struct PairNode **head_ref, int position)
{
	if (*head_ref == NULL)
		return;

	// store current head node
	struct PairNode *temp = *head_ref;

	// if head is to be removed
	if (position == 0)
	{
		*head_ref = temp->next;
		free(temp);
		return;
	}

	// find previous node of the node to be deleted
	for (int i = 0; temp != NULL && i < position-1; ++i)
		temp = temp->next;

	// if position is off the end
	if (temp == NULL || temp->next == NULL)
		return;

	// node temp->next is the node to be deleted
	// store pointer to the next node after the node to be deleted
	struct PairNode *next = temp->next->next;

	// unlink the node from the list
	free(temp->next);

	// link to next node
	temp->next = next;
}

// Given a reference (pointer to pointer) to the head of a list
// and a position, deletes the node at the given position
static void deleteProjectNode(struct ProjectNode **head_ref, int position)
{
	if (*head_ref == NULL)
		return;

	// store current head node
	struct ProjectNode *temp = *head_ref;

	// head is to be removed
	if (position == 0)
	{
		*head_ref = temp->next;
		free(temp);
		return;
	}

	// find previous node of the node to be deleted
	for (int i = 0; temp != NULL && i < position-1; ++i)
		temp = temp->next;

	// if position is off the end
	if (temp == NULL || temp->next == NULL)
		return;

	// node temp->next is the node to be deleted
	// store pointer to the next node after the node to be deleted
	struct ProjectNode *next = temp->next->next;

	// unlink the node from the list
	free(temp->next);

	// link to next node
	temp->next = next;
}

// delete the entire linked list
static void deletePairList(struct PairNode **head_ref)
{
	// de-reference head_ref to get the real head
	struct PairNode *current = *head_ref;
	struct PairNode *next;

	while (current != NULL)
	{
		next = current->next;
		free(current);
		current = next;
	}

	*head_ref = NULL;
}

// delete the entire linked list
static void deleteProjectList(struct ProjectNode **head_ref)
{
	// de-reference head_ref to get the real head
	struct ProjectNode *current = *head_ref;
	struct ProjectNode *next;

	while (current != NULL)
	{
		next = current->next;
		free(current);
		current = next;
	}

	*head_ref = NULL;
}
#endif

// count nodes in list
static int countPairNodes(struct PairNode *head)
{
	int count = 0;
	struct PairNode *current = head;

	while (current != NULL)
	{
		++count;
		current = current->next;
	}
	return count;
}

// count nodes in list
static int countProjectNodes(struct ProjectNode *head)
{
	int count = 0;
	struct ProjectNode *current = head;

	while (current != NULL)
	{
		++count;
		current = current->next;
	}
	return count;
}

static bool isProjectName(wchar_t *text, int len)
{
	for (int i = 0; i < len && text[i] != '\0'; ++i)
		if (text[i] == '>')
			return false;
	return true;
}
