#include "kitchen_sync.h"

#define DANGEROUS 0

DWORD CALLBACK entryPointSync(LPVOID);

void synchronizeFiles(HWND pbHwnd, HWND lbSyncHwnd, HWND bSync, HWND tabHwnd, struct PairNode **pairs)
{
	HANDLE threads[1];
	DWORD threadIDs[1];

	struct SyncArguments syncArgs = { pbHwnd, lbSyncHwnd, bSync, tabHwnd, pairs };

	threads[0] = CreateThread(NULL, 0, entryPointSync, &syncArgs, 0, &threadIDs[0]);
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

	if (args->pairs == NULL)
	{
		wchar_t buf[MAX_LINE] = L"Can't sync, args->pairs list is empty";
		logger(buf);
		MessageBox(NULL, buf, L"Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	struct PairNode **pairs = args->pairs;
	struct PairNode *current = *pairs;

	if (current == NULL)
	{
		wchar_t buf[MAX_LINE] = L"Can't sync, current pointer to pairs list is null";
		logger(buf);
		MessageBox(NULL, buf, L"Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	int position = 0;
	int completed = 0;
	int actionCount = countPairNodes(*pairs);
	assert(actionCount > 0);
	wchar_t buf[MAX_LINE] = { 0 };

	SendMessage(lbSyncHwnd, LB_RESETCONTENT, 0, 0);
	SendMessage(pbHwnd, PBM_SETPOS, 0, 0);

	startCount();

	for (int i = 0; i < actionCount; ++i)
	{
		if (wcscmp(current->pair.source, L"Delete folder") == 0)
			continue;

		if (wcscmp(current->pair.source, L"Delete file") == 0)
		{
			// delete file
#if DANGEROUS
			if (deleteFile(current->pair.destination))
			{
				//swprintf(buf, MAX_LINE, L"Deleted file %s OK", current->pair.destination);
				wcscpy_s(buf, MAX_LINE, L"Deleted file ");
				wcscat(buf, current->pair.destination);
				wcscat(buf, L" OK");
			}
			else
			{
				//swprintf(buf, MAX_LINE, L"Failed deleting file %s OK", current->pair.destination);
				wcscpy_s(buf, MAX_LINE, L"Failed deleting file ");
				wcscat(buf, current->pair.destination);
			}
#else
	//swprintf(buf, MAX_LINE, L"Deleted file %s OK", current->pair.destination);
	wcscpy_s(buf, MAX_LINE, L"Deleted file ");
	wcscat(buf, current->pair.destination);
	wcscat(buf, L" OK");
#endif
		}
		else
		{
			// create folder
			size_t lastPosition = wcslen(current->pair.destination) - 1;
			if (current->pair.destination[lastPosition] == '\\')
			{
#if DANGEROUS
				if (createFolder(current->pair.destination))
				{
					//swprintf(buf, MAX_LINE, L"Created folder %s -> %s OK", current->pair.source, current->pair.destination);
					wcscpy_s(buf, MAX_LINE, L"Created folder ");
					wcscat(buf, current->pair.source);
					wcscat(buf, L" -> ");
					wcscat(buf, current->pair.destination);
					wcscat(buf, L" OK");
				}
				else
				{
					//swprintf(buf, MAX_LINE, L"Failed creating folder %s -> %s", current->pair.source, current->pair.destination);
					wcscpy_s(buf, MAX_LINE, L"Failed creating folder ");
					wcscat(buf, current->pair.source);
					wcscat(buf, L" -> ");
					wcscat(buf, current->pair.destination);
				}
#else
	//swprintf(buf, MAX_LINE, L"Created folder %s -> %s OK", current->pair.source, current->pair.destination);
	wcscpy_s(buf, MAX_LINE, L"Created folder ");
	wcscat(buf, current->pair.source);
	wcscat(buf, L" -> ");
	wcscat(buf, current->pair.destination);
	wcscat(buf, L" OK");
#endif
			}
			else // copy file
			{
#if DANGEROUS
				if (copyFile(current->pair.source, current->pair.destination))
				{
					//swprintf(buf, MAX_LINE, L"Copied file %s -> %s OK", current->pair.source, current->pair.destination);
					wcscpy_s(buf, MAX_LINE, L"Copied file ");
					wcscat(buf, current->pair.source);
					wcscat(buf, L" -> ");
					wcscat(buf, current->pair.destination);
					wcscat(buf, L" OK");
				}
				else
				{
					//swprintf(buf, MAX_LINE, L"Failed copying file %s -> %s", current->pair.source, current->pair.destination);
					wcscpy_s(buf, MAX_LINE, L"Failed copying file ");
					wcscat(buf, current->pair.source);
					wcscat(buf, L" -> ");
					wcscat(buf, current->pair.destination);
			}
#else
	//swprintf(buf, MAX_LINE, L"Copied file %s -> %s OK", current->pair.source, current->pair.destination);
	wcscpy_s(buf, MAX_LINE, L"Copied file ");
	wcscat(buf, current->pair.source);
	wcscat(buf, L" -> ");
	wcscat(buf, current->pair.destination);
	wcscat(buf, L" OK");
#endif
			}
		}

		logger(buf);
		int progressPosition = (int)(((float)++completed / actionCount) * 100.0f);
		SendMessage(pbHwnd, PBM_SETPOS, progressPosition, 0);
		SendMessage(lbSyncHwnd, LB_ADDSTRING, position++, (LPARAM)buf);
		current = current->next;
	}

	// set current to last pair
	current = *pairs;
	for (int i = 0; i < actionCount; ++i)
	{
#if DEBUG_MODE
	swprintf(buf, MAX_LINE, L"Action: %s -> %s", current->pair.source, current->pair.destination);
	logger(buf);
#endif
		if (current->next != NULL)
			current = current->next;
	}

	// search backwards for folder deletions
	while (wcscmp(current->pair.source, L"Delete folder") == 0)
	{
		// delete folder
#if DANGEROUS
		if (deleteFolder(current->pair.destination))
		{
			//swprintf(buf, MAX_LINE, L"Deleted folder %s OK", current->pair.destination);
			wcscpy_s(buf, MAX_LINE, L"Deleted folder ");
			wcscat(buf, current->pair.destination);
			wcscat(buf, L" OK");
		}
		else
		{
			//swprintf(buf, MAX_LINE, L"Failed deleting folder %s", current->pair.destination);
			wcscpy_s(buf, MAX_LINE, L"Failed deleting folder ");
			wcscat(buf, current->pair.destination);
		}
#else
	//swprintf(buf, MAX_LINE, L"Deleted folder %s OK", current->pair.destination);
	wcscpy_s(buf, MAX_LINE, L"Deleted folder ");
	wcscat(buf, current->pair.destination);
	wcscat(buf, L" OK");
#endif

		logger(buf);
		int progressPosition = (int)(((float)++completed / actionCount) * 100.0f);
		SendMessage(pbHwnd, PBM_SETPOS, progressPosition, 0);
		SendMessage(lbSyncHwnd, LB_ADDSTRING, position++, (LPARAM)buf);

		if (current->previous != NULL)
			current = current->previous;
		else
			break;
	}

	endCount(L"Sync");
	EnableWindow(bSync, false);
	EnableWindow(tabHwnd, true);
	deletePairList(pairs);
	return 0;
}
