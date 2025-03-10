#include "kitchen_sync.h"
#include "logger.h"

static DWORD CALLBACK entryPointLogger(LPVOID);
static void appendLoggerNode(struct LoggerNode **, wchar_t *);
static void deleteLoggerNode(struct LoggerNode **);

static HANDLE loggerSemaphoreHandle;
static struct LoggerNode *loggerHead = NULL;

void startLoggingThread()
{
	// reset log file
	FILE *f = fopen(LOG_FILE, "wt, ccs=UNICODE");
	if (f == NULL)
	{
		MessageBox(NULL, L"Can't open log file", L"Error", MB_ICONEXCLAMATION | MB_OK);
		return;
	}
	else
		fclose(f);

	HANDLE threads[1] = {0};
	DWORD threadIDs[1] = {0};
	int initialCount = 0;
	int threadCount = 1;
	loggerSemaphoreHandle = CreateSemaphoreEx(0, initialCount, threadCount, 0, 0, SEMAPHORE_ALL_ACCESS);
	threads[0] = CreateThread(NULL, 0, entryPointLogger, NULL, 0, &threadIDs[0]);

	if (threads[0] == NULL)
		MessageBox(NULL, L"Failed to create logger thread", L"Error", MB_ICONEXCLAMATION | MB_OK);
}

static DWORD CALLBACK entryPointLogger(LPVOID arguments)
{
	FILE *f;

	for (;;)
	{
		if (loggerHead != NULL)
		{
			f = fopen(LOG_FILE, "at, ccs=UNICODE");
			if (f == NULL)
			{
				MessageBox(NULL, L"Can't open file", L"Error", MB_ICONEXCLAMATION | MB_OK);
				return 0;
			}

			while (loggerHead != NULL)
			{
				fwprintf(f, L"%s\n", loggerHead->text);
				deleteLoggerNode(&loggerHead);
			}
			fclose(f);
		}

		WaitForSingleObjectEx(loggerSemaphoreHandle, INFINITE, FALSE);
	}
}

// add text to next node for later writing
void logger(wchar_t *text)
{
	appendLoggerNode(&loggerHead, text);
	ReleaseSemaphore(loggerSemaphoreHandle, 1, 0);
}

static void appendLoggerNode(struct LoggerNode **head_ref, wchar_t *text)
{
	struct LoggerNode *newLoggerNode = (struct LoggerNode *)malloc(sizeof(struct LoggerNode));

	if (!newLoggerNode)
	{
		MessageBox(NULL, L"Failed to allocate memory for new logger node", L"Error", MB_ICONEXCLAMATION | MB_OK);
		return;
	}

	size_t len = wcslen(text) + 1;
	newLoggerNode->text = (wchar_t *)calloc(len, sizeof(wchar_t));
	if (newLoggerNode->text)
		wcscpy_s(newLoggerNode->text, len, text);
	newLoggerNode->next = NULL;

	// if list is empty set newLoggerNode as head
	if (*head_ref == NULL)
	{
		*head_ref = newLoggerNode;
		return;
	}

	// point current last node to newLoggerNode
	struct LoggerNode *last = *head_ref;
	while (last->next != NULL)
		last = last->next;
	last->next = newLoggerNode;
}

// delete first node only
static void deleteLoggerNode(struct LoggerNode **head_ref)
{
	if (*head_ref == NULL)
		return;

	struct LoggerNode *temp = *head_ref;

	// remove head
	*head_ref = temp->next;
	free(temp->text);
	free(temp);
	return;
}
