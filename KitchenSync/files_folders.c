#include "kitchen_sync.h"

void addPath(wchar_t *path, wchar_t *folder, wchar_t *filename)
{
	wcscpy_s(path, MAX_LINE, folder);
	wcscat(path, L"\\");
	wcscat(path, filename);
}

bool fileExists(wchar_t *path)
{
	DWORD dwAttrib = GetFileAttributes(path);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool folderExists(wchar_t *path)
{
	DWORD dwAttrib = GetFileAttributes(path);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool readOnly(wchar_t *path)
{
	DWORD dwAttrib = GetFileAttributes(path);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_READONLY));
}

bool hiddenFile(wchar_t *path)
{
	DWORD dwAttrib = GetFileAttributes(path);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_HIDDEN));
}

LONGLONG getFileSize(wchar_t *path)
{
	WIN32_FILE_ATTRIBUTE_DATA info = {0};

	if (!GetFileAttributesEx(path, GetFileExInfoStandard, &info))
	{
		wchar_t buf[MAX_LINE] = {0};
		wcscpy_s(buf, MAX_LINE, L"Error reading file attributes for ");
		wcscat(buf, path);
		logger(buf);
		MessageBox(NULL, buf, L"Error", MB_ICONEXCLAMATION | MB_OK);
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
//		logger(L"Error reading file attributes!");
//		MessageBox(NULL, L"Error reading file attributes", L"Error", MB_ICONEXCLAMATION | MB_OK);
//		return;
//	}
//
//	if (!FileTimeToSystemTime(&info.ftLastWriteTime, systemTime))
//	{
//		logger(L"Error converting file time to system time!");
//		MessageBox(NULL, L"Error converting file time to system time", L"Error", MB_ICONEXCLAMATION | MB_OK);
//		return;
//	}
//
//	return;
//}

bool fileDateIsDifferent(FILETIME srcCreate, FILETIME srcAccess, FILETIME srcWrite, wchar_t *dstPath)
{
	SYSTEMTIME srcInfo = {0};
	SYSTEMTIME dstInfo = {0};
	SYSTEMTIME srcLocal = {0};
	SYSTEMTIME dstLocal = {0};
	FILETIME dstCreate = {0};
	FILETIME dstAccess = {0};
	FILETIME dstWrite = {0};

	HANDLE dst = CreateFile(dstPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	if (!GetFileTime(dst, &dstCreate, &dstAccess, &dstWrite))
	{
		wchar_t buf[MAX_LINE] = {0};
		wcscpy_s(buf, MAX_LINE, L"Failed to get file time for destination ");
		wcscat(buf, dstPath);
		logger(buf);
		return false;
	}

	CloseHandle(dst);

	FileTimeToSystemTime(&srcWrite, &srcInfo);
	SystemTimeToTzSpecificLocalTime(NULL, &srcInfo, &srcLocal);

	FileTimeToSystemTime(&dstWrite, &dstInfo);
	SystemTimeToTzSpecificLocalTime(NULL, &dstInfo, &dstLocal);

	if (srcLocal.wYear != dstLocal.wYear) return true;
	if (srcLocal.wMonth != dstLocal.wMonth) return true;
	if (srcLocal.wDay != dstLocal.wDay) return true;
	if (srcLocal.wHour != dstLocal.wHour) return true;
	//if (srcLocal.wMinute != dstLocal.wMinute) return true;
	//if (srcLocal.wSecond != dstLocal.wSecond) return true;

	// files copied to network drive have a modified time inaccurate to the second so are detected as changed.
	// this takes the minutes & seconds combined for src & dst and only marks the file as outdated if there is
	// more than 1 second difference. GetFileTime & SetFileTime do not help as the file still receives the
	// inaccurate timestamp
	int srcTime = (srcLocal.wMinute * 60) + srcLocal.wSecond;
	int dstTime = (dstLocal.wMinute * 60) + dstLocal.wSecond;

	if (abs(srcTime - dstTime) > 1)
		return true;

	return false;
}

bool copyFile(wchar_t *source, wchar_t *dest)
{
	if (CopyFile(source, dest, false))
		return true;

	if ((readOnly(dest) || hiddenFile(dest)) && setNormalFile(dest) && CopyFile(source, dest, false))
		return true;

	wchar_t buf[MAX_LINE] = {0};
	wcscpy_s(buf, MAX_LINE, L"Error copying file ");
	wcscat(buf, source);
	logger(buf);
	displayErrorBox(TEXT("CopyFile"));
	return false;
}

bool createFolder(wchar_t *path)
{
	if (CreateDirectory(path, NULL))
		return true;

	wchar_t buf[MAX_LINE] = {0};
	wcscpy_s(buf, MAX_LINE, L"Error creating folder ");
	wcscat(buf, path);
	logger(buf);
	displayErrorBox(TEXT("CreateDirectory"));
	return false;
}

bool deleteFile(wchar_t *path)
{
	if (DeleteFile(path))
		return true;

	if ((readOnly(path) || hiddenFile(path)) && setNormalFile(path) && DeleteFile(path))
		return true;

	wchar_t buf[MAX_LINE] = {0};
	wcscpy_s(buf, MAX_LINE, L"Error deleting file ");
	wcscat(buf, path);
	logger(buf);
	displayErrorBox(TEXT("DeleteFile"));
	return false;
}

bool deleteFolder(wchar_t *path)
{
	// folder must be empty
	if (RemoveDirectory(path))
		return true;

	wchar_t buf[MAX_LINE] = {0};
	wcscpy_s(buf, MAX_LINE, L"Error deleting folder ");
	wcscat(buf, path);
	logger(buf);
	displayErrorBox(TEXT("RemoveDirectory"));
	return false;
}

bool setNormalFile(wchar_t *path)
{
	if (SetFileAttributes(path, FILE_ATTRIBUTE_NORMAL))
	{
		wchar_t buf[MAX_LINE] = {0};
		wcscpy_s(buf, MAX_LINE, L"Removed read-only/hidden attribute for ");
		wcscat(buf, path);
		logger(buf);
		return true;
	}

	wchar_t buf[MAX_LINE] = {0};
	wcscpy_s(buf, MAX_LINE, L"Error setting normal attributes for ");
	wcscat(buf, path);
	logger(buf);
	displayErrorBox(TEXT("SetFileAttributes"));
	return false;
}

// cut off last folder name from path
void cutOffLastFolder(wchar_t *path)
{
	size_t len = wcslen(path);
	assert(len > 0);
	while (path[len] != '\\' && len > 0)
		--len;
	path[len] = '\0';
}

// list the folder names within the supplied folder level only, to the specified listbox
bool listSubFolders(HWND hwnd, wchar_t *path)
{
	wchar_t szDir[MAX_LINE];
	if (wcslen(path) >= MAX_LINE)
	{
		wchar_t buf[MAX_LINE] = L"Folder path is full, can't add \\";
		logger(buf);
		MessageBox(NULL, buf, L"Error", MB_ICONEXCLAMATION | MB_OK);
		return false;
	}
	addPath(szDir, path, L"*");

	WIN32_FIND_DATA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	hFind = FindFirstFile(szDir, &ffd);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		wchar_t buf[MAX_LINE] = {0};
		wcscpy_s(buf, MAX_LINE, L"Unable to access ");
		wcscat(buf, path);
		logger(buf);
		displayErrorBox(TEXT("listSubFolders"));
		return false;
	}

	int position = 0;

	// insert drive names into the list before adding the folders underneath
	wchar_t drivesList[MAX_LINE] = {0};
	DWORD driveListLength = getDriveStrings(MAX_LINE, drivesList);

	for (size_t i = 0; i < driveListLength; ++i)
		if (drivesList[i] == '\0')
			drivesList[i] = '\n';

	size_t i = 0;
	do
	{
		wchar_t name[MAX_LINE] = {0};
		size_t j = 0;

		while (drivesList[i] != '\n' && drivesList[i] != '\0')
			name[j++] = drivesList[i++];

		SendMessage(hwnd, LB_ADDSTRING, position++, (LPARAM)name);
		++i;
	} while (drivesList[i] != '\0');

	// add folders to list
	do
	{
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (wcscmp(ffd.cFileName, L".") == 0)
				continue;
			SendMessage(hwnd, LB_ADDSTRING, position++, (LPARAM)ffd.cFileName);
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	if (GetLastError() != ERROR_NO_MORE_FILES)
		displayErrorBox(TEXT("listSubFolders"));

	FindClose(hFind);
	return true;
}

DWORD getDriveStrings(DWORD length, wchar_t *buffer)
{
	DWORD result = GetLogicalDriveStrings(length, buffer);

	if (result == 0)
		logger(L"GetLogicalDriveStrings() failed");

	if (result > length)
	{
		logger(L"GetLogicalDriveStrings() buffer too small");
		result = length;
	}

	return result;
}

LONGLONG getDriveSpace(int driveIndex)
{
	ULARGE_INTEGER size;
	wchar_t driveLetter = toupper((wchar_t)(driveIndex + 65));
	if (driveLetter < 65 || driveLetter > 95)
		return 0;

	wchar_t drivePath[3] = {0};
	drivePath[0] = driveLetter;
	drivePath[1] = ':';

	//NOTE c:\\ or c:\ or c:
	if (GetDiskFreeSpaceEx(drivePath, NULL, NULL, &size))
	{
#if DEBUG_MODE
		wchar_t buf[MAX_LINE] = {0};
		swprintf(buf, MAX_LINE, L"total free bytes = %lld", size.QuadPart);
		logger(buf);
		swprintf(buf, MAX_LINE, L"total free KB 1024 = %lld", size.QuadPart / 1024);
		logger(buf);
		swprintf(buf, MAX_LINE, L"total free MB 1024 / 1024 = %lld", size.QuadPart / 1024 / 1024);
		logger(buf);
		swprintf(buf, MAX_LINE, L"total free GB 1024 / 1024 / 1024 = %lld", size.QuadPart / 1024 / 1024 / 1024);
		logger(buf);
#endif
		return size.QuadPart;
	}

	logger(L"Error getting drive space");
	displayErrorBox(TEXT("GetDiskFreeSpaceEX"));
	return 0;
}
