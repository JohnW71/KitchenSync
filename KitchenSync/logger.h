#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define UNICODE
#define _UNICODE

#include <stdio.h>
#include <stdbool.h>
#include <wchar.h>
#include <windows.h>

void logger(wchar_t *);
void startLoggingThread(void);

struct LoggerNode
{
	wchar_t *text;
	struct LoggerNode *next;
};
