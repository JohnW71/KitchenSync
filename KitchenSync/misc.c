#include "kitchen_sync.h"

static void sizeFormatted(LONGLONG, wchar_t *);
static void clampWindowPosition(int *, int *, int *, int *);
static void compressPairs(struct Pair **);

void startCount()
{
#if DEBUG_MODE
	beginCycleCount = __rdtsc();
#endif
}

void endCount(wchar_t *text)
{
#if DEBUG_MODE
	endCycleCount = __rdtsc();
	uint64_t cyclesElapsed = endCycleCount - beginCycleCount;

	results[resultCount++] = cyclesElapsed;

	wchar_t buf[256];
	swprintf(buf, 256, L"%-25s: %14llu cycles", text, cyclesElapsed);
	logger(buf);
#endif
}

void shutDown(HWND hwnd, struct ProjectNode **head_ref)
{
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
		wchar_t buf[MAX_LINE] = L"Error saving settings!";
		logger(buf);
		MessageBox(NULL, buf, L"Error", MB_ICONEXCLAMATION | MB_OK);
		return;
	}

	RECT rc = {0};
	GetWindowRect(hwnd, &rc);
	int windowHeight = rc.bottom - rc.top;
	int windowWidth = rc.right - rc.left;

	int windowCol = rc.left;
	int windowRow = rc.top;

	fprintf(f, "window_row=%d\n", windowRow);
	fprintf(f, "window_col=%d\n", windowCol);
	fprintf(f, "window_height=%d\n", windowHeight);
	fprintf(f, "window_width=%d\n", windowWidth);
	fprintf(f, "skip_desktop_ini=%d\n", settings.skipDesktopIni);
	fprintf(f, "skip_symbolic_links=%d\n", settings.skipSymbolicLinks);
	fprintf(f, "skip_small_differences=%d\n", settings.skipSmallDifferences);
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

	settings.skipDesktopIni = true;
	settings.skipSymbolicLinks = false;
	settings.skipSmallDifferences = false;
	int windowHeight = 0;
	int windowWidth = 0;
	int windowCol = 0;
	int windowRow = 0;

	char *line = (char *)malloc(MAX_LINE);
	if (!line)
	{
		wchar_t buf[MAX_LINE] = L"Failed to allocate memory for line from settings file";
		logger(buf);
		MessageBox(NULL, buf, L"Error", MB_ICONEXCLAMATION | MB_OK);
		fclose(f);
		return;
	}

	char *setting = (char *)calloc(MAX_LINE, sizeof(char));
	if (!setting)
	{
		wchar_t buf[MAX_LINE] = L"Failed to allocate memory for setting from settings file";
		logger(buf);
		MessageBox(NULL, buf, L"Error", MB_ICONEXCLAMATION | MB_OK);
		fclose(f);
		free(line);
		return;
	}

	char *value = (char *)calloc(MAX_LINE, sizeof(char));
	if (!value)
	{
		wchar_t buf[MAX_LINE] = L"Failed to allocate memory for value from settings file";
		logger(buf);
		MessageBox(NULL, buf, L"Error", MB_ICONEXCLAMATION | MB_OK);
		fclose(f);
		free(line);
		free(setting);
		return;
	}

	while (fgets(line, MAX_LINE, f) != NULL)
	{
		if (line[0] == '#')
			continue;

		memset(setting, '\0', MAX_LINE);
		memset(value, '\0', MAX_LINE);

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

		if (strcmp(setting, "window_row") == 0)				windowRow = atoi(value);
		if (strcmp(setting, "window_col") == 0)				windowCol = atoi(value);
		if (strcmp(setting, "window_height") == 0)			windowHeight = atoi(value);
		if (strcmp(setting, "window_width") == 0)			windowWidth = atoi(value);
		if (strcmp(setting, "skip_desktop_ini") == 0)		settings.skipDesktopIni = atoi(value);
		if (strcmp(setting, "skip_symbolic_links") == 0)	settings.skipSymbolicLinks = atoi(value);
		if (strcmp(setting, "skip_small_differences") == 0)	settings.skipSmallDifferences = atoi(value);
	}

	fclose(f);
	free(line);
	free(setting);
	free(value);

	clampWindowPosition(&windowCol, &windowRow, &windowHeight, &windowWidth);
	SetWindowPos(hwnd, HWND_TOP, windowCol, windowRow, windowWidth, windowHeight, SWP_SHOWWINDOW);
}

static void clampWindowPosition(int *windowCol, int *windowRow, int *windowHeight, int *windowWidth)
{
	if (*windowHeight == 0)
		*windowHeight = WINDOW_HEIGHT;

	if (*windowWidth == 0)
		*windowWidth = WINDOW_WIDTH;

	if (*windowRow == 0) // center by default
	{
		int screenHeight = GetSystemMetrics(SM_CYSCREEN);
		*windowRow = (screenHeight - *windowHeight) / 2;
	}

	if (*windowCol == 0) // center by default
	{
		int screenWidth = GetSystemMetrics(SM_CXSCREEN);
		*windowCol = (screenWidth - WINDOW_WIDTH) / 2;
		return;
	}

	//int monitorCount = GetSystemMetrics(SM_CMONITORS);
	int displayWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	int oneDisplay = GetSystemMetrics(SM_CXSCREEN);
	int leftEdge = GetSystemMetrics(SM_XVIRTUALSCREEN);
	int rightEdge = displayWidth - abs(leftEdge);

	if (*windowCol < leftEdge)
		*windowCol = (oneDisplay - abs(*windowCol));

	if (*windowCol > rightEdge)
		*windowCol = *windowCol % oneDisplay;
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
		wchar_t buf[MAX_LINE] = L"Failed to allocate memory for line from projects file";
		logger(buf);
		MessageBox(NULL, buf, L"Error", MB_ICONEXCLAMATION | MB_OK);
		fclose(f);
		return;
	}

	wchar_t *name = (wchar_t *)calloc(MAX_LINE, sizeof(wchar_t));
	if (!name)
	{
		wchar_t buf[MAX_LINE] = L"Failed to allocate memory for name from projects file";
		logger(buf);
		MessageBox(NULL, buf, L"Error", MB_ICONEXCLAMATION | MB_OK);
		fclose(f);
		free(line);
		return;
	}

	wchar_t *source = (wchar_t *)calloc(MAX_LINE, sizeof(wchar_t));
	if (!source)
	{
		wchar_t buf[MAX_LINE] = L"Failed to allocate memory for source from projects file";
		logger(buf);
		MessageBox(NULL, buf, L"Error", MB_ICONEXCLAMATION | MB_OK);
		fclose(f);
		free(line);
		free(name);
		return;
	}

	wchar_t *destination = (wchar_t *)calloc(MAX_LINE, sizeof(wchar_t));
	if (!destination)
	{
		wchar_t buf[MAX_LINE] = L"Failed to allocate memory for destination from projects file";
		logger(buf);
		MessageBox(NULL, buf, L"Error", MB_ICONEXCLAMATION | MB_OK);
		fclose(f);
		free(line);
		free(name);
		free(source);
		return;
	}

	while (fgetws(line, MAX_LINE * 2, f) != NULL)
	{
		if (line[0] == '#' || line[0] == '\n' || line[0] == '\0')
			continue;

		memset(name, '\0', MAX_LINE);
		memset(source, '\0', MAX_LINE);
		memset(destination, '\0', MAX_LINE);

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
			wchar_t buff[MAX_LINE] = L"Failed to allocate memory for logging buf";
			logger(buff);
			MessageBox(NULL, buff, L"Error", MB_ICONEXCLAMATION | MB_OK);
			fclose(f);
			free(line);
			free(name);
			free(source);
			free(destination);
			return;
		}

		wcscpy_s(buf, MAX_LINE * 4, L"Loaded project: ");
		wcscat(buf, name);
		wcscat(buf, L", source: ");
		wcscat(buf, source);
		wcscat(buf, L",dest: ");
		wcscat(buf, destination);
		logger(buf);
		free(buf);

		appendProjectNode(head_ref, name, source, destination);
	}
#if DEBUG_MODE
	wchar_t buf[100] = {0};
	swprintf(buf, 100, L"Loaded %d projects", countProjectNodes(*head_ref));
	logger(buf);
#endif

	fclose(f);
	free(line);
	free(name);
	free(source);
	free(destination);
	sortProjectNodes(head_ref);
	fillProjectListbox(hwnd, head_ref);
}

// load all project nodes into Projects listbox
void fillProjectListbox(HWND hwnd, struct ProjectNode **head_ref)
{
	wchar_t currentProjectName[MAX_LINE] = {0};
	int position = 0;
	struct ProjectNode *current = *head_ref;

	wchar_t *buffer = (wchar_t *)calloc(FOLDER_PAIR_SIZE, sizeof(wchar_t));
	if (!buffer)
	{
		wchar_t buf[MAX_LINE] = L"Failed to allocate memory for folder pair buffer from projects file";
		logger(buf);
		MessageBox(NULL, buf, L"Error", MB_ICONEXCLAMATION | MB_OK);
		return;
	}

	while (current != NULL)
	{
		// add folder pair into a new or existing project section
		if (wcscmp(currentProjectName, current->project.name) != 0) // new project section
		{
			// add blank line between each project
			if (position > 0)
				SendMessage(hwnd, LB_ADDSTRING, position++, (LPARAM)"");

			SendMessage(hwnd, LB_ADDSTRING, position++, (LPARAM)current->project.name);
		}

		wcscpy_s(buffer, FOLDER_PAIR_SIZE, current->project.pair.source);
		wcscat(buffer, L" -> ");
		wcscat(buffer, current->project.pair.destination);
		SendMessage(hwnd, LB_ADDSTRING, position++, (LPARAM)buffer);
		wcscpy_s(currentProjectName, MAX_LINE, current->project.name);
		current = current->next;
	}
	free(buffer);
}

// load all file pair nodes into Sync listbox
void fillSyncListbox(HWND hwnd, struct Pair **pairIndex)
{
	compressPairs(pairIndex);
	int position = 0;

	struct DriveSpace
	{
		LONGLONG requiredSpace;
		bool used;
	};
	struct DriveSpace driveSpace[26] = {0};

	// NOTE this is the slow part
	// Add every pair to listbox
	for (int i = 0; i < pairCount; ++i)
	{
		struct Pair *pair = pairIndex[i];
		if (pair == NULL)
			continue;

		wchar_t *buffer = (wchar_t *)calloc(FOLDER_PAIR_SIZE, sizeof(wchar_t));
		if (!buffer)
		{
			wchar_t buf[MAX_LINE] = L"Failed to allocate memory for file pair buffer from preview list";
			logger(buf);
			MessageBox(NULL, buf, L"Error", MB_ICONEXCLAMATION | MB_OK);
			return;
		}

		LONGLONG size = pair->filesize;
		int drivePosition = ((int)(toupper(pair->destination[0])) - 65);

		if (drivePosition >= 0 && drivePosition <= 25)
		{
			driveSpace[drivePosition].requiredSpace += size;
			driveSpace[drivePosition].used = true;
		}
		else
			logger(L"drivePosition outside expected limits");

		wchar_t formatted[MAX_LINE] = {0};
		sizeFormatted(size, formatted);
		wcscpy_s(buffer, FOLDER_PAIR_SIZE, pair->source);
		wcscat(buffer, L" -> ");
		wcscat(buffer, pair->destination);
		wcscat(buffer, L"  (");
		wcscat(buffer, formatted);
		wcscat(buffer, L")");
		SendMessage(hwnd, LB_ADDSTRING, position++, (LPARAM)buffer);
		free(buffer);
	}

	SendMessage(hwnd, LB_ADDSTRING, position++, (LPARAM)L"");

	wchar_t pairCountFormatted[MAX_LINE] = {0};
	sizeFormatted(pairCount, pairCountFormatted);

	// Display every drive's summary
	for (int i = 0; i < 26; ++i)
		if (driveSpace[i].used)
		{
			wchar_t required[MAX_LINE] = {0};
			sizeFormatted(driveSpace[i].requiredSpace, required);

			wchar_t available[MAX_LINE] = {0};
			LONGLONG availableSpace = getDriveSpace(i);
			sizeFormatted(availableSpace, available);

			wchar_t result[MAX_LINE] = {0};
			if (availableSpace > driveSpace[i].requiredSpace)
				wcscpy_s(result, MAX_LINE, L"OK");
			else
				wcscpy_s(result, MAX_LINE, L"Not enough space!");

			wchar_t summary[MAX_LINE] = {0};
			summary[0] = (wchar_t)(i + 65);
			wcscat(summary, L": space required ");
			wcscat(summary, required);
			wcscat(summary, L" available ");
			wcscat(summary, available);
			wcscat(summary, L" pair count ");
			wcscat(summary, pairCountFormatted);
			wcscat(summary, L"... ");
			wcscat(summary, result);
			SendMessage(hwnd, LB_ADDSTRING, position, (LPARAM)summary);
		}
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

		if (++count == 3 && length > 0 && *t != '-')
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

	wchar_t *buf = (wchar_t *)calloc(MAX_LINE * 4, sizeof(wchar_t));
	if (!buf)
	{
		wchar_t buff[MAX_LINE] = L"Failed to allocate memory for buf for projects file";
		logger(buff);
		MessageBox(NULL, buff, L"Error", MB_ICONEXCLAMATION | MB_OK);
		fclose(f);
		return;
	}
	while (current != NULL)
	{
		// don't save project with no pairs
		if (wcslen(current->project.pair.source) > 0 && wcslen(current->project.pair.destination) > 0)
		{
			wcscpy_s(buf, MAX_LINE * 4, current->project.name);
			wcscat(buf, L",");
			wcscat(buf, current->project.pair.source);
			wcscat(buf, L",");
			wcscat(buf, current->project.pair.destination);
			fwprintf(f, L"%s\n", buf);
		}

		wcscpy_s(buf, MAX_LINE * 4, L"Saved project: ");
		wcscat(buf, current->project.name);
		wcscat(buf, L", source: ");
		wcscat(buf, current->project.pair.source);
		wcscat(buf, L", dest: ");
		wcscat(buf, current->project.pair.destination);
		logger(buf);

		++count;
		current = current->next;
	}
#if DEBUG_MODE
	wchar_t buff[100] = {0};
	swprintf(buff, 100, L"Saved %d projects", count);
	logger(buff);
#endif

	fclose(f);
	free(buf);
}

// determine if text is a project name by absence of > character
bool isProjectName(wchar_t *text, int len)
{
	for (int i = 0; i < len && text[i] != '\0'; ++i)
		if (text[i] == '>')
			return false;
	return true;
}

// search backwards to find project name from selected pair
void findProjectName(HWND hwnd, LRESULT selectedRow, wchar_t *projectName)
{
	wchar_t selectedRowText[MAX_LINE] = {0};

	do
	{
		int textLen = (int)SendMessage(hwnd, LB_GETTEXT, --selectedRow, (LPARAM)selectedRowText);
		if (isProjectName(selectedRowText, textLen))
		{
			wcscpy_s(projectName, MAX_LINE, selectedRowText);
			return;
		}
	} while (selectedRow >= 0);
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

void fillInProject(struct Project *project, wchar_t *name, wchar_t *source, wchar_t *destination)
{
	wcscpy_s(project->name, MAX_LINE, name);
	wcscpy_s(project->pair.source, MAX_LINE, source);
	wcscpy_s(project->pair.destination, MAX_LINE, destination);
}

void deleteAllPairs(struct Pair **pairIndex)
{
	for (int i = 0; i < pairCount; ++i)
		if (pairIndex[i] != NULL)
		{
			free(pairIndex[i]);
			pairIndex[i] = NULL;
		}

	pairCount = 0;
	memset(pairIndex, '\0', sizeof(struct Pair *) * MAX_PAIRS);
}

void deleteFilePair(struct Pair **pairIndex, wchar_t *filePair)
{
	size_t length = wcslen(filePair);
	assert(length > 0);

	if (length > 0)
	{
		wchar_t src[MAX_LINE] = {0};
		wchar_t dst[MAX_LINE] = {0};
		splitPair(filePair, src, dst, length);

		int position = findPair(pairIndex, src, dst);
		if (position >= 0)
		{
			free(pairIndex[position]);
			pairIndex[position] = NULL;
		}
		else
			logger(L"Failed to delete pair, position wasn't found");
	}
}

int findPair(struct Pair **pairIndex, wchar_t *src, wchar_t *dst)
{
	for (int i = 0; i < pairCount; ++i)
	{
		struct Pair *pair = pairIndex[i];
		if (pair == NULL)
			continue;

		if (wcscmp(pair->source, src) == 0 && wcscmp(pair->destination, dst) == 0)
			return i;
	}

	return -1;
}

void sortPairs(struct Pair **pairIndex)
{
	bool changed;
	struct Pair *current;
	struct Pair *next;

	do
	{
		int i = 0;
		changed = false;

		while (i < pairCount - 1)
		{
			current = (struct Pair *)pairIndex[i];
			next = (struct Pair *)pairIndex[i + 1];

			if (wcscmp(current->source, next->source) > 0 ||
				(wcscmp(current->source, next->source) == 0 &&
				 wcscmp(current->destination, next->destination) > 0))
			{
				struct Pair *tmp = pairIndex[i];
				pairIndex[i] = pairIndex[i + 1];
				pairIndex[i + 1] = tmp;

				changed = true;
			}
			++i;
		}
	} while (changed);
}

void addPair(struct Pair **pairIndex, wchar_t *source, wchar_t *destination, LONGLONG size)
{
	struct Pair *pair = (struct Pair *)malloc(sizeof(struct Pair));
	if (!pair)
	{
		wchar_t buf[MAX_LINE] = L"Failed to allocate memory for new pair";
		logger(buf);
		MessageBox(NULL, buf, L"Error", MB_ICONEXCLAMATION | MB_OK);
		return;
	}

	wcscpy_s(pair->source, MAX_LINE, source);
	wcscpy_s(pair->destination, MAX_LINE, destination);
	pair->filesize = size;

	pairIndex[pairCount] = pair;
	++pairCount;
}

void compressPairs(struct Pair **pairIndex)
{
	int newCount = 0;

	for (int i = 0; i < pairCount; ++i)
	{
		struct Pair *pair = pairIndex[i];

		if (pair == NULL)
			continue;

		pairIndex[newCount++] = pairIndex[i];
	}

	pairCount = newCount;
}
