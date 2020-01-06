#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define UNICODE
#define _UNICODE
#define DEBUG_MODE 1

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <windows.h>
#include <CommCtrl.h>
#include <wchar.h>

#define LOG_FILE "KitchenSync.log"
#define INI_FILE "KitchenSync.ini"
#define PRJ_FILE "KitchenSync.prj"
#define MAX_LINE 512
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 1024
#define WINDOW_HEIGHT_MINIMUM 200

#if DEBUG_MODE
#define assert(expression) if(!(expression)) {*(int *)0 = 0;}
#else
#define assert(expression)
#endif

void addPair(struct PairNode **, wchar_t *, wchar_t *, LONGLONG);
void addPath(wchar_t *, wchar_t *, wchar_t *);
void appendLoggerNode(struct LoggerNode **, wchar_t *);
void appendPairNode(struct PairNode **, struct Pair, LONGLONG);
void appendProjectNode(struct ProjectNode **, wchar_t *, wchar_t *, wchar_t *);
void centerWindow(HWND);
bool copyFile(wchar_t *, wchar_t *);
int countPairNodes(struct PairNode *);
int countProjectNodes(struct ProjectNode *);
int countProjectPairs(struct ProjectNode *, wchar_t *);
bool createFolder(wchar_t *);
void cutOffLastFolder(wchar_t *);
bool deleteFile(wchar_t *);
bool deleteFolder(wchar_t *);
void deleteFilePair(struct PairNode **, wchar_t *);
void deleteFolderPair(struct ProjectNode **, wchar_t *, wchar_t *);
void deleteLoggerNode(struct LoggerNode **);
void deletePairList(struct PairNode **);
void deleteProject(struct ProjectNode **, wchar_t *);
void displayErrorBox(LPTSTR);
bool fileDateIsDifferent(FILETIME, FILETIME, FILETIME, wchar_t *);
bool fileExists(wchar_t *);
void fillInProject(struct Project *, wchar_t *, wchar_t *, wchar_t *);
void fillProjectListbox(HWND, struct ProjectNode **);
void fillSyncListbox(HWND, struct PairNode **);
void findProjectName(HWND, LRESULT, wchar_t *);
bool folderExists(wchar_t *);
LONGLONG getFileSize(wchar_t *);
bool hiddenFile(wchar_t *);
bool isProjectName(wchar_t *, int);
bool listSubFolders(HWND, wchar_t *);
void loadProjects(HWND, char *, struct ProjectNode **);
void logger(wchar_t *);
void previewFolderPair(HWND, HWND, struct PairNode **, struct Project *);
void previewProject(HWND, HWND, struct ProjectNode **, struct PairNode **, wchar_t *);
bool readOnly(wchar_t *);
void readSettings(HWND, char *);
void reloadFolderPairs(HWND, HWND, struct ProjectNode *, wchar_t *);
void renameProject(struct ProjectNode **, wchar_t *, wchar_t *);
void replaceFolderPair(struct ProjectNode **, wchar_t *, wchar_t *, wchar_t *, wchar_t *);
void saveProjects(char *, struct ProjectNode **);
bool setNormalFile(wchar_t *);
void shutDown(HWND, struct ProjectNode **);
void sortProjectNodes(struct ProjectNode **);
void sortPairNodes(struct PairNode **);
void splitPair(wchar_t *, wchar_t *, wchar_t *, size_t);
void startLoggingThread(void);
void startPreviewScanThread(HWND, HWND, HWND, HWND, HWND, struct ProjectNode **, struct PairNode **, wchar_t [MAX_LINE], LRESULT);
void synchronizeFiles(HWND, HWND, HWND, HWND, struct PairNode **);
void writeSettings(HWND, char *);

struct Pair
{
	wchar_t source[MAX_LINE];
	wchar_t destination[MAX_LINE];
	LONGLONG filesize; // __int64
};

struct PairNode
{
	struct Pair pair;
	struct PairNode *previous;
	struct PairNode *next;
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

struct LoggerNode
{
	wchar_t text[MAX_LINE];
	struct LoggerNode *next;
};

struct PreviewScanArguments
{
	HWND pbHwnd;
	HWND lbSyncHwnd;
	HWND lbProjectsHwnd;
	HWND bSync;
	HWND tabHwnd;
	struct ProjectNode **project;
	struct PairNode **pairs;
	LRESULT selectedRow;
	wchar_t selectedRowText[MAX_LINE];
};

struct PreviewFolderArguments
{
	HWND hwnd;
	struct PairNode **pairs;
	struct Project *project;
};

struct SyncArguments
{
	HWND pbHwnd;
	HWND lbSyncHwnd;
	HWND bSync;
	HWND tabHwnd;
	struct PairNode **pairs;
};
