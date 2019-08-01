#include "kitchen_sync.h"

static void displayErrorBox(LPTSTR);

int listFolders(HWND hwnd, wchar_t *folder)
{
#if DEV_MODE
	writeFileW(LOG_FILE, L"listDir()");
#endif

	DWORD dwError = 0;
	WIN32_FIND_DATA ffd;

	wchar_t szDir[MAX_LINE];
	wcscpy_s(szDir, MAX_LINE, folder);
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

int listFolderContent(HWND hwnd, wchar_t *folder)
{
#if DEV_MODE
	writeFileW(LOG_FILE, L"listDir()");
#endif

	DWORD dwError = 0;
	WIN32_FIND_DATA ffd;

	wchar_t szDir[MAX_LINE];
	wcscpy_s(szDir, MAX_LINE, folder);
	wcscat(szDir, L"\\*");

	HANDLE hFind = INVALID_HANDLE_VALUE;
	hFind = FindFirstFile(szDir, &ffd);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		displayErrorBox(TEXT("FindFirstFile"));
		return dwError;
	}

	LARGE_INTEGER filesize;
	do
	{
		if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0)
			continue;

		filesize.LowPart = ffd.nFileSizeLow;
		filesize.HighPart = ffd.nFileSizeHigh;

		wchar_t currentItem[MAX_LINE] = {0};
		wcscpy_s(currentItem, MAX_LINE, folder);
		wcscat(currentItem, L"\\");
		wcscat(currentItem, ffd.cFileName);

		// recursive folder reading
		static int position = 0;
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			wchar_t subFolder[MAX_LINE] = {0};
			wcscpy_s(subFolder, MAX_LINE, currentItem);
			wcscat(subFolder, L"\\");
			SendMessage(hwnd, LB_ADDSTRING, position++, (LPARAM)subFolder);
			listFolderContent(hwnd, currentItem);
		}

#if DEV_MODE
		wchar_t buf[MAX_LINE] = {0};
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			swprintf(buf, MAX_LINE, L"Dir: %s", currentItem);
		else
			swprintf(buf, MAX_LINE, L"File: %s, Size: %lld", currentItem, filesize.QuadPart);
		writeFileW(LOG_FILE, buf);
#endif

		if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			SendMessage(hwnd, LB_ADDSTRING, position++, (LPARAM)currentItem);
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
