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

#define LOG_FILE "kitchen_sync.log"
#define INI_FILE "kitchen_sync.ini"
#define PROJECTS "kitchen_sync.prj"
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
//void clearArray(char *, int);
void clearArrayW(wchar_t *, int);
//void clearNewlines(char *, int);
//void clearNewlinesW(wchar_t *, int);
//void writeFile(char *, char *);
void writeSettings(char *, HWND);
void readSettings(char *, HWND);
void loadProjects(char *, struct ProjectNode **, HWND);
void saveProjects(char *, struct ProjectNode **);
void writeFileW(char *, wchar_t *);
void appendPairNode(struct PairNode **, struct Pair);
void appendProjectNode(struct ProjectNode **, struct Project);
void deleteProject(struct ProjectNode **, wchar_t *);
void deleteFolderPair(struct ProjectNode **, wchar_t *, wchar_t *);
void replaceFolderPair(struct ProjectNode **, wchar_t *, wchar_t *, wchar_t *, wchar_t *);
void sortProjectNodes(struct ProjectNode **);
void fillListbox(HWND, struct ProjectNode **);
void fillSyncListbox(HWND, struct PairNode **);
void findProjectName(HWND, LRESULT, wchar_t *, wchar_t *);
void reloadFolderPairs(struct ProjectNode *, wchar_t *, HWND, HWND);
void previewProject(HWND, struct ProjectNode **, struct PairNode **, wchar_t *);
void previewFolderPair(HWND, struct PairNode **, struct Project);
void deleteFilePair(struct PairNode **, wchar_t *);
void deletePairList(struct PairNode **);
bool isProjectName(wchar_t *, int);
int countPairNodes(struct PairNode *);
int countProjectNodes(struct ProjectNode *);
int listFolders(HWND, wchar_t *);
int listFolderContent(HWND, struct PairNode **, wchar_t *);

struct Pair
{
	wchar_t source[MAX_LINE];
	wchar_t destination[MAX_LINE];
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
