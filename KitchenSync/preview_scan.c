#include "kitchen_sync.h"

#define DETAIL_MODE 1

static void previewFolderPairSource(HWND, struct PairNode **, struct Project *);
static void previewFolderPairTarget(HWND, struct PairNode **, struct Project *);
static void listTreeContent(struct PairNode **, wchar_t *, wchar_t *);
static void listForRemoval(struct PairNode **, wchar_t *);
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
	struct PairNode **pairs,
	wchar_t selectedRowText[MAX_LINE],
	LRESULT selectedRow)
{
	HANDLE threads[1];
	DWORD threadIDs[1];

	struct PreviewScanArguments args = { pbHwnd, lbSyncHwnd, lbProjectsHwnd, bSync, tabHwnd, head_ref, pairs, selectedRow };
	wcscpy_s(args.selectedRowText, MAX_LINE, selectedRowText);
	threads[0] = CreateThread(NULL, 0, entryPointPreviewScan, &args, 0, &threadIDs[0]);

	if (threads[0] == NULL)
	{
		wchar_t buf[MAX_LINE] = { 0 };
		swprintf(buf, MAX_LINE, L"Failed to create preview scan thread");
		logger(buf);
	}
}

static DWORD CALLBACK entryPointPreviewScan(LPVOID arguments)
{
	struct PreviewScanArguments *args = (struct PreviewScanArguments *)arguments;
	HWND pbHwnd = args->pbHwnd;
	HWND lbSyncHwnd = args->lbSyncHwnd;
	HWND lbProjectsHwnd = args->lbProjectsHwnd;
	HWND bSync = args->bSync;
	HWND tabHwnd = args->tabHwnd;
	struct ProjectNode **project = args->project;
	struct PairNode **pairs = args->pairs;
	LRESULT selectedRow = args->selectedRow;
	wchar_t selectedRowText[MAX_LINE];
	wcscpy_s(selectedRowText, MAX_LINE, args->selectedRowText);
	int textLen = (int)wcslen(selectedRowText);

	assert(textLen > 0);

	if (isProjectName(selectedRowText, textLen))
	{
		// preview whole project
		previewProject(pbHwnd, lbSyncHwnd, project, pairs, selectedRowText);
	}
	else
	{
		// preview folder pair
		struct Project sourceProject = { 0 };
		splitPair(selectedRowText, sourceProject.pair.source, sourceProject.pair.destination, textLen);
		findProjectName(lbProjectsHwnd, selectedRow, sourceProject.name);
		previewFolderPair(pbHwnd, lbSyncHwnd, pairs, &sourceProject);
		SendMessage(lbSyncHwnd, LB_RESETCONTENT, 0, 0);
		sortPairNodes(pairs);
		fillSyncListbox(lbSyncHwnd, pairs);
		SendMessage(pbHwnd, PBM_SETPOS, 100, 0);
	}

	EnableWindow(tabHwnd, true);

	if (countPairNodes(*pairs) > 0)
		EnableWindow(bSync, true);
	else
		EnableWindow(bSync, false);

	return 0;
}

// find each matching folder pair under this project and send it for Preview
void previewProject(HWND pbHwnd, HWND lbSyncHwnd, struct ProjectNode **head_ref, struct PairNode **pairs, wchar_t *projectName)
{
	struct ProjectNode *current = *head_ref;
	int completed = 0;
	int pairCount = countProjectPairs(*head_ref, projectName);
	SendMessage(pbHwnd, PBM_SETPOS, 0, 0);
	do
	{
		if (wcscmp(current->project.name, projectName) == 0)
		{
			struct Project project = { 0 };
			fillInProject(&project, projectName, current->project.pair.source, current->project.pair.destination);
			int progressPosition = ((100 / pairCount) * ++completed) + 1;
			previewFolderPair(pbHwnd, lbSyncHwnd, pairs, &project);
			sortPairNodes(pairs);
			SendMessage(lbSyncHwnd, LB_RESETCONTENT, 0, 0);
			fillSyncListbox(lbSyncHwnd, pairs);
			SendMessage(pbHwnd, PBM_SETPOS, progressPosition, 0);
		}
		current = current->next;
	} while (*head_ref != NULL && current != NULL);
}

// send one specific folder pair for Preview in both directions
void previewFolderPair(HWND pbHwnd, HWND lbSyncHwnd, struct PairNode **pairs, struct Project *project)
{
	// reverse source & destination
	struct Project reversed = { 0 };
	fillInProject(&reversed, project->name, project->pair.destination, project->pair.source);

#if 0
	previewFolderPairSource(lbSyncHwnd, pairs, project);
	previewFolderPairTarget(lbSyncHwnd, pairs, &reversed);
#else
	HANDLE threads[2];
	DWORD threadIDs[2];

	struct PreviewFolderArguments sourceArgs = { lbSyncHwnd, pairs, project };

	threads[0] = CreateThread(NULL, 0, entryPointSource, &sourceArgs, 0, &threadIDs[0]);
	if (threads[0] == NULL)
	{
		wchar_t buf[MAX_LINE] = { 0 };
		swprintf(buf, MAX_LINE, L"Failed to create source->destination thread");
		logger(buf);
		return;
	}

	struct PreviewFolderArguments targetArgs = { lbSyncHwnd, pairs, &reversed };

	threads[1] = CreateThread(NULL, 0, entryPointTarget, &targetArgs, 0, &threadIDs[1]);
	if (threads[1] == NULL)
	{
		wchar_t buf[MAX_LINE] = { 0 };
		swprintf(buf, MAX_LINE, L"Failed to create destination->source thread");
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
	struct PairNode **pairs = args->pairs;
	struct Project *project = args->project;

	previewFolderPairSource(hwnd, pairs, project);
	return 0;
}

DWORD CALLBACK entryPointTarget(LPVOID arguments)
{
	struct PreviewFolderArguments *args = (struct PreviewFolderArguments *)arguments;
	HWND hwnd = args->hwnd;
	struct PairNode **pairs = args->pairs;
	struct Project *project = args->project;

	previewFolderPairTarget(hwnd, pairs, project);
	return 0;
}

// this recursively adds all the files/folders in and below the supplied folders to the pairs list
static void listTreeContent(struct PairNode **pairs, wchar_t *source, wchar_t *destination)
{
	wchar_t szDir[MAX_LINE];
	if (wcslen(source) >= MAX_LINE)
	{
		wchar_t buf[MAX_LINE] = L"Folder path is full, can't add \\";
		logger(buf);
		MessageBox(NULL, buf, L"Error", MB_ICONEXCLAMATION | MB_OK);
		return;
	}
	addPath(szDir, source, L"*");

	WIN32_FIND_DATA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	hFind = FindFirstFile(szDir, &ffd);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		wchar_t buf[MAX_LINE] = { 0 };
		swprintf(buf, MAX_LINE, L"Unable to access %s", source);
		logger(buf);
		displayErrorBox(TEXT("listTreeContent"));
		return;
	}

	do
	{
		if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0)
			continue;

		// combine source path \ new filename
		wchar_t currentItem[MAX_LINE] = { 0 };
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
			wchar_t currentItemSlash[MAX_LINE] = { 0 };
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
			wchar_t destinationSubFolder[MAX_LINE] = { 0 };
			wcscpy_s(destinationSubFolder, MAX_LINE, destination);
			if ((wcslen(destinationSubFolder) + wcslen(currentItem) + 2) >= MAX_LINE)
			{
				wchar_t buf[MAX_LINE] = L"Sub-folder path is full, can't add \\ & folder name";
				logger(buf);
				MessageBox(NULL, buf, L"Error", MB_ICONEXCLAMATION | MB_OK);
				return;
			}
			addPath(destinationSubFolder, destinationSubFolder, ffd.cFileName);

			wchar_t destinationSubFolderSlash[MAX_LINE] = { 0 };
			wcscpy_s(destinationSubFolderSlash, MAX_LINE, destinationSubFolder);
			if (wcslen(destinationSubFolderSlash) + 2 >= MAX_LINE)
			{
				wchar_t buf[MAX_LINE] = L"Sub-folder path is full, can't add \\";
				logger(buf);
				MessageBox(NULL, buf, L"Error", MB_ICONEXCLAMATION | MB_OK);
				return;
			}
			wcscat(destinationSubFolderSlash, L"\\");

			addPair(pairs, currentItemSlash, destinationSubFolderSlash, filesize.QuadPart);
			listTreeContent(pairs, currentItem, destinationSubFolder);
		}
		else
		{
			// add to files list
			wchar_t newDestination[MAX_LINE] = { 0 };
			addPath(newDestination, destination, ffd.cFileName);
			addPair(pairs, currentItem, newDestination, filesize.QuadPart);
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	if (GetLastError() != ERROR_NO_MORE_FILES)
		displayErrorBox(TEXT("listTreeContent"));

	FindClose(hFind);
	return;
}

// this recursively adds all the files/folders in and below the supplied folder to the pairs list for removal
void listForRemoval(struct PairNode **pairs, wchar_t *path)
{
	wchar_t szDir[MAX_LINE];
	if (wcslen(path) >= MAX_LINE)
	{
		wchar_t buf[MAX_LINE] = L"Folder path is full, can't add \\";
		logger(buf);
		MessageBox(NULL, buf, L"Error", MB_ICONEXCLAMATION | MB_OK);
		return;
	}
	addPath(szDir, path, L"*");

	WIN32_FIND_DATA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	hFind = FindFirstFile(szDir, &ffd);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		wchar_t buf[MAX_LINE] = { 0 };
		swprintf(buf, MAX_LINE, L"Unable to access %s", path);
		logger(buf);
		displayErrorBox(TEXT("listForRemoval"));
		return;
	}

	do
	{
		if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0)
			continue;

		// combine path \ new filename
		wchar_t currentItem[MAX_LINE] = { 0 };
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
			wchar_t currentItemSlash[MAX_LINE] = { 0 };
			wcscpy_s(currentItemSlash, MAX_LINE, currentItem);
			wcscat(currentItemSlash, L"\\");

			addPair(pairs, L"Delete folder", currentItemSlash, filesize.QuadPart);
			listForRemoval(pairs, currentItem);
		}
		else
		{
			// add to files list
			addPair(pairs, L"Delete file", currentItem, filesize.QuadPart);
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	if (GetLastError() != ERROR_NO_MORE_FILES)
		displayErrorBox(TEXT("listForRemoval"));

	FindClose(hFind);
	return;
}

static void previewFolderPairSource(HWND hwnd, struct PairNode **pairs, struct Project *project)
{
	wchar_t szDir[MAX_LINE];
	if (wcslen(project->pair.source) >= MAX_LINE)
	{
		wchar_t buf[MAX_LINE] = L"Folder path is full, can't add \\";
		logger(buf);
		MessageBox(NULL, buf, L"Error", MB_ICONEXCLAMATION | MB_OK);
		return;
	}
	addPath(szDir, project->pair.source, L"*");

	WIN32_FIND_DATA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	hFind = FindFirstFile(szDir, &ffd);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		wchar_t buf[MAX_LINE];
		swprintf(buf, MAX_LINE, L"Unable to access %s", szDir);
		logger(buf);
		displayErrorBox(TEXT("previewFolderPairSource"));
		return;
	}

	wchar_t buf[MAX_LINE];
	do
	{
		if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0)
			continue;

		LARGE_INTEGER filesize;
		filesize.LowPart = ffd.nFileSizeLow;
		filesize.HighPart = ffd.nFileSizeHigh;

		wchar_t source[MAX_LINE];
		addPath(source, project->pair.source, ffd.cFileName);

		wchar_t destination[MAX_LINE] = { 0 };
		addPath(destination, project->pair.destination, ffd.cFileName);

		// folder
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			// check if target folder exists. if exists do recursive search in folder
			if (folderExists(destination))
			{
				struct Project destinationSubFolder = { 0 };
				fillInProject(&destinationSubFolder, project->name, source, destination);
#if DETAIL_MODE
	swprintf(buf, MAX_LINE, L"recursive call to previewFolderPairSource() for %s -> %s", source, destination);
	logger(buf);
#endif
				previewFolderPairSource(hwnd, pairs, &destinationSubFolder);
			}
			else // target folder does not exist
			{
#if DETAIL_MODE
	swprintf(buf, MAX_LINE, L"dump entire tree with listTreeContent() for %s -> %s", source, destination);
	logger(buf);
#endif
				// if new, add entire folder contents to list (recursive but without comparison)
				wchar_t sourceSlash[MAX_LINE] = { 0 };
				wcscpy_s(sourceSlash, MAX_LINE, source);
				wcscat(sourceSlash, L"\\");

				wchar_t destinationSlash[MAX_LINE] = { 0 };
				wcscpy_s(destinationSlash, MAX_LINE, destination);
				wcscat(destinationSlash, L"\\");

				addPair(pairs, sourceSlash, destinationSlash, filesize.QuadPart);
				listTreeContent(pairs, source, destination);
			}
		}
		else // item is a file
		{
			// check if target file exists
			if (fileExists(destination))
			{
#if DETAIL_MODE
{
	swprintf(buf, MAX_LINE, L"target file exists, compare date/time and sizes");
	logger(buf);
}
#endif
				LONGLONG targetSize = getFileSize(destination);

				// if different size, add file to list
				if (targetSize != filesize.QuadPart)
				{
#if DETAIL_MODE
	swprintf(buf, MAX_LINE, L"target file is out of date, add %s to list", destination);
	logger(buf);
#endif
					addPair(pairs, source, destination, filesize.QuadPart);
				}
				else // size is the same so test date/time
				{
					// if last write time is different between source and destination, add file to list
					if (fileDateIsDifferent(ffd.ftCreationTime, ffd.ftLastAccessTime, ffd.ftLastWriteTime, destination))
					{
#if DETAIL_MODE
	swprintf(buf, MAX_LINE, L"source & target files have different date/time, add %s to list", destination);
	logger(buf);
#endif
						addPair(pairs, source, destination, filesize.QuadPart);
					}
				}
			}
			else // destination file does not exist
			{
				// add to files list
#if DETAIL_MODE
	swprintf(buf, MAX_LINE, L"target file does not exist, add %s to list", destination);
	logger(buf);
#endif
				addPair(pairs, source, destination, filesize.QuadPart);
			}
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	if (GetLastError() != ERROR_NO_MORE_FILES)
		displayErrorBox(TEXT("previewFolderPairSource"));

	FindClose(hFind);
	return;
}

// reversed folder direction
static void previewFolderPairTarget(HWND hwnd, struct PairNode **pairs, struct Project *project)
{
	wchar_t szDir[MAX_LINE];
	if (wcslen(project->pair.source) >= MAX_LINE)
	{
		wchar_t buf[MAX_LINE] = L"Folder path is full, can't add \\";
		logger(buf);
		MessageBox(NULL, buf, L"Error", MB_ICONEXCLAMATION | MB_OK);
		return;
	}
	addPath(szDir, project->pair.source, L"*");

	WIN32_FIND_DATA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	hFind = FindFirstFile(szDir, &ffd);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		wchar_t buf[MAX_LINE] = { 0 };
		swprintf(buf, MAX_LINE, L"Unable to access %s", szDir);
		logger(buf);
		displayErrorBox(TEXT("previewFolderPairTarget"));
		return;
	}

	do
	{
		if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0)
			continue;

		LARGE_INTEGER filesize;
		filesize.LowPart = ffd.nFileSizeLow;
		filesize.HighPart = ffd.nFileSizeHigh;

		wchar_t source[MAX_LINE];
		addPath(source, project->pair.source, ffd.cFileName);

		wchar_t destination[MAX_LINE] = { 0 };
		addPath(destination, project->pair.destination, ffd.cFileName);

		// folder
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			// check if target folder exists
			// if exists do recursive search in folder
			if (folderExists(destination))
			{
				struct Project destinationSubFolder = { 0 };
				fillInProject(&destinationSubFolder, project->name, source, destination);
#if DETAIL_MODE
	wchar_t buf[MAX_LINE] = { 0 };
	swprintf(buf, MAX_LINE, L"recursive call to previewFolderPairTarget() for %s -> %s", source, destination);
	logger(buf);
#endif
				previewFolderPairTarget(hwnd, pairs, &destinationSubFolder);
			}
			else // target folder does not exist
			{
#if DETAIL_MODE
	wchar_t buf[MAX_LINE] = { 0 };
	swprintf(buf, MAX_LINE, L"Entire folder tree to be removed %s", source);
	logger(buf);
#endif
				wchar_t sourceSlash[MAX_LINE] = { 0 };
				wcscpy_s(sourceSlash, MAX_LINE, source);
				wcscat(sourceSlash, L"\\");

				addPair(pairs, L"Delete folder", sourceSlash, filesize.QuadPart);
				listForRemoval(pairs, source);
			}
		}
		else // item is a file
		{
			// check if target file exists
			if (!fileExists(destination))
			{
				// remove source file
#if DETAIL_MODE
	wchar_t buf[MAX_LINE] = { 0 };
	swprintf(buf, MAX_LINE, L"target file does not exist, add source %s for deletion", source);
	logger(buf);
#endif
				addPair(pairs, L"Delete file", source, filesize.QuadPart);
			}
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	if (GetLastError() != ERROR_NO_MORE_FILES)
		displayErrorBox(TEXT("previewFolderPairTarget"));

	FindClose(hFind);
	return;
}
