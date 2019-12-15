#include "kitchen_sync.h"

#define DANGEROUS 0

DWORD CALLBACK entryPointSync(LPVOID);

void synchronizeFiles(HWND pbHwnd, HWND lbSyncHwnd, HWND bSync, struct PairNode **pairs)
{
#if DEV_MODE
	logger(L"sync()");
#endif
	HANDLE threads[2];
	DWORD threadIDs[2];

	struct SyncArguments syncArgs = { pbHwnd, lbSyncHwnd, bSync, pairs };

	threads[0] = CreateThread(NULL, 0, entryPointSync, &syncArgs, 0, &threadIDs[0]);
	if (threads[0] == NULL)
	{
		wchar_t buf[MAX_LINE] = { 0 };
		swprintf(buf, MAX_LINE, L"Failed to create sync thread");
		logger(buf);
		return;
	}
}

DWORD CALLBACK entryPointSync(LPVOID arguments)
{
	struct SyncArguments *args = (struct SyncArguments *)arguments;
	HWND pbHwnd = args->pbHwnd;
	HWND lbSyncHwnd = args->lbSyncHwnd;
	HWND bSync = args->bSync;
	struct PairNode **pairs = args->pairs;
	struct PairNode *current = *pairs;

	if (current == NULL)
	{
		logger(L"Can't sync, list is empty");
		return 0;
	}

	int position = 0;
	int completed = 0;
	int actionCount = countPairNodes(*pairs);
	wchar_t buf[MAX_LINE] = { 0 };

	SendMessage(lbSyncHwnd, LB_RESETCONTENT, 0, 0);
	SendMessage(pbHwnd, PBM_SETPOS, 0, 0);

	for (int i = 0; i < actionCount; ++i)
	{
		if (wcscmp(current->pair.source, L"Delete file") == 0 || wcscmp(current->pair.source, L"Delete folder") == 0)
		{
			// Delete file
			if (wcscmp(current->pair.source, L"Delete file") == 0)
			{
				swprintf(buf, MAX_LINE, L"%d: Delete file %s", i, current->pair.destination);
				logger(buf);
				swprintf(buf, MAX_LINE, L"Deleted file %s OK", current->pair.destination);
#if DANGEROUS
				if (deleteFile(current->pair.destination))
					swprintf(buf, MAX_LINE, L"Deleted file %s OK", current->pair.destination);
				else
					swprintf(buf, MAX_LINE, L"Failed deleting file %s OK", current->pair.destination);
#endif
			}

			// Delete folder
			if (wcscmp(current->pair.source, L"Delete folder") == 0)
			{
				swprintf(buf, MAX_LINE, L"%d: Delete folder %s", i, current->pair.destination);
				logger(buf);
				swprintf(buf, MAX_LINE, L"Deleted folder %s OK", current->pair.destination);
#if DANGEROUS
				if (deleteFolder(current->pair.destination))
					swprintf(buf, MAX_LINE, L"Deleted folder %s OK", current->pair.destination);
				else
					swprintf(buf, MAX_LINE, L"Failed deleting folder %s", current->pair.destination);
#endif
			}
		}
		else
		{
			size_t pos = wcslen(current->pair.destination) - 1;

			// Create folder
			if (current->pair.destination[pos] == '\\')
			{
				swprintf(buf, MAX_LINE, L"%d: Create folder %s -> %s", i, current->pair.source, current->pair.destination);
				logger(buf);
				swprintf(buf, MAX_LINE, L"Created folder %s -> %s OK", current->pair.source, current->pair.destination);
#if DANGEROUS
				if (createFolder(current->pair.destination))
					swprintf(buf, MAX_LINE, L"Created folder %s -> %s OK", current->pair.source, current->pair.destination);
				else
					swprintf(buf, MAX_LINE, L"Failed creating folder %s -> %s", current->pair.source, current->pair.destination);
#endif
			}
			else // Copy file
			{
				swprintf(buf, MAX_LINE, L"%d: Copy file %s -> %s", i, current->pair.source, current->pair.destination);
				logger(buf);
				swprintf(buf, MAX_LINE, L"Copied file %s -> %s OK", current->pair.source, current->pair.destination);

#if DANGEROUS
				if (copyFile(current->pair.source, current->pair.destination))
					swprintf(buf, MAX_LINE, L"Copied file %s -> %s OK", current->pair.source, current->pair.destination);
				else
					swprintf(buf, MAX_LINE, L"Failed copying file %s -> %s", current->pair.source, current->pair.destination);
#endif
			}
		}

		int progressPosition = (int)(((float)++completed / actionCount) * 100.0f);
		SendMessage(pbHwnd, PBM_SETPOS, progressPosition, 0);
		SendMessage(lbSyncHwnd, LB_ADDSTRING, position++, (LPARAM)buf);
		current = current->next;
	}

	EnableWindow(bSync, true);
	return 0;
}
