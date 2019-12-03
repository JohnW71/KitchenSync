#include "kitchen_sync.h"

static void sizeFormatted(LONGLONG, wchar_t *);
static DWORD CALLBACK entryPointLogger(LPVOID);
static DWORD CALLBACK entryPointProgressBar(LPVOID);

static struct LoggerNode *loggerHead = NULL;
static HANDLE loggerSemaphoreHandle;
static HANDLE progressSemaphoreHandle;

void shutDown(HWND hwnd, struct ProjectNode **head_ref)
{
	while (loggerHead != NULL)
	{
		MessageBox(NULL, L"Logging still in progress", L"Error", MB_ICONEXCLAMATION | MB_OK);
	}
	writeSettings(hwnd, INI_FILE);
	saveProjects(PRJ_FILE, head_ref);
	PostQuitMessage(0);
}

void centerWindow(HWND hwnd)
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

void writeSettings(HWND hwnd, char *filename)
{
	FILE *f = fopen(filename, "w");
	if (f == NULL)
	{
		logger(L"Error saving settings!");
		MessageBox(NULL, L"Error saving settings", L"Error", MB_ICONEXCLAMATION | MB_OK);
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

void readSettings(HWND hwnd, char *filename)
{
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
		logger(L"Failed to allocate memory for line from settings file");
		MessageBox(NULL, L"Failed to allocate memory for line from settings file", L"Error", MB_ICONEXCLAMATION | MB_OK);
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
			logger(L"Failed to allocate memory for setting from settings file");
			MessageBox(NULL, L"Failed to allocate memory for setting from settings file", L"Error", MB_ICONEXCLAMATION | MB_OK);
			fclose(f);
			return;
		}

		char *value = (char *)calloc(MAX_LINE, sizeof(char));
		if (!value)
		{
			logger(L"Failed to allocate memory for value from settings file");
			MessageBox(NULL, L"Failed to allocate memory for value from settings file", L"Error", MB_ICONEXCLAMATION | MB_OK);
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

void loadProjects(HWND hwnd, char *filename, struct ProjectNode **head_ref)
{
	FILE *f = fopen(filename, "rt, ccs=UNICODE");
	if (f == NULL)
	{
		logger(L"No project history found");
		return;
	}

	wchar_t *line = (wchar_t *)malloc(MAX_LINE * 4);
	if (!line)
	{
		logger(L"Failed to allocate memory for line from projects file");
		MessageBox(NULL, L"Failed to allocate memory for line from projects file", L"Error", MB_ICONEXCLAMATION | MB_OK);
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
			logger(L"Failed to allocate memory for name from projects file");
			MessageBox(NULL, L"Failed to allocate memory for name from projects file", L"Error", MB_ICONEXCLAMATION | MB_OK);
			fclose(f);
			return;
		}

		wchar_t *source = (wchar_t *)calloc(MAX_LINE, sizeof(wchar_t));
		if (!source)
		{
			logger(L"Failed to allocate memory for source from projects file");
			MessageBox(NULL, L"Failed to allocate memory for source from projects file", L"Error", MB_ICONEXCLAMATION | MB_OK);
			fclose(f);
			return;
		}

		wchar_t *destination = (wchar_t *)calloc(MAX_LINE, sizeof(wchar_t));
		if (!destination)
		{
			logger(L"Failed to allocate memory for destination from projects file");
			MessageBox(NULL, L"Failed to allocate memory for destination from projects file", L"Error", MB_ICONEXCLAMATION | MB_OK);
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
			logger(L"Failed to allocate memory for logging buf");
			MessageBox(NULL, L"Failed to allocate memory for logging buf", L"Error", MB_ICONEXCLAMATION | MB_OK);
			fclose(f);
			return;
		}

		swprintf(buf, MAX_LINE * 4, L"Name: %s, source: %s, dest: %s", name, source, destination);
		logger(buf);

		struct Project project = {0};
		wcscpy_s(project.name, MAX_LINE, name);
		wcscpy_s(project.pair.source, MAX_LINE, source);
		wcscpy_s(project.pair.destination, MAX_LINE, destination);

		appendProjectNode(head_ref, project);
	}

#if DEV_MODE
	wchar_t buf[100] = {0};
	swprintf(buf, 100, L"Loaded %d projects", countProjectNodes(*head_ref));
	logger(buf);
#endif

	fclose(f);
	sortProjectNodes(head_ref);
	fillListbox(hwnd, head_ref);
}

// load all project nodes into Projects listbox
void fillListbox(HWND hwnd, struct ProjectNode **head_ref)
{
	wchar_t *currentProjectName = (wchar_t *)calloc(MAX_LINE, sizeof(wchar_t));
	if (!currentProjectName)
	{
		logger(L"Failed to allocate memory for currentProjectName from projects file");
		MessageBox(NULL, L"Failed to allocate memory for currentProjectName from projects file", L"Error", MB_ICONEXCLAMATION | MB_OK);
		return;
	}

	int position = 0;
	struct ProjectNode *current = *head_ref;

	while (current != NULL)
	{
		// add folder pair, into a new or existing project section
		if (wcscmp(currentProjectName, current->project.name) != 0) // new project section
		{
			// add blank line between each project
			if (position > 0)
				SendMessage(hwnd, LB_ADDSTRING, position++, (LPARAM)"");

			SendMessage(hwnd, LB_ADDSTRING, position++, (LPARAM)current->project.name);
		}

		wchar_t *buffer = (wchar_t *)calloc(MAX_LINE * 4, sizeof(wchar_t));
		if (!buffer)
		{
			logger(L"Failed to allocate memory for folder pair buffer from projects file");
			MessageBox(NULL, L"Failed to allocate memory for folder pair buffer from projects file", L"Error", MB_ICONEXCLAMATION | MB_OK);
			return;
		}

		swprintf(buffer, MAX_LINE * 4, L"%s -> %s", current->project.pair.source, current->project.pair.destination);
		SendMessage(hwnd, LB_ADDSTRING, position++, (LPARAM)buffer);
		wcscpy_s(currentProjectName, MAX_LINE, current->project.name);
		current = current->next;
	}
}

// load all file pair nodes into Sync listbox
void fillSyncListbox(HWND hwnd, struct PairNode **head_ref)
{
	wchar_t *currentPairName = (wchar_t *)calloc(MAX_LINE, sizeof(wchar_t));
	if (!currentPairName)
	{
		logger(L"Failed to allocate memory for currentPairName from preview list");
		MessageBox(NULL, L"Failed to allocate memory for currentPairName from preview list", L"Error", MB_ICONEXCLAMATION | MB_OK);
		return;
	}

	LONGLONG totalSize = 0;
	int position = 0;
	struct PairNode *current = *head_ref;

	while (current != NULL)
	{
		wchar_t *buffer = (wchar_t *)calloc(MAX_LINE * 3, sizeof(wchar_t));
		if (!buffer)
		{
			logger(L"Failed to allocate memory for file pair buffer from preview list");
			MessageBox(NULL, L"Failed to allocate memory for file pair buffer from preview list", L"Error", MB_ICONEXCLAMATION | MB_OK);
			return;
		}

		LONGLONG size = current->pair.filesize;
		totalSize += size;
		wchar_t formatted[MAX_LINE] = {0};
		sizeFormatted(size, formatted);
		swprintf(buffer, MAX_LINE * 3, L"%s -> %s  (%s)", current->pair.source, current->pair.destination, formatted);
		SendMessage(hwnd, LB_ADDSTRING, position++, (LPARAM)buffer);
		current = current->next;
	}

	wchar_t buf[MAX_LINE] = {0};
	wchar_t formatted[MAX_LINE] = {0};
	sizeFormatted(totalSize, formatted);
	swprintf(buf, MAX_LINE, L"Total size to copy: %s", formatted);
	SendMessage(hwnd, LB_ADDSTRING, position++, (LPARAM)L"");
	SendMessage(hwnd, LB_ADDSTRING, position, (LPARAM)buf);
}

static void sizeFormatted(LONGLONG size, wchar_t *buf)
{
	#define LIMIT 256

	// convert size to string
	wchar_t text[LIMIT] = {0};
	swprintf(text, LIMIT, L"%lld", size);
	size_t length = wcslen(text);
	wchar_t *t = text + (length - 1);

	wchar_t reversed[LIMIT] = {0};
	wchar_t *r = reversed;

	// reverse it and add commas
	int count = 0;
	while (length--)
	{
		*r++ = *t--;

		if (++count == 3 && length > 0)
		{
			*r++ = ',';
			count = 0;
		}
	}

	// reverse again into buf
	length = wcslen(reversed);
	r = reversed + (length - 1);

	while (length--)
		*buf++ = *r--;
}

void saveProjects(char *filename, struct ProjectNode **head_ref)
{
	FILE *f = fopen(filename, "wt, ccs=UNICODE");
	if (f == NULL)
	{
		logger(L"Unable to save project history");
		return;
	}

	struct ProjectNode *current = *head_ref;
	int count = 0;

	while (current != NULL)
	{
		wchar_t *buf = (wchar_t *)calloc(MAX_LINE * 4, sizeof(wchar_t));
		if (!buf)
		{
			logger(L"Failed to allocate memory for buf for projects file");
			MessageBox(NULL, L"Failed to allocate memory for buf for projects file", L"Error", MB_ICONEXCLAMATION | MB_OK);
			return;
		}

		// don't save project with no pairs
		if (wcslen(current->project.pair.source) > 0 && wcslen(current->project.pair.destination) > 0)
		{
			swprintf(buf, MAX_LINE * 4, L"%s,%s,%s", current->project.name, current->project.pair.source, current->project.pair.destination);
			fwprintf(f, L"%s\n", buf);
		}

		swprintf(buf, MAX_LINE * 4, L"Name: %s, source: %s, dest: %s", current->project.name, current->project.pair.source, current->project.pair.destination);
		logger(buf);

		++count;
		current = current->next;
	}

#if DEV_MODE
	wchar_t buf[100] = {0};
	swprintf(buf, 100, L"Saved %d projects", count);
	logger(buf);
#endif

	fclose(f);
}

// determine if project name by absence of > character
bool isProjectName(wchar_t *text, int len)
{
	for (int i = 0; i < len && text[i] != '\0'; ++i)
		if (text[i] == '>')
			return false;
	return true;
}

// look backwards to find project name from selected pair
void findProjectName(HWND hwnd, LRESULT selectedRow, wchar_t *projectName)
{
	wchar_t selectedRowText[MAX_LINE] = {0};
	bool found = false;

	do
	{
		int textLen = (int)SendMessage(hwnd, LB_GETTEXT, --selectedRow, (LPARAM)selectedRowText);
		if (isProjectName(selectedRowText, textLen))
		{
			wcscpy_s(projectName, MAX_LINE, selectedRowText);
			found = true;
		}
	}
	while (!found && selectedRow >= 0);
}

// reload source & destination folder pair listboxes
void reloadFolderPairs(HWND src, HWND dst, struct ProjectNode *projectsHead, wchar_t *projectName)
{
	int position = 0;
	struct ProjectNode *current = projectsHead;

	while (current != NULL)
	{
		// add all folder pairs from current project
		if (wcscmp(projectName, current->project.name) == 0)
		{
			SendMessage(src, LB_ADDSTRING, position, (LPARAM)current->project.pair.source);
			SendMessage(dst, LB_ADDSTRING, position++, (LPARAM)current->project.pair.destination);
		}
		current = current->next;
	}
}

void splitPair(wchar_t *pair, wchar_t *source, wchar_t *dest, size_t length)
{
	assert(length > 0);

	int pos = 0;
	while (pos < length && pair[pos] != '>')
		++pos;

	// get positions before & after >
	int sourceEnd = pos - 2;
	int destStart = pos + 2;

	// find position of size marker (xx)
	size_t bracketPos = length;
	while (bracketPos > 0 && pair[bracketPos] != '(')
		--bracketPos;
	if (bracketPos > 0)
		length = bracketPos - destStart - 2;

	wcsncpy_s(source, MAX_LINE, pair, sourceEnd);
	wcsncpy_s(dest, MAX_LINE, pair + destStart, length);
}

void startLoggingThread()
{
	HANDLE threads[1];
	DWORD threadIDs[1];
	int initialCount = 0;
	int threadCount = 1;
	loggerSemaphoreHandle = CreateSemaphoreEx(0, initialCount, threadCount, 0, 0, SEMAPHORE_ALL_ACCESS);
	threads[0] = CreateThread(NULL, 0, entryPointLogger, NULL, 0, &threadIDs[0]);

	if (threads[0] == NULL)
	{
		wchar_t buf[MAX_LINE] = { 0 };
		swprintf(buf, MAX_LINE, L"Failed to create logger thread");
		logger(buf);
	}
	else
	{
		logger(L"Test text");
	}
}

static DWORD CALLBACK entryPointLogger(LPVOID arguments)
{
	FILE *f;

	for (;;)
	{
		if (loggerHead != NULL)
		{
			f = fopen(LOG_FILE, "at, ccs=UNICODE");
			if (f == NULL)
			{
				MessageBox(NULL, L"Can't open file", L"Error", MB_ICONEXCLAMATION | MB_OK);
				return 0;
			}

			while (loggerHead != NULL)
			{
				fwprintf(f, L"%s\n", loggerHead->text);
				deleteLoggerNode(&loggerHead);
			}
			fclose(f);
		}
		WaitForSingleObjectEx(loggerSemaphoreHandle, INFINITE, FALSE);
	}

	return 0;
}

// add text to next node for later writing
void logger(wchar_t *text)
{
	appendLoggerNode(&loggerHead, text);
	ReleaseSemaphore(loggerSemaphoreHandle, 1, 0);
}

extern int progressPosition;

void startProgressBarThread(HWND hwnd)
{
	HANDLE threads[1];
	DWORD threadIDs[1];
	int initialCount = 0;
	int threadCount = 1;

	progressSemaphoreHandle = CreateSemaphoreEx(0, initialCount, threadCount, 0, 0, SEMAPHORE_ALL_ACCESS);
	struct Arguments args = { hwnd };
	threads[0] = CreateThread(NULL, 0, entryPointProgressBar, &args, 0, &threadIDs[0]);

	if (threads[0] == NULL)
	{
		wchar_t buf[MAX_LINE] = { 0 };
		swprintf(buf, MAX_LINE, L"Failed to create progress bar thread");
		logger(buf);
	}
	else
	{
		logger(L"Progress bar thread started");
	}
}

static DWORD CALLBACK entryPointProgressBar(LPVOID arguments)
{
	struct Arguments *args = (struct Arguments *)arguments;
	HWND hwnd = args->hwnd;

	for (;;)
	{
		if (progressPosition > 0)
		{
			SendMessage(hwnd, PBM_SETPOS, progressPosition, 0);
			WaitForSingleObjectEx(progressSemaphoreHandle, INFINITE, FALSE);
		}
	}

	return 0;
}

void activateProgressBar()
{
	//
	ReleaseSemaphore(progressSemaphoreHandle, 1, 0);
}
