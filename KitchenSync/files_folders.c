#include "kitchen_sync.h"

static bool fileExists(wchar_t *);
static bool folderExists(wchar_t *);
static bool fileDateIsDifferent(FILETIME, FILETIME, FILETIME, wchar_t *);
static LONGLONG getFileSize(wchar_t *);
static void displayErrorBox(LPTSTR);
static void catPath(wchar_t *, wchar_t *, wchar_t *);
static int previewFolderPairSourceTest(HWND, struct PairNode **, struct Project *);
static int previewFolderPairTargetTest(HWND, struct PairNode **, struct Project *);

static void displayErrorBox(LPTSTR lpszFunction)
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

// list the folder names within the supplied folder level only to the specified listbox
int listSubFolders(HWND hwnd, wchar_t *folder)
{
	wchar_t szDir[MAX_LINE];
	if (wcslen(folder) >= MAX_LINE)
	{
		writeFileW(LOG_FILE, L"Folder path is full, can't add \\");
		MessageBox(NULL, L"Folder path is full, can't add \\", L"Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}
	catPath(szDir, folder, L"*");

	DWORD dwError = 0;
	WIN32_FIND_DATA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	hFind = FindFirstFile(szDir, &ffd);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		displayErrorBox(TEXT("listSubFolders"));
		return dwError;
	}

	int position = 0;
	do
	{
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (wcscmp(ffd.cFileName, L".") == 0)
				continue;

#if DEV_MODE
	wchar_t buf[MAX_LINE] = { 0 };
	swprintf(buf, MAX_LINE, L"LSF() Dir: %s", ffd.cFileName);
	writeFileW(LOG_FILE, buf);
#endif
			SendMessage(hwnd, LB_ADDSTRING, position++, (LPARAM)ffd.cFileName);
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	dwError = GetLastError();
	if (dwError != ERROR_NO_MORE_FILES)
		displayErrorBox(TEXT("listSubFolders"));

	FindClose(hFind);
	return dwError;
}

// find each matching folder pair under this project and send it for Preview
void previewProject(HWND hwnd, struct ProjectNode **head_ref, struct PairNode **pairs, wchar_t *projectName)
{
	struct ProjectNode *current = *head_ref;
	do
	{
		if (wcscmp(current->project.name, projectName) == 0)
		{
			struct Project project = {0};
			wcscpy_s(project.name, MAX_LINE, projectName);
			wcscpy_s(project.pair.source, MAX_LINE, current->project.pair.source);
			wcscpy_s(project.pair.destination, MAX_LINE, current->project.pair.destination);

			//previewFolderPair(hwnd, pairs, project);
			//listTreeContent(hwnd, pairs, project.pair.source);
			previewFolderPairTest(hwnd, pairs, &project);
		}
		current = current->next;
	}
	while (*head_ref != NULL && current != NULL);
}

// void previewFolderPair(HWND hwnd, struct PairNode **pairs, struct Project project)
// {
// 	listTreeContent(hwnd, pairs, project.pair.source);
// }

// send one specific folder pair for Preview
void previewFolderPairTest(HWND hwnd, struct PairNode **pairs, struct Project *project)
{
	previewFolderPairSourceTest(hwnd, pairs, project);
	previewFolderPairTargetTest(hwnd, pairs, project);
}

// this adds all the files/folders in and below the supplied folder to the pairs list
//NOTE hwnd parameter is not currently used
int listTreeContent(HWND hwnd, struct PairNode **pairs, wchar_t *source, wchar_t *destination)
{
	wchar_t szDir[MAX_LINE];
	if (wcslen(source) >= MAX_LINE)
	{
		writeFileW(LOG_FILE, L"Folder path is full, can't add \\");
		MessageBox(NULL, L"Folder path is full, can't add \\", L"Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}
	catPath(szDir, source, L"*");

	DWORD dwError = 0;
	WIN32_FIND_DATA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	hFind = FindFirstFile(szDir, &ffd);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		displayErrorBox(TEXT("listTreeContent"));
		return dwError;
	}

	do
	{
		if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0)
			continue;

		// combine source path \ new filename
		wchar_t currentItem[MAX_LINE] = {0};
		if (wcslen(source) + wcslen(ffd.cFileName) + 1 > MAX_LINE)
		{
			writeFileW(LOG_FILE, L"Folder path is full, can't add filename");
			MessageBox(NULL, L"Folder path is full, can't add filename", L"Error", MB_ICONEXCLAMATION | MB_OK);
			return 0;
		}
		catPath(currentItem, source, ffd.cFileName);

		LARGE_INTEGER filesize;
		filesize.LowPart = ffd.nFileSizeLow;
		filesize.HighPart = ffd.nFileSizeHigh;

		// recursive folder reading
		//static int position = 0;
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			wchar_t subFolder[MAX_LINE] = {0};
			wcscpy_s(subFolder, MAX_LINE, currentItem);

			if (wcslen(subFolder) >= MAX_LINE)
			{
				writeFileW(LOG_FILE, L"Sub-folder path is full, can't add \\");
				MessageBox(NULL, L"Sub-folder path is full, can't add \\", L"Error", MB_ICONEXCLAMATION | MB_OK);
				return 0;
			}
			wcscat(subFolder, L"\\");
			//SendMessage(hwnd, LB_ADDSTRING, position++, (LPARAM)subFolder);
			listTreeContent(hwnd, pairs, currentItem, destination); //NOTE does this need extra path info added?
#if DEV_MODE
	wchar_t buf[MAX_LINE] = {0};
	swprintf(buf, MAX_LINE, L"LTC() Dir: %s", currentItem);
	writeFileW(LOG_FILE, buf);
#endif
		}
		else
		{
			// add to files list
			addPair(pairs, currentItem, destination, filesize.QuadPart);
		}
	}
	while (FindNextFile(hFind, &ffd) != 0);

	dwError = GetLastError();
	if (dwError != ERROR_NO_MORE_FILES)
		displayErrorBox(TEXT("listTreeContent"));

	FindClose(hFind);
	return dwError;
}

static void catPath(wchar_t *path, wchar_t *folder, wchar_t *filename)
{
	wcscpy_s(path, MAX_LINE, folder);
	wcscat(path, L"\\");
	wcscat(path, filename);
}

static bool fileExists(wchar_t *path)
{
	DWORD dwAttrib = GetFileAttributes(path);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

static bool folderExists(wchar_t *path)
{
	DWORD dwAttrib = GetFileAttributes(path);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

static LONGLONG getFileSize(wchar_t *path)
{
	WIN32_FILE_ATTRIBUTE_DATA info = { 0 };

	if (!GetFileAttributesEx(path, GetFileExInfoStandard, &info))
	{
		writeFileW(LOG_FILE, L"Error reading file attributes!");
		MessageBox(NULL, L"Error reading file attributes", L"Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	LARGE_INTEGER size;
	size.HighPart = info.nFileSizeHigh;
	size.LowPart = info.nFileSizeLow;
	return size.QuadPart;
}

// this is being handled by the FindNextFile structure instead
//static void getFileDate(wchar_t *path, SYSTEMTIME *systemTime)
//{
//	WIN32_FILE_ATTRIBUTE_DATA info = { 0 };
//
//	if (!GetFileAttributesEx(path, GetFileExInfoStandard, &info))
//	{
//		writeFileW(LOG_FILE, L"Error reading file attributes!");
//		MessageBox(NULL, L"Error reading file attributes", L"Error", MB_ICONEXCLAMATION | MB_OK);
//		return;
//	}
//
//	if (!FileTimeToSystemTime(&info.ftLastWriteTime, systemTime))
//	{
//		writeFileW(LOG_FILE, L"Error converting file time to system time!");
//		MessageBox(NULL, L"Error converting file time to system time", L"Error", MB_ICONEXCLAMATION | MB_OK);
//		return;
//	}
//
//	return;
//}

static bool fileDateIsDifferent(FILETIME srcCreate, FILETIME srcAccess, FILETIME srcWrite, wchar_t * dstPath)
{
	SYSTEMTIME srcInfo = { 0 };
	SYSTEMTIME dstInfo = { 0 };

	SYSTEMTIME srcLocal = { 0 };
	SYSTEMTIME dstLocal = { 0 };

	//HANDLE src = CreateFile(srcPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	HANDLE dst = CreateFile(dstPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	FILETIME dstCreate = { 0 };
	FILETIME dstAccess = { 0 };
	FILETIME dstWrite = { 0 };

	if (!GetFileTime(dst, &dstCreate, &dstAccess, &dstWrite))
	{
		wchar_t buf[MAX_LINE] = {0};
		swprintf(buf, MAX_LINE, L"Failed to get file time for destination %s", dstPath);
		writeFileW(LOG_FILE, buf);
		return false;
	}

	FileTimeToSystemTime(&srcWrite, &srcInfo);
	SystemTimeToTzSpecificLocalTime(NULL, &srcInfo, &srcLocal);

	FileTimeToSystemTime(&dstWrite, &dstInfo);
	SystemTimeToTzSpecificLocalTime(NULL, &dstInfo, &dstLocal);

	//TODO add more specific date check?
	if (srcInfo.wYear != dstInfo.wYear) return true;
	if (srcInfo.wMonth != dstInfo.wMonth) return true;
	if (srcInfo.wDay != dstInfo.wDay) return true;
	if (srcInfo.wHour != dstInfo.wHour) return true;
	if (srcInfo.wMinute != dstInfo.wMinute) return true;
	if (srcInfo.wSecond != dstInfo.wSecond) return true;

	return false;
}

/*
static void copyFile(wchar_t *source, wchar_t *dest)
{
	//TODO handle hidden & read-only & failures
//FILE_ATTRIBUTE_HIDDEN
//FILE_ATTRIBUTE_READONLY

	if (!CopyFile(source, dest, false))
	{
		writeFileW(LOG_FILE, L"Error copying file!");
		MessageBox(NULL, L"Error copying file", L"Error", MB_ICONEXCLAMATION | MB_OK);
	}
}

static void createFolder(wchar_t *path)
{
	if (!CreateDirectory(path, NULL))
	{
		writeFileW(LOG_FILE, L"Error creating folder!");
		MessageBox(NULL, L"Error creating folder", L"Error", MB_ICONEXCLAMATION | MB_OK);
	}
}

static void deleteFile(wchar_t *path)
{
	if (!DeleteFileW(path))
	{
		writeFileW(LOG_FILE, L"Error deleting file!");
		MessageBox(NULL, L"Error deleting file", L"Error", MB_ICONEXCLAMATION | MB_OK);
	}
}

static void deleteFolder(wchar_t *path)
{
	//NOTE is this function recursive?
	if (!RemoveDirectory(path))
	{
		writeFileW(LOG_FILE, L"Error deleting folder!");
		MessageBox(NULL, L"Error deleting folder", L"Error", MB_ICONEXCLAMATION | MB_OK);
	}
}
*/

//FIX some files are passed without path so are duplicated in the list
static int previewFolderPairSourceTest(HWND hwnd, struct PairNode **pairs, struct Project *project)
{
	wchar_t szDir[MAX_LINE];
	if (wcslen(project->pair.source) >= MAX_LINE)
	{
		writeFileW(LOG_FILE, L"Folder path is full, can't add \\");
		MessageBox(NULL, L"Folder path is full, can't add \\", L"Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}
	catPath(szDir, project->pair.source, L"*");

	DWORD dwError = 0;
	WIN32_FIND_DATA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	hFind = FindFirstFile(szDir, &ffd);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		displayErrorBox(TEXT("FindFirstFile"));
		return dwError;
	}

	do
	{
		if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0)
			continue;

		LARGE_INTEGER filesize;
		filesize.LowPart = ffd.nFileSizeLow;
		filesize.HighPart = ffd.nFileSizeHigh;

		wchar_t newSource[MAX_LINE];
		catPath(newSource, project->pair.source, ffd.cFileName);

		wchar_t destination[MAX_LINE] = { 0 };
		catPath(destination, project->pair.destination, ffd.cFileName);

		// folder
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			// check if target folder exists
			// if exists do recursive Select folder
			if (folderExists(destination))
			{
				struct Project subFolder = {0};
				wcscpy_s(subFolder.name, MAX_LINE, project->name);
				wcscpy_s(subFolder.pair.source, MAX_LINE, newSource);
				wcscpy_s(subFolder.pair.destination, MAX_LINE, destination);

#if DEV_MODE
	wchar_t buf[MAX_LINE] = {0};
	swprintf(buf, MAX_LINE, L"recursive call to previewFolderPairSourceTest() for %s -> %s", newSource, destination);
	writeFileW(LOG_FILE, buf);
#endif
				previewFolderPairSourceTest(hwnd, pairs, &subFolder);
			}
			else // target folder does not exist
			{
#if DEV_MODE
	wchar_t buf[MAX_LINE] = {0};
	swprintf(buf, MAX_LINE, L"dump entire tree with listTreeContent() for %s -> %s", newSource, destination);
	writeFileW(LOG_FILE, buf);
#endif
				// if new, add entire folder contents to list (recursive but without comparison)
				listTreeContent(hwnd, pairs, newSource, destination);
			}
		}
		else // item is a file
		{
			// check if target file exists
			if (fileExists(destination))
			{
#if DEV_MODE
	wchar_t buf[MAX_LINE] = {0};
	swprintf(buf, MAX_LINE, L"target file exists, compare date/time and sizes");
	writeFileW(LOG_FILE, buf);
#endif
				LONGLONG targetSize = getFileSize(destination);

				// if different size, add file to list
				if (targetSize != filesize.QuadPart)
				{
#if DEV_MODE
	//wchar_t buf[MAX_LINE] = {0};
	swprintf(buf, MAX_LINE, L"target file is out of date, add %s to list", destination);
	writeFileW(LOG_FILE, buf);
#endif
					addPair(pairs, newSource, destination, filesize.QuadPart);
				}
				else // size is the same so test date/time
				{
					// if last write time is different between source and destination, add file to list
					if (fileDateIsDifferent(ffd.ftCreationTime, ffd.ftLastAccessTime, ffd.ftLastWriteTime, destination))
					{
#if DEV_MODE
	//wchar_t buf[MAX_LINE] = {0};
	swprintf(buf, MAX_LINE, L"source & target files have different date/time, add %s to list", destination);
	writeFileW(LOG_FILE, buf);
#endif
						addPair(pairs, newSource, destination, filesize.QuadPart);
					}
				}
			}
			else // destination file does not exist
			{
				// add to files list
#if DEV_MODE
	wchar_t buf[MAX_LINE] = {0};
	swprintf(buf, MAX_LINE, L"target file does not exist, add %s to list", destination);
	writeFileW(LOG_FILE, buf);
#endif
				addPair(pairs, newSource, destination, filesize.QuadPart);
			}
		}
	}
	while (FindNextFile(hFind, &ffd) != 0);

	dwError = GetLastError();
	if (dwError != ERROR_NO_MORE_FILES)
		displayErrorBox(TEXT("previewFolderPairSourceTest"));

	FindClose(hFind);
	return dwError;
}

static int previewFolderPairTargetTest(HWND hwnd, struct PairNode **pairs, struct Project *project)
{
	return 0;
}
