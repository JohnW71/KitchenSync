#include "kitchen_sync.h"

void catPath(wchar_t *path, wchar_t *folder, wchar_t *filename)
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

LONGLONG getFileSize(wchar_t *path)
{
	WIN32_FILE_ATTRIBUTE_DATA info = { 0 };

	if (!GetFileAttributesEx(path, GetFileExInfoStandard, &info))
	{
		logger(L"Error reading file attributes!");
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

bool fileDateIsDifferent(FILETIME srcCreate, FILETIME srcAccess, FILETIME srcWrite, wchar_t * dstPath)
{
	SYSTEMTIME srcInfo = { 0 };
	SYSTEMTIME dstInfo = { 0 };
	SYSTEMTIME srcLocal = { 0 };
	SYSTEMTIME dstLocal = { 0 };
	FILETIME dstCreate = { 0 };
	FILETIME dstAccess = { 0 };
	FILETIME dstWrite = { 0 };

	HANDLE dst = CreateFile(dstPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	if (!GetFileTime(dst, &dstCreate, &dstAccess, &dstWrite))
	{
		wchar_t buf[MAX_LINE] = { 0 };
		swprintf(buf, MAX_LINE, L"Failed to get file time for destination %s", dstPath);
		logger(buf);
		return false;
	}

	FileTimeToSystemTime(&srcWrite, &srcInfo);
	SystemTimeToTzSpecificLocalTime(NULL, &srcInfo, &srcLocal);

	FileTimeToSystemTime(&dstWrite, &dstInfo);
	SystemTimeToTzSpecificLocalTime(NULL, &dstInfo, &dstLocal);

	if (srcInfo.wYear != dstInfo.wYear) return true;
	if (srcInfo.wMonth != dstInfo.wMonth) return true;
	if (srcInfo.wDay != dstInfo.wDay) return true;
	if (srcInfo.wHour != dstInfo.wHour) return true;
	if (srcInfo.wMinute != dstInfo.wMinute) return true;
	if (srcInfo.wSecond != dstInfo.wSecond) return true;

	return false;
}

bool copyFile(wchar_t *source, wchar_t *dest)
{
//FILE_ATTRIBUTE_HIDDEN
//FILE_ATTRIBUTE_READONLY

	if (CopyFile(source, dest, false))
		return true;

	logger(L"Error copying file!");
	logger(source);
	MessageBox(NULL, L"Error copying file", L"Error", MB_ICONEXCLAMATION | MB_OK);
	return false;
}

bool createFolder(wchar_t *path)
{
	if (CreateDirectory(path, NULL))
		return true;

	logger(L"Error creating folder!");
	logger(path);
	MessageBox(NULL, L"Error creating folder", L"Error", MB_ICONEXCLAMATION | MB_OK);
	return false;
}

bool deleteFile(wchar_t *path)
{
	if (DeleteFileW(path))
		return true;

	logger(L"Error deleting file!");
	logger(path);
	MessageBox(NULL, L"Error deleting file", L"Error", MB_ICONEXCLAMATION | MB_OK);
	return false;
}

bool deleteFolder(wchar_t *path)
{
	//NOTE is this function recursive?
	if (RemoveDirectory(path))
		return true;

	logger(L"Error deleting folder!");
	logger(path);
	MessageBox(NULL, L"Error deleting folder", L"Error", MB_ICONEXCLAMATION | MB_OK);
	return false;
}

// cut off last folder name from path
void cutOffLastFolder(wchar_t *folderPath)
{
	size_t len = wcslen(folderPath);
	assert(len > 0);
	while (folderPath[len] != '\\' && len > 0)
		--len;
	folderPath[len] = '\0';
}
