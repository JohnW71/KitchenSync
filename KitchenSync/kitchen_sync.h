#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define UNICODE
#define _UNICODE
#define DEBUG_MODE 0

#include <stdio.h>
#include <stdint.h> // int64_t
#include <stdlib.h>
#include <stdbool.h>
#include <windows.h>
#include <CommCtrl.h>
#include <wchar.h>
#include "logger.h"

#define LOG_FILE "KitchenSync.log"
#define INI_FILE "KitchenSync.ini"
#define PRJ_FILE "KitchenSync.prj"
#define MAX_LINE 512
// 0.75MB for pairs index array
#define MAX_PAIRS 100000
#define FOLDER_PAIR_SIZE MAX_LINE*3
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 1024
#define WINDOW_HEIGHT_MINIMUM 200

#if DEBUG_MODE
uint64_t firstCount;
uint64_t beginCycleCount;
uint64_t endCycleCount;

uint64_t results[10000];
int resultCount;
#endif
int pairCount;

#if DEBUG_MODE
#define assert(expression) if(!(expression)) {*(int *)0 = 0;}
#else
#define assert(expression)
#endif

void addPair(struct Pair **, wchar_t *, wchar_t *, LONGLONG);
void addPath(wchar_t *, wchar_t *, wchar_t *);
void appendProjectNode(struct ProjectNode **, wchar_t *, wchar_t *, wchar_t *);
void centerWindow(HWND);
bool copyFile(wchar_t *, wchar_t *);
int countProjectNodes(struct ProjectNode *);
int countProjectPairs(struct ProjectNode *, wchar_t *);
bool createFolder(wchar_t *);
void cutOffLastFolder(wchar_t *);
void deleteAllPairs(struct Pair **);
bool deleteFile(wchar_t *);
bool deleteFolder(wchar_t *);
void deleteFilePair(struct Pair **, wchar_t *);
void deleteFolderPair(struct ProjectNode **, wchar_t *, wchar_t *);
void deleteProject(struct ProjectNode **, wchar_t *);
void displayErrorBox(LPTSTR);
void endCount(wchar_t *);
bool fileDateIsDifferent(FILETIME, FILETIME, FILETIME, wchar_t *);
bool fileExists(wchar_t *);
void fillInProject(struct Project *, wchar_t *, wchar_t *, wchar_t *);
void fillProjectListbox(HWND, struct ProjectNode **);
void fillSyncListbox(HWND, struct Pair **);
int findPair(struct Pair **, wchar_t *, wchar_t *);
void findProjectName(HWND, LRESULT, wchar_t *);
bool folderExists(wchar_t *);
LONGLONG getDriveSpace(int);
DWORD getDriveStrings(DWORD, wchar_t *);
LONGLONG getFileSize(wchar_t *);
bool hiddenFile(wchar_t *);
bool isProjectName(wchar_t *, int);
bool listSubFolders(HWND, wchar_t *);
void loadProjects(HWND, char *, struct ProjectNode **);
void previewFolderPair(HWND, HWND, struct Pair **, struct Project *, wchar_t *, DWORD);
void previewProject(HWND, HWND, HWND, struct ProjectNode **, struct Pair **, wchar_t *);
bool readOnly(wchar_t *);
void readSettings(HWND, char *);
void reloadFolderPairs(HWND, HWND, struct ProjectNode *, wchar_t *);
void renameProject(struct ProjectNode **, wchar_t *, wchar_t *);
void replaceFolderPair(struct ProjectNode **, wchar_t *, wchar_t *, wchar_t *, wchar_t *);
void saveProjects(char *, struct ProjectNode **);
bool setNormalFile(wchar_t *);
void shutDown(HWND, struct ProjectNode **);
void sortProjectNodes(struct ProjectNode **);
void sortPairs(struct Pair **);
void splitPair(wchar_t *, wchar_t *, wchar_t *, size_t);
void startCount(void);
void startPreviewScanThread(HWND, HWND, HWND, HWND, HWND, struct ProjectNode **, struct Pair **, wchar_t [MAX_LINE], LRESULT);
void synchronizeFiles(HWND, HWND, HWND, HWND, struct Pair **);
void writeSettings(HWND, char *);

struct Pair
{
	wchar_t source[MAX_LINE];
	wchar_t destination[MAX_LINE];
	LONGLONG filesize; // __int64
};

struct Project
{
	wchar_t name[MAX_LINE];
	struct Pair pair;
};

struct ProjectNode
{
	struct Project project;
	struct ProjectNode *next;
};

struct PreviewScanArguments
{
	HWND pbHwnd;
	HWND lbSyncHwnd;
	HWND lbProjectsHwnd;
	HWND bSync;
	HWND tabHwnd;
	struct ProjectNode **project;
	struct Pair **pairIndex;
	LRESULT selectedRow;
	wchar_t selectedRowText[MAX_LINE];
};

struct PreviewFolderArguments
{
	HWND hwnd;
	struct Pair **pairIndex;
	struct Project *project;
};

struct SyncArguments
{
	HWND pbHwnd;
	HWND lbSyncHwnd;
	HWND bSync;
	HWND tabHwnd;
	struct Pair **pairIndex;
};

struct Settings
{
	bool skipDesktopIni;
	bool skipDocumentLinks;
} settings;
