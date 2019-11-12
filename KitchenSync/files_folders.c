#include "kitchen_sync.h"

static bool fileExists(wchar_t *);
static bool folderExists(wchar_t *);
static void displayErrorBox(LPTSTR);
static int previewFolderPairSourceTest(HWND, struct PairNode **, struct Project);
static int previewFolderPairTargetTest(HWND, struct PairNode **, struct Project);

void previewProject(HWND hwnd, struct ProjectNode **head_ref, struct PairNode **pairs, wchar_t *projectName)
{
#if DEV_MODE
	writeFileW(LOG_FILE, L"previewProject()");
#endif

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
			previewFolderPairTest(hwnd, pairs, project);
		}
		current = current->next;
	}
	while (*head_ref != NULL && current != NULL);
}

// void previewFolderPair(HWND hwnd, struct PairNode **pairs, struct Project project)
// {
// #if DEV_MODE
// 	writeFileW(LOG_FILE, L"previewFolderPair()");
// #endif

// 	listTreeContent(hwnd, pairs, project.pair.source);
// }

void previewFolderPairTest(HWND hwnd, struct PairNode **pairs, struct Project project)
{
	previewFolderPairSourceTest(hwnd, pairs, project);
	previewFolderPairTargetTest(hwnd, pairs, project);
}

// this lists the files/folders within the supplied folder level only
int listFolderContent(HWND hwnd, wchar_t *folder)
{
#if DEV_MODE
	writeFileW(LOG_FILE, L"listFolderContent()");
#endif

	DWORD dwError = 0;
	WIN32_FIND_DATA ffd;

	wchar_t szDir[MAX_LINE];
	wcscpy_s(szDir, MAX_LINE, folder);
	if (wcslen(szDir) >= MAX_LINE)
	{
		writeFileW(LOG_FILE, L"Folder path is full, can't add \\");
		MessageBox(NULL, L"Folder path is full, can't add \\", L"Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}
	wcscat(szDir, L"\\*");

	HANDLE hFind = INVALID_HANDLE_VALUE;
	hFind = FindFirstFile(szDir, &ffd);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		displayErrorBox(TEXT("FindFirstFile"));
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
	wchar_t buf[MAX_LINE] = {0};
	swprintf(buf, MAX_LINE, L"Dir: %s", ffd.cFileName);
	writeFileW(LOG_FILE, buf);
#endif
			SendMessage(hwnd, LB_ADDSTRING, position++, (LPARAM)ffd.cFileName);
		}
	}
	while (FindNextFile(hFind, &ffd) != 0);

	dwError = GetLastError();
	if (dwError != ERROR_NO_MORE_FILES)
		displayErrorBox(TEXT("FindFirstFile"));

	FindClose(hFind);
	return dwError;
}

// this adds all the files/folders in and below the supplied folder to the pairs list
int listTreeContent(HWND hwnd, struct PairNode **pairs, wchar_t *source, wchar_t *destination)
{
#if DEV_MODE
	writeFileW(LOG_FILE, L"listTreeContent()");
#endif

	DWORD dwError = 0;
	WIN32_FIND_DATA ffd;

	wchar_t szDir[MAX_LINE];
	wcscpy_s(szDir, MAX_LINE, source);
	if (wcslen(szDir) >= MAX_LINE)
	{
		writeFileW(LOG_FILE, L"Folder path is full, can't add \\");
		MessageBox(NULL, L"Folder path is full, can't add \\", L"Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}
	wcscat(szDir, L"\\*");

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

		wchar_t currentItem[MAX_LINE] = {0};
		wcscpy_s(currentItem, MAX_LINE, source);
		if (wcslen(currentItem) + wcslen(ffd.cFileName) + 1 > MAX_LINE)
		{
			writeFileW(LOG_FILE, L"Folder path is full, can't add filename");
			MessageBox(NULL, L"Folder path is full, can't add filename", L"Error", MB_ICONEXCLAMATION | MB_OK);
			return 0;
		}
		wcscat(currentItem, L"\\");
		wcscat(currentItem, ffd.cFileName);

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
//#if DEV_MODE
//	wchar_t buf[MAX_LINE] = {0};
//	swprintf(buf, MAX_LINE, L"Dir: %s", currentItem);
//	writeFileW(LOG_FILE, buf);
//#endif
		}
		else
		{
			// add to files list
			struct Pair newPair = {0};
			wcscpy_s(newPair.source, MAX_LINE, ffd.cFileName);
			wcscpy_s(newPair.destination, MAX_LINE, destination); //NOTE does this need extra path info added?
			appendPairNode(pairs, newPair, filesize.QuadPart);
//#if DEV_MODE
//	wchar_t buf[MAX_LINE] = {0};
//	swprintf(buf, MAX_LINE, L"File: %s, Size: %lld", currentItem, filesize.QuadPart);
//	writeFileW(LOG_FILE, buf);
//#endif
		}
	}
	while (FindNextFile(hFind, &ffd) != 0);

	dwError = GetLastError();
	if (dwError != ERROR_NO_MORE_FILES)
		displayErrorBox(TEXT("FindFirstFile"));

	FindClose(hFind);
	return dwError;
}

static void displayErrorBox(LPTSTR lpszFunction)
{
#if DEV_MODE
	writeFileW(LOG_FILE, L"DisplayErrorBox()");
#endif

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

//typedef struct _WIN32_FILE_ATTRIBUTE_DATA {
//	DWORD    dwFileAttributes;
//	FILETIME ftCreationTime;
//	FILETIME ftLastAccessTime;
//	FILETIME ftLastWriteTime;
//	DWORD    nFileSizeHigh;
//	DWORD    nFileSizeLow;
//} WIN32_FILE_ATTRIBUTE_DATA, *LPWIN32_FILE_ATTRIBUTE_DATA;
//
//typedef struct _FILETIME {
//	DWORD dwLowDateTime;
//	DWORD dwHighDateTime;
//} FILETIME, *PFILETIME, *LPFILETIME;
//

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
	//NOTE is this recursive?
	if (!RemoveDirectory(path))
	{
		writeFileW(LOG_FILE, L"Error deleting folder!");
		MessageBox(NULL, L"Error deleting folder", L"Error", MB_ICONEXCLAMATION | MB_OK);
	}
}
*/

static int previewFolderPairSourceTest(HWND hwnd, struct PairNode **pairs, struct Project project)
{
#if DEV_MODE
	writeFileW(LOG_FILE, L"previewFolderPairSourceTest()");
#endif

	DWORD dwError = 0;
	WIN32_FIND_DATA ffd;

	wchar_t szDir[MAX_LINE];
	wcscpy_s(szDir, MAX_LINE, project.pair.source);
	if (wcslen(szDir) >= MAX_LINE)
	{
		writeFileW(LOG_FILE, L"Folder path is full, can't add \\");
		MessageBox(NULL, L"Folder path is full, can't add \\", L"Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}
	wcscat(szDir, L"\\*");

//#if DEV_MODE
//	wchar_t buf[MAX_LINE] = {0};
//	swprintf(buf, MAX_LINE, L"Source folder %s", szDir);
//	writeFileW(LOG_FILE, buf);
//#endif

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

//NOTE what value does this give if it's a folder?
		LARGE_INTEGER filesize;
		filesize.LowPart = ffd.nFileSizeLow;
		filesize.HighPart = ffd.nFileSizeHigh;

		// folder
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			// check if target folder exists
			wchar_t destination[MAX_LINE] = {0};
			wcscpy_s(destination, MAX_LINE, project.pair.destination);
			wcscat(destination, L"\\");
			wcscat(destination, ffd.cFileName);

//#if DEV_MODE
//	wchar_t buf[MAX_LINE] = {0};
//	swprintf(buf, MAX_LINE, L"Target folder %s", destination);
//	writeFileW(LOG_FILE, buf);
//#endif

			// if exists recursive Select folder
			if (folderExists(destination))
			{
				wchar_t newSource[MAX_LINE];
				wcscpy_s(newSource, MAX_LINE, project.pair.source);
				wcscat(newSource, L"\\");
				wcscat(newSource, ffd.cFileName);

				struct Project subFolder = {0};
				wcscpy_s(subFolder.name, MAX_LINE, project.name);
				wcscpy_s(subFolder.pair.source, MAX_LINE, newSource);
				wcscpy_s(subFolder.pair.destination, MAX_LINE, destination);

//#if DEV_MODE
//	wchar_t buf[MAX_LINE] = {0};
//	swprintf(buf, MAX_LINE, L"recursive call to previewFolderPairSourceTest()");
//	writeFileW(LOG_FILE, buf);
//#endif
				previewFolderPairSourceTest(hwnd, pairs, subFolder);
			}
			else // target folder does not exist
			{
//#if DEV_MODE
//	wchar_t buf[MAX_LINE] = {0};
//	swprintf(buf, MAX_LINE, L"dump entire tree with listTreeContent()");
//	writeFileW(LOG_FILE, buf);
//#endif
				// if new, add entire folder contents to list (recursive but without comparison)
				//TODO this needs to send the destination folder path as well
				listTreeContent(hwnd, pairs, project.pair.source, destination);
			}
		}
		else // item is a file
		{
			// check if target file exists
			wchar_t destination[MAX_LINE] = {0};
			wcscpy_s(destination, MAX_LINE, project.pair.destination);
			wcscat(destination, L"\\");
			wcscat(destination, ffd.cFileName);

//#if DEV_MODE
//	wchar_t buf[MAX_LINE] = {0};
//	swprintf(buf, MAX_LINE, L"Target file: %s, Size: %lld", destination, filesize.QuadPart);
//	writeFileW(LOG_FILE, buf);
//#endif

			if (fileExists(destination))
			{
#if DEV_MODE
	wchar_t buf[MAX_LINE] = {0};
	swprintf(buf, MAX_LINE, L"target file exists, compare date/time and sizes");
	writeFileW(LOG_FILE, buf);
#endif

				LONGLONG targetSize = getFileSize(destination);
#if DEV_MODE
	//wchar_t buf[MAX_LINE] = { 0 };
	swprintf(buf, MAX_LINE, L"target file size %lld", targetSize);
	writeFileW(LOG_FILE, buf);
#endif
				//TODO filesize comparison
				//TODO date/time comparison
				//NOTE need to work out the file attribute methods for this
				// if different, add file to list
				if (targetSize != filesize.QuadPart)
				{
#if DEV_MODE
	//wchar_t buf[MAX_LINE] = {0};
	swprintf(buf, MAX_LINE, L"target file is out of date, add to list");
	writeFileW(LOG_FILE, buf);
#endif

					wchar_t srcFile[MAX_LINE] = {0};
					wcscpy_s(srcFile, MAX_LINE, project.pair.source);
					wcscat(srcFile, L"\\");
					wcscat(srcFile, ffd.cFileName);

					wchar_t destFile[MAX_LINE] = {0};
					wcscpy_s(destFile, MAX_LINE, project.pair.destination);
					wcscat(destFile, L"\\");
					wcscat(destFile, ffd.cFileName);

					struct Pair newPair = {0};
					wcscpy_s(newPair.source, MAX_LINE, srcFile);
					wcscpy_s(newPair.destination, MAX_LINE, destFile);
					appendPairNode(pairs, newPair, filesize.QuadPart);
//#if DEV_MODE
//	wchar_t buf[MAX_LINE] = {0};
//	swprintf(buf, MAX_LINE, L"Source file:\n%sDestination file:\n%s", srcFile, destFile);
//	writeFileW(LOG_FILE, buf);
//#endif
				}
			}
			else
			{
				// add to files list
 #if DEV_MODE
	wchar_t buf[MAX_LINE] = {0};
	swprintf(buf, MAX_LINE, L"target file does not exist, add to list");
	writeFileW(LOG_FILE, buf);
 #endif

				wchar_t srcFile[MAX_LINE] = {0};
				wcscpy_s(srcFile, MAX_LINE, project.pair.source);
				wcscat(srcFile, L"\\");
				wcscat(srcFile, ffd.cFileName);

				wchar_t destFile[MAX_LINE] = {0};
				wcscpy_s(destFile, MAX_LINE, project.pair.destination);
				wcscat(destFile, L"\\");
				wcscat(destFile, ffd.cFileName);

				struct Pair newPair = {0};
				wcscpy_s(newPair.source, MAX_LINE, srcFile);
				wcscpy_s(newPair.destination, MAX_LINE, destFile);
				appendPairNode(pairs, newPair, filesize.QuadPart);
//#if DEV_MODE
//	wchar_t buf[MAX_LINE] = {0};
//	swprintf(buf, MAX_LINE, L"Source file:\n%sDestination file:\n%s", srcFile, destFile);
//	writeFileW(LOG_FILE, buf);
//#endif
			}
		}
	}
	while (FindNextFile(hFind, &ffd) != 0);

	dwError = GetLastError();
	if (dwError != ERROR_NO_MORE_FILES)
		displayErrorBox(TEXT("FindFirstFile"));

	FindClose(hFind);
	return dwError;
}

static int previewFolderPairTargetTest(HWND hwnd, struct PairNode **pairs, struct Project project)
{
#if DEV_MODE
	writeFileW(LOG_FILE, L"previewFolderPairTargetTest()");
#endif
	return 0;
}
