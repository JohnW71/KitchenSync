#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define UNICODE
#define _UNICODE
#define DEV_MODE 1

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

#define arrayCount(array) (sizeof(array) / sizeof((array)[0]))

#if DEV_MODE
#define assert(expression) if(!(expression)) {*(int *)0 = 0;}
#else
#define assert(expression)
#endif

void shutDown(HWND, struct ProjectNode **);
void centerWindow(HWND);
void writeSettings(HWND, char *);
void readSettings(HWND, char *);
void loadProjects(HWND, char *, struct ProjectNode **);
void saveProjects(char *, struct ProjectNode **);
void writeFileW(char *, wchar_t *);
void appendPairNode(struct PairNode **, struct Pair, LONGLONG);
void appendProjectNode(struct ProjectNode **, struct Project);
void deleteProject(struct ProjectNode **, wchar_t *);
void deleteFolderPair(struct ProjectNode **, wchar_t *, wchar_t *);
void replaceFolderPair(struct ProjectNode **, wchar_t *, wchar_t *, wchar_t *, wchar_t *);
void sortProjectNodes(struct ProjectNode **);
void sortPairNodes(struct PairNode **);
void fillListbox(HWND, struct ProjectNode **);
void fillSyncListbox(HWND, struct PairNode **);
void findProjectName(HWND, LRESULT, wchar_t *);
void reloadFolderPairs(HWND, HWND, struct ProjectNode *, wchar_t *);
void previewProject(HWND, struct ProjectNode **, struct PairNode **, wchar_t *);
//void previewFolderPair(HWND, struct PairNode **, struct Project);
void previewFolderPairTest(HWND, struct PairNode **, struct Project *);
void deleteFilePair(struct PairNode **, wchar_t *);
void deletePairList(struct PairNode **);
void splitPair(wchar_t *, wchar_t *, wchar_t *, size_t);
void addPair(struct PairNode **, wchar_t *, wchar_t *, LONGLONG);
bool isProjectName(wchar_t *, int);
int countPairNodes(struct PairNode *);
int countProjectNodes(struct ProjectNode *);
int listSubFolders(HWND, wchar_t *);
//int listTreeContent(struct PairNode **, wchar_t *, wchar_t *);

struct Pair
{
	wchar_t source[MAX_LINE];
	wchar_t destination[MAX_LINE];
	LONGLONG filesize; // __int64
};

struct PairNode
{
	struct Pair pair;
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
