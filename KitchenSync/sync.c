#include "kitchen_sync.h"

#define LIVE_NOT_SIMULATED 1

DWORD CALLBACK entryPointSync(LPVOID);

void synchronizeFiles(HWND pbHwnd, HWND lbSyncHwnd, HWND bSync, HWND tabHwnd, struct Pair **pairIndex)
{
	HANDLE threads[1];
	DWORD threadIDs[1];

	static struct SyncArguments args;
	args.pbHwnd = pbHwnd;
	args.lbSyncHwnd = lbSyncHwnd;
	args.bSync = bSync;
	args.tabHwnd = tabHwnd;
	args.pairIndex = pairIndex;

	threads[0] = CreateThread(NULL, 0, entryPointSync, &args, 0, &threadIDs[0]);

	if (threads[0] == NULL)
	{
		wchar_t buf[MAX_LINE] = L"Failed to create sync thread";
		logger(buf);
	}
}

DWORD CALLBACK entryPointSync(LPVOID arguments)
{
	struct SyncArguments *args = (struct SyncArguments *)arguments;
	HWND pbHwnd = args->pbHwnd;
	HWND lbSyncHwnd = args->lbSyncHwnd;
	HWND bSync = args->bSync;
	HWND tabHwnd = args->tabHwnd;
	struct Pair **pairIndex = args->pairIndex;
	int position = 0;
	int completed = 0;
	assert(pairCount > 0);
	wchar_t buf[MAX_LINE] = { 0 };

	SetWindowText(bSync, L"Working...");
	SendMessage(lbSyncHwnd, LB_RESETCONTENT, 0, 0);
	SendMessage(pbHwnd, PBM_SETPOS, 0, 0);

	startCount();

	for (int i = 0; i < pairCount; ++i)
	{
		struct Pair *pair = pairIndex[i];
		if (pair == NULL)
			continue;

		if (wcscmp(pair->source, L"Delete folder") == 0)
			continue;

		if (wcscmp(pair->source, L"Delete file") == 0)
		{
			// delete file
#if LIVE_NOT_SIMULATED
			if (deleteFile(pair->destination))
			{
				wcscpy_s(buf, MAX_LINE, L"Deleted file ");
				wcscat(buf, pair->destination);
				wcscat(buf, L" OK");
			}
			else
			{
				wcscpy_s(buf, MAX_LINE, L"Failed deleting file ");
				wcscat(buf, pair->destination);
			}
#else
			wcscpy_s(buf, MAX_LINE, L"Deleted file ");
			wcscat(buf, pair->destination);
			wcscat(buf, L" OK");
#endif
		}
		else
		{
			// create folder
			size_t lastPosition = wcslen(pair->destination) - 1;
			if (pair->destination[lastPosition] == '\\')
			{
#if LIVE_NOT_SIMULATED
				if (createFolder(pair->destination))
				{
					wcscpy_s(buf, MAX_LINE, L"Created folder ");
					wcscat(buf, pair->source);
					wcscat(buf, L" -> ");
					wcscat(buf, pair->destination);
					wcscat(buf, L" OK");
				}
				else
				{
					wcscpy_s(buf, MAX_LINE, L"Failed creating folder ");
					wcscat(buf, pair->source);
					wcscat(buf, L" -> ");
					wcscat(buf, pair->destination);
				}
#else
				wcscpy_s(buf, MAX_LINE, L"Created folder ");
				wcscat(buf, pair->source);
				wcscat(buf, L" -> ");
				wcscat(buf, pair->destination);
				wcscat(buf, L" OK");
#endif
			}
			else // copy file
			{
#if LIVE_NOT_SIMULATED
				if (copyFile(pair->source, pair->destination))
				{
					wcscpy_s(buf, MAX_LINE, L"Copied file ");
					wcscat(buf, pair->source);
					wcscat(buf, L" -> ");
					wcscat(buf, pair->destination);
					wcscat(buf, L" OK");
				}
				else
				{
					wcscpy_s(buf, MAX_LINE, L"Failed copying file ");
					wcscat(buf, pair->source);
					wcscat(buf, L" -> ");
					wcscat(buf, pair->destination);
			}
#else
				wcscpy_s(buf, MAX_LINE, L"Copied file ");
				wcscat(buf, pair->source);
				wcscat(buf, L" -> ");
				wcscat(buf, pair->destination);
				wcscat(buf, L" OK");
#endif
			}
		}

		logger(buf);
		int progressPosition = (int)(((float)++completed / pairCount) * 100.0f);
		SendMessage(pbHwnd, PBM_SETPOS, progressPosition, 0);
		SendMessage(lbSyncHwnd, LB_ADDSTRING, position++, (LPARAM)buf);
	}

	// set current to last non-null pair
	int pairPosition = pairCount-1;
	if (pairPosition < 0)
	{
		logger(L"Invalid pairPosition");
		return 0;
	}
	struct Pair *pair = pairIndex[pairPosition];
	while (pair == NULL && pairPosition > 0)
	{
		--pairPosition;
		pair = pairIndex[pairPosition];
	}

	if (pair == NULL)
	{
		logger(L"No valid pair found");
		return 0;
	}

	// search backwards for folder deletions
	while (wcscmp(pair->source, L"Delete folder") == 0)
	{
		// delete folder
#if LIVE_NOT_SIMULATED
		if (deleteFolder(pair->destination))
		{
			wcscpy_s(buf, MAX_LINE, L"Deleted folder ");
			wcscat(buf, pair->destination);
			wcscat(buf, L" OK");
		}
		else
		{
			wcscpy_s(buf, MAX_LINE, L"Failed deleting folder ");
			wcscat(buf, pair->destination);
		}
#else
		wcscpy_s(buf, MAX_LINE, L"Deleted folder ");
		wcscat(buf, pair->destination);
		wcscat(buf, L" OK");
#endif

		logger(buf);
		int progressPosition = (int)(((float)++completed / pairCount) * 100.0f);
		SendMessage(pbHwnd, PBM_SETPOS, progressPosition, 0);
		SendMessage(lbSyncHwnd, LB_ADDSTRING, position++, (LPARAM)buf);

		if (pairPosition > 0)
			pair = pairIndex[--pairPosition];
		else
			break;
	}

	endCount(L"Sync");
	SendMessage(pbHwnd, PBM_SETPOS, 100, 0); // force 100%
	SetWindowText(bSync, L"Sync");
	EnableWindow(bSync, false);
	EnableWindow(tabHwnd, true);
	deleteAllPairs(pairIndex);
	return 0;
}
