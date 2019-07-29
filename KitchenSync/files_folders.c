#include "kitchen_sync.h"

static void displayErrorBox(LPTSTR);

int listDir(HWND hwnd, wchar_t *folder)
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
	//LARGE_INTEGER filesize;
	do
	{
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (wcscmp(ffd.cFileName, L".") == 0)
				continue;

			wchar_t buf[MAX_LINE] = {0};
			swprintf(buf, MAX_LINE, L"Dir: %s", ffd.cFileName);
			writeFileW(LOG_FILE, buf);
			SendMessage(hwnd, LB_ADDSTRING, position++, (LPARAM)ffd.cFileName);
		}
		//else
		//{
		//	filesize.LowPart = ffd.nFileSizeLow;
		//	filesize.HighPart = ffd.nFileSizeHigh;

		//	wchar_t buf[MAX_LINE] = {0};
		//	swprintf(buf, MAX_LINE, L"File: %s, Size: %lld", ffd.cFileName, filesize.QuadPart);
		//	writeFileW(LOG_FILE, buf);
		//	SendMessage(hwnd, LB_ADDSTRING, position++, (LPARAM)ffd.cFileName);
		//}
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
