#include "kitchen_sync.h"

#define DETAILED_LOGGING 0

static void previewFolderPairSource(HWND, struct Pair **, struct Project *);
static void previewFolderPairTarget(HWND, struct Pair **, struct Project *);
static void listTreeContent(struct Pair **, wchar_t *, wchar_t *);
static void listForRemoval(struct Pair **, wchar_t *);
static DWORD CALLBACK entryPointPreviewScan(LPVOID);
static DWORD CALLBACK entryPointSource(LPVOID);
static DWORD CALLBACK entryPointTarget(LPVOID);

void displayErrorBox(LPTSTR lpszFunction)
{
	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				  NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);

	lpDisplayBuf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (lstrlen((LPCTSTR)lpMsgBuf) + (size_t)lstrlen((LPCTSTR)lpszFunction) + (size_t)40) * sizeof(TCHAR));
	if (lpDisplayBuf)
	{
		swprintf((LPTSTR)lpDisplayBuf, LocalSize(lpDisplayBuf) / sizeof(TCHAR), TEXT("%s failed with error %lu: %s"), lpszFunction, dw, (LPTSTR)lpMsgBuf);
		MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);
		HeapFree(GetProcessHeap(), 0, lpMsgBuf);
		HeapFree(GetProcessHeap(), 0, lpDisplayBuf);
	}
}

void startPreviewScanThread(HWND pbHwnd, HWND lbSyncHwnd, HWND lbProjectsHwnd, HWND bSync, HWND tabHwnd,
							struct ProjectNode **head_ref,
							struct Pair **pairIndex,
							wchar_t selectedRowText[MAX_LINE],
							LRESULT selectedRow)
{
	HANDLE threads[1];
	DWORD threadIDs[1];

	static struct PreviewScanArguments args;
	args.pbHwnd = pbHwnd;
	args.lbSyncHwnd = lbSyncHwnd;
	args.lbProjectsHwnd = lbProjectsHwnd;
	args.bSync = bSync;
	args.tabHwnd = tabHwnd;
	args.project = head_ref;
	args.selectedRow = selectedRow;
	args.pairIndex = pairIndex;
	wcscpy_s(args.selectedRowText, MAX_LINE, selectedRowText);

	threads[0] = CreateThread(NULL, 0, entryPointPreviewScan, &args, 0, &threadIDs[0]);

	if (threads[0] == NULL)
	{
		wchar_t buf[MAX_LINE] = L"Failed to create preview scan thread";
		logger(buf);
	}
}

static DWORD CALLBACK entryPointPreviewScan(LPVOID arguments)
{
	struct PreviewScanArguments *args = (struct PreviewScanArguments *)arguments;
	HWND pbHwnd = args->pbHwnd;
	HWND lbSyncHwnd = args->lbSyncHwnd;
	HWND bSync = args->bSync;
	HWND tabHwnd = args->tabHwnd;
	HWND lbProjectsHwnd = args->lbProjectsHwnd;
	struct ProjectNode **project = args->project;
	struct Pair **pairIndex = args->pairIndex;
	LRESULT selectedRow = args->selectedRow;
	wchar_t selectedRowText[MAX_LINE];
	wcscpy_s(selectedRowText, MAX_LINE, args->selectedRowText);

	int textLen = (int)wcslen(selectedRowText);
	assert(textLen > 0);
	if (textLen <= 0)
	{
		MessageBox(NULL, L"Blank row text", L"Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

#if DEBUG_MODE
	startCount();
	firstCount = beginCycleCount;
	resultCount = 0;
#endif

	if (isProjectName(selectedRowText, textLen))
	{
		// preview whole project
		previewProject(pbHwnd, lbSyncHwnd, bSync, project, pairIndex, selectedRowText);
	}
	else
	{
		// preview one folder pair
		struct Project sourceProject = {0};
		splitPair(selectedRowText, sourceProject.pair.source, sourceProject.pair.destination, textLen);
#if DEBUG_MODE
		startCount();
#endif
		findProjectName(lbProjectsHwnd, selectedRow, sourceProject.name);
#if DEBUG_MODE
		endCount(L"findProjectName");
		startCount();
#endif
		SetWindowText(bSync, L"Scanning...");
		SendMessage(pbHwnd, PBM_SETPOS, 25, 0);
		wchar_t driveList[MAX_LINE] = {0};
		DWORD drivesLength = getDriveStrings(MAX_LINE, driveList);
		previewFolderPair(pbHwnd, lbSyncHwnd, pairIndex, &sourceProject, driveList, drivesLength);
#if DEBUG_MODE
		endCount(L"previewFolderPair");
		startCount();
#endif
		SendMessage(lbSyncHwnd, LB_RESETCONTENT, 0, 0);
		SetWindowText(bSync, L"Sorting...");
		SendMessage(pbHwnd, PBM_SETPOS, 50, 0);
		sortPairs(pairIndex);
#if DEBUG_MODE
		endCount(L"sortPairNodes");
		startCount();
#endif
		SetWindowText(bSync, L"Loading...");
		SendMessage(pbHwnd, PBM_SETPOS, 75, 0);
		fillSyncListbox(lbSyncHwnd, pairIndex);
#if DEBUG_MODE
		endCount(L"fillSyncListbox");
#endif
		SendMessage(pbHwnd, PBM_SETPOS, 100, 0);
	}

#if DEBUG_MODE
	beginCycleCount = firstCount;
	endCount(L"Preview total");
#endif
	EnableWindow(tabHwnd, true);
	SetWindowText(bSync, L"Sync");

	if (pairCount > 0)
		EnableWindow(bSync, true);
	else
		EnableWindow(bSync, false);

	return 0;
}

// find each matching folder pair under this project and send it for Preview
void previewProject(HWND pbHwnd, HWND lbSyncHwnd, HWND bSync, struct ProjectNode **head_ref, struct Pair **pairIndex, wchar_t *projectName)
{
	struct ProjectNode *current = *head_ref;
	if (current == NULL)
	{
		MessageBox(NULL, L"Null head_ref", L"Error", MB_ICONEXCLAMATION | MB_OK);
		return;
	}

	SetWindowText(bSync, L"Scanning...");
	SendMessage(pbHwnd, PBM_SETPOS, 25, 0);
	wchar_t driveList[MAX_LINE] = {0};
	DWORD drivesLength = getDriveStrings(MAX_LINE, driveList);
	do
	{
		if (wcscmp(current->project.name, projectName) == 0)
		{
			struct Project project = {0};
			fillInProject(&project, projectName, current->project.pair.source, current->project.pair.destination);
			previewFolderPair(pbHwnd, lbSyncHwnd, pairIndex, &project, driveList, drivesLength);
		}
		current = current->next;
	} while (*head_ref != NULL && current != NULL);

	SetWindowText(bSync, L"Sorting...");
	SendMessage(pbHwnd, PBM_SETPOS, 50, 0);
	sortPairs(pairIndex);

	SetWindowText(bSync, L"Loading...");
	SendMessage(lbSyncHwnd, LB_RESETCONTENT, 0, 0);
	SendMessage(pbHwnd, PBM_SETPOS, 75, 0);
	fillSyncListbox(lbSyncHwnd, pairIndex);

	SetWindowText(bSync, L"Sync");
	SendMessage(pbHwnd, PBM_SETPOS, 100, 0);
}

// send one specific folder pair for Preview in both directions
void previewFolderPair(HWND pbHwnd, HWND lbSyncHwnd, struct Pair **pairIndex, struct Project *project, wchar_t *driveList, DWORD drivesLength)
{
	// abort if source or destination drive is missing
	wchar_t srcDrive = project->pair.source[0];
	wchar_t dstDrive = project->pair.destination[0];
	bool foundSrc = false;
	bool foundDst = false;

	for (size_t i = 0; i < drivesLength; ++i)
	{
		if (driveList[i] == srcDrive)
			foundSrc = true;
		if (driveList[i] == dstDrive)
			foundDst = true;
		if (foundSrc && foundDst)
			break;
	}

	if (!foundSrc)
	{
		wchar_t buf[MAX_LINE];
		swprintf(buf, MAX_LINE, L"Can't find source drive %c:", srcDrive);
		logger(buf);
		MessageBox(NULL, buf, L"Error", MB_ICONEXCLAMATION | MB_OK);
		return;
	}
	if (!foundDst)
	{
		wchar_t buf[MAX_LINE];
		swprintf(buf, MAX_LINE, L"Can't find destination drive %c:", dstDrive);
		logger(buf);
		MessageBox(NULL, buf, L"Error", MB_ICONEXCLAMATION | MB_OK);
		return;
	}

	// get reversed source & destination
	struct Project reversed = {0};
	fillInProject(&reversed, project->name, project->pair.destination, project->pair.source);

#if 0
	previewFolderPairSource(lbSyncHwnd, pairIndex, project);
	previewFolderPairTarget(lbSyncHwnd, pairIndex, &reversed);
#else
	HANDLE threads[2];
	DWORD threadIDs[2];

	static struct PreviewFolderArguments sourceArgs;
	sourceArgs.hwnd = lbSyncHwnd;
	sourceArgs.pairIndex = pairIndex;
	sourceArgs.project = project;

	threads[0] = CreateThread(NULL, 0, entryPointSource, &sourceArgs, 0, &threadIDs[0]);
	if (threads[0] == NULL)
	{
		wchar_t buf[MAX_LINE] = L"Failed to create source->destination thread";
		logger(buf);
		return;
	}

	static struct PreviewFolderArguments targetArgs;
	targetArgs.hwnd = lbSyncHwnd;
	targetArgs.pairIndex = pairIndex;
	targetArgs.project = &reversed;

	threads[1] = CreateThread(NULL, 0, entryPointTarget, &targetArgs, 0, &threadIDs[1]);
	if (threads[1] == NULL)
	{
		wchar_t buf[MAX_LINE] = L"Failed to create destination->source thread";
		logger(buf);
		return;
	}

	WaitForMultipleObjects(2, threads, TRUE, INFINITE);
#endif
}

DWORD CALLBACK entryPointSource(LPVOID arguments)
{
	struct PreviewFolderArguments *args = (struct PreviewFolderArguments *)arguments;
	HWND hwnd = args->hwnd;
	struct Pair **pairIndex = args->pairIndex;
	struct Project *project = args->project;

	previewFolderPairSource(hwnd, pairIndex, project);
	return 0;
}

DWORD CALLBACK entryPointTarget(LPVOID arguments)
{
	struct PreviewFolderArguments *args = (struct PreviewFolderArguments *)arguments;
	HWND hwnd = args->hwnd;
	struct Pair **pairIndex = args->pairIndex;
	struct Project *project = args->project;

	previewFolderPairTarget(hwnd, pairIndex, project);
	return 0;
}

// this recursively adds all the files/folders in and below the supplied folder to the pairs list
static void listTreeContent(struct Pair **pairIndex, wchar_t *source, wchar_t *destination)
{
	wchar_t currentPath[MAX_LINE];
	if (wcslen(source) >= MAX_LINE)
	{
		wchar_t buf[MAX_LINE] = L"Folder path is full, can't add \\";
		logger(buf);
		MessageBox(NULL, buf, L"Error", MB_ICONEXCLAMATION | MB_OK);
		return;
	}
	addPath(currentPath, source, L"*");

	WIN32_FIND_DATA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	hFind = FindFirstFile(currentPath, &ffd);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		wchar_t buf[MAX_LINE] = {0};
		wcscpy_s(buf, MAX_LINE, L"Unable to access ");
		wcscat(buf, source);
		logger(buf);
		displayErrorBox(TEXT("listTreeContent"));
		return;
	}

	do
	{
		if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0)
			continue;

		if (settings.skipDesktopIni && wcscmp(ffd.cFileName, L"desktop.ini") == 0)
			continue;

		// combine source path \ new filename
		wchar_t currentItem[MAX_LINE] = {0};
		if (wcslen(source) + wcslen(ffd.cFileName) + 2 > MAX_LINE)
		{
			wchar_t buf[MAX_LINE] = L"Folder path is full, can't add filename";
			logger(buf);
			MessageBox(NULL, buf, L"Error", MB_ICONEXCLAMATION | MB_OK);
			return;
		}
		addPath(currentItem, source, ffd.cFileName);

		LARGE_INTEGER filesize;
		filesize.LowPart = ffd.nFileSizeLow;
		filesize.HighPart = ffd.nFileSizeHigh;

		// recursive folder reading
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			wchar_t currentItemSlash[MAX_LINE] = {0};
			wcscpy_s(currentItemSlash, MAX_LINE, currentItem);
			if (wcslen(currentItemSlash) + 2 >= MAX_LINE)
			{
				wchar_t buf[MAX_LINE] = L"Sub-folder path is full, can't add \\";
				logger(buf);
				MessageBox(NULL, buf, L"Error", MB_ICONEXCLAMATION | MB_OK);
				return;
			}
			wcscat(currentItemSlash, L"\\");

			// make new destination subfolder path
			wchar_t destinationSubFolder[MAX_LINE] = {0};
			wcscpy_s(destinationSubFolder, MAX_LINE, destination);
			if ((wcslen(destinationSubFolder) + wcslen(currentItem) + 2) >= MAX_LINE)
			{
				wchar_t buf[MAX_LINE] = L"Sub-folder path is full, can't add \\ & folder name";
				logger(buf);
				MessageBox(NULL, buf, L"Error", MB_ICONEXCLAMATION | MB_OK);
				return;
			}
			addPath(destinationSubFolder, destinationSubFolder, ffd.cFileName);

			wchar_t destinationSubFolderSlash[MAX_LINE] = {0};
			wcscpy_s(destinationSubFolderSlash, MAX_LINE, destinationSubFolder);
			if (wcslen(destinationSubFolderSlash) + 2 >= MAX_LINE)
			{
				wchar_t buf[MAX_LINE] = L"Sub-folder path is full, can't add \\";
				logger(buf);
				MessageBox(NULL, buf, L"Error", MB_ICONEXCLAMATION | MB_OK);
				return;
			}
			wcscat(destinationSubFolderSlash, L"\\");

			addPair(pairIndex, currentItemSlash, destinationSubFolderSlash, filesize.QuadPart);
			listTreeContent(pairIndex, currentItem, destinationSubFolder);
		}
		else // item is a file
		{
			wchar_t newDestination[MAX_LINE] = {0};
			addPath(newDestination, destination, ffd.cFileName);
			addPair(pairIndex, currentItem, newDestination, filesize.QuadPart);
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	if (GetLastError() != ERROR_NO_MORE_FILES)
		displayErrorBox(TEXT("listTreeContent"));

	FindClose(hFind);
	return;
}

// this recursively adds all the files/folders in and below the supplied folder to the pairs list for removal
void listForRemoval(struct Pair **pairIndex, wchar_t *path)
{
	wchar_t currentPath[MAX_LINE];
	if (wcslen(path) >= MAX_LINE)
	{
		wchar_t buf[MAX_LINE] = L"Folder path is full, can't add \\";
		logger(buf);
		MessageBox(NULL, buf, L"Error", MB_ICONEXCLAMATION | MB_OK);
		return;
	}
	addPath(currentPath, path, L"*");

	WIN32_FIND_DATA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	hFind = FindFirstFile(currentPath, &ffd);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		wchar_t buf[MAX_LINE] = {0};
		wcscpy_s(buf, MAX_LINE, L"Unable to access ");
		wcscat(buf, path);
		logger(buf);
		displayErrorBox(TEXT("listForRemoval"));
		return;
	}

	do
	{
		if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0)
			continue;

		if (settings.skipDesktopIni && wcscmp(ffd.cFileName, L"desktop.ini") == 0)
			continue;

		// combine path \ new filename
		wchar_t currentItem[MAX_LINE] = {0};
		if (wcslen(path) + wcslen(ffd.cFileName) + 2 > MAX_LINE)
		{
			wchar_t buf[MAX_LINE] = L"Folder path is full, can't add filename";
			logger(buf);
			MessageBox(NULL, buf, L"Error", MB_ICONEXCLAMATION | MB_OK);
			return;
		}
		addPath(currentItem, path, ffd.cFileName);

		LARGE_INTEGER filesize;
		filesize.LowPart = ffd.nFileSizeLow;
		filesize.HighPart = ffd.nFileSizeHigh;

		// recursive folder reading
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			wchar_t currentItemSlash[MAX_LINE] = {0};
			wcscpy_s(currentItemSlash, MAX_LINE, currentItem);
			wcscat(currentItemSlash, L"\\");

			addPair(pairIndex, L"Delete folder", currentItemSlash, -filesize.QuadPart);
			listForRemoval(pairIndex, currentItem);
		}
		else // item is a file
		{
			addPair(pairIndex, L"Delete file", currentItem, -filesize.QuadPart);
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	if (GetLastError() != ERROR_NO_MORE_FILES)
		displayErrorBox(TEXT("listForRemoval"));

	FindClose(hFind);
	return;
}

static void previewFolderPairSource(HWND hwnd, struct Pair **pairIndex, struct Project *project)
{
	wchar_t currentPath[MAX_LINE];
	if (wcslen(project->pair.source) >= MAX_LINE)
	{
		wchar_t buf[MAX_LINE] = L"Folder path is full, can't add \\";
		logger(buf);
		MessageBox(NULL, buf, L"Error", MB_ICONEXCLAMATION | MB_OK);
		return;
	}
	addPath(currentPath, project->pair.source, L"*");

	static WIN32_FIND_DATA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	hFind = FindFirstFile(currentPath, &ffd);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		wchar_t buf[MAX_LINE];
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
		{
			if (settings.skipSymbolicLinks)
			{
				wcscpy_s(buf, MAX_LINE, L"Skipped symbolic link to ");
				wcscat(buf, currentPath);
				logger(buf);
				return;
			}
			else
			{
				wcscpy_s(buf, MAX_LINE, L"Detected symbolic link ");
				wcscat(buf, currentPath);
				logger(buf);
			}
		}

		wcscpy_s(buf, MAX_LINE, L"Unable to access ");
		wcscat(buf, currentPath);
		logger(buf);
		displayErrorBox(TEXT("previewFolderPairSource"));
		return;
	}

#if DETAILED_LOGGING
	wchar_t buf[MAX_LINE];
#endif
	do
	{
		if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0)
			continue;

		if (settings.skipDesktopIni && wcscmp(ffd.cFileName, L"desktop.ini") == 0)
			continue;

		LARGE_INTEGER filesize;
		filesize.LowPart = ffd.nFileSizeLow;
		filesize.HighPart = ffd.nFileSizeHigh;

		wchar_t source[MAX_LINE];
		addPath(source, project->pair.source, ffd.cFileName);

		wchar_t destination[MAX_LINE];
		addPath(destination, project->pair.destination, ffd.cFileName);

		// folder
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			// check if target folder exists. if exists do recursive search in folder
			if (folderExists(destination))
			{
				struct Project destinationSubFolder = {0};
				fillInProject(&destinationSubFolder, project->name, source, destination);
#if DETAILED_LOGGING
				swprintf(buf, MAX_LINE, L"recursive call to previewFolderPairSource() for %s -> %s", source, destination);
				logger(buf);
#endif
				previewFolderPairSource(hwnd, pairIndex, &destinationSubFolder);
			}
			else // target folder does not exist
			{
#if DETAILED_LOGGING
				swprintf(buf, MAX_LINE, L"dump entire tree with listTreeContent() for %s -> %s", source, destination);
				logger(buf);
#endif
				// if new, add entire folder contents to list (recursive but without comparison)
				wchar_t sourceSlash[MAX_LINE] = {0};
				wcscpy_s(sourceSlash, MAX_LINE, source);
				wcscat(sourceSlash, L"\\");

				wchar_t destinationSlash[MAX_LINE] = {0};
				wcscpy_s(destinationSlash, MAX_LINE, destination);
				wcscat(destinationSlash, L"\\");

				addPair(pairIndex, sourceSlash, destinationSlash, filesize.QuadPart);
				listTreeContent(pairIndex, source, destination);
			}
			continue;
		}

		// item is a file
		if (!fileExists(destination))
		{
#if DETAILED_LOGGING
			swprintf(buf, MAX_LINE, L"target file does not exist, add %s to list", destination);
			logger(buf);
#endif
			addPair(pairIndex, source, destination, filesize.QuadPart);
			continue;
		}
#if DETAILED_LOGGING
		{
			swprintf(buf, MAX_LINE, L"target file exists, compare date/time and sizes");
			logger(buf);
		}
#endif
		LONGLONG targetSize = getFileSize(destination);

		// if different size, add file to list
		if (targetSize != filesize.QuadPart)
		{
#if DETAILED_LOGGING
			swprintf(buf, MAX_LINE, L"target file is out of date, add %s to list", destination);
			logger(buf);
#endif
			addPair(pairIndex, source, destination, filesize.QuadPart);
			continue;
		}

		// size is the same so test date/time
		// if last write time is different between source and destination, add file to list
		if (fileDateIsDifferent(ffd.ftCreationTime, ffd.ftLastAccessTime, ffd.ftLastWriteTime, destination))
		{
#if DETAILED_LOGGING
			swprintf(buf, MAX_LINE, L"source & target files have different date/time, add %s to list", destination);
			logger(buf);
#endif
			addPair(pairIndex, source, destination, filesize.QuadPart);
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	if (GetLastError() != ERROR_NO_MORE_FILES)
		displayErrorBox(TEXT("previewFolderPairSource"));

	FindClose(hFind);
	return;
}

// reversed folder direction
static void previewFolderPairTarget(HWND hwnd, struct Pair **pairIndex, struct Project *project)
{
	wchar_t currentPath[MAX_LINE];
	if (wcslen(project->pair.source) >= MAX_LINE)
	{
		wchar_t buf[MAX_LINE] = L"Folder path is full, can't add \\";
		logger(buf);
		MessageBox(NULL, buf, L"Error", MB_ICONEXCLAMATION | MB_OK);
		return;
	}
	addPath(currentPath, project->pair.source, L"*");

	WIN32_FIND_DATA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	hFind = FindFirstFile(currentPath, &ffd);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		wchar_t buf[MAX_LINE] = {0};
		wcscpy_s(buf, MAX_LINE, L"Unable to access ");
		wcscat(buf, currentPath);
		logger(buf);
		displayErrorBox(TEXT("previewFolderPairTarget"));
		return;
	}

	do
	{
		if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0)
			continue;

		if (settings.skipDesktopIni && wcscmp(ffd.cFileName, L"desktop.ini") == 0)
			continue;

		LARGE_INTEGER filesize;
		filesize.LowPart = ffd.nFileSizeLow;
		filesize.HighPart = ffd.nFileSizeHigh;

		wchar_t source[MAX_LINE];
		addPath(source, project->pair.source, ffd.cFileName);

		wchar_t destination[MAX_LINE];
		addPath(destination, project->pair.destination, ffd.cFileName);

		// folder
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			// if target folder exists do recursive search in folder
			if (folderExists(destination))
			{
				struct Project destinationSubFolder = {0};
				fillInProject(&destinationSubFolder, project->name, source, destination);
#if DETAILED_LOGGING
				wchar_t buf[MAX_LINE] = {0};
				swprintf(buf, MAX_LINE, L"recursive call to previewFolderPairTarget() for %s -> %s", source, destination);
				logger(buf);
#endif
				previewFolderPairTarget(hwnd, pairIndex, &destinationSubFolder);
			}
			else // target folder does not exist
			{
#if DETAILED_LOGGING
				wchar_t buf[MAX_LINE] = {0};
				swprintf(buf, MAX_LINE, L"Entire folder tree to be removed %s", source);
				logger(buf);
#endif
				wchar_t sourceSlash[MAX_LINE] = {0};
				wcscpy_s(sourceSlash, MAX_LINE, source);
				wcscat(sourceSlash, L"\\");

				addPair(pairIndex, L"Delete folder", sourceSlash, -filesize.QuadPart);
				listForRemoval(pairIndex, source);
			}
			continue;
		}

		// item is a file
		if (!fileExists(destination))
		{
			// delete source file
#if DETAILED_LOGGING
			wchar_t buf[MAX_LINE] = {0};
			swprintf(buf, MAX_LINE, L"target file does not exist, add source %s for deletion", source);
			logger(buf);
#endif
			addPair(pairIndex, L"Delete file", source, -filesize.QuadPart);
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	if (GetLastError() != ERROR_NO_MORE_FILES)
		displayErrorBox(TEXT("previewFolderPairTarget"));

	FindClose(hFind);
	return;
}
