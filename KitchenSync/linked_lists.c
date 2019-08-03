#include "kitchen_sync.h"

static void deletePairNode(struct PairNode **, int);
static void deleteProjectNode(struct ProjectNode **, int);
static void deleteProjectList(struct ProjectNode **);

// append a new node at the end
void appendPairNode(struct PairNode **head_ref, struct Pair pair)
{
#if DEV_MODE
	writeFileW(LOG_FILE, L"appendPairNode()");
#endif

	struct PairNode *newPairNode = (struct PairNode *)malloc(sizeof(struct PairNode));

	if (!newPairNode)
	{
		writeFileW(LOG_FILE, L"Failed to allocate memory for new pair");
		return;
	}

	if (wcscpy_s(newPairNode->pair.source, MAX_LINE, pair.source))
	{
		writeFileW(LOG_FILE, L"Failed copying new source");
		writeFileW(LOG_FILE, pair.source);
		free(newPairNode);
		return;
	}

	if (wcscpy_s(newPairNode->pair.destination, MAX_LINE, pair.destination))
	{
		writeFileW(LOG_FILE, L"Failed copying new destination");
		writeFileW(LOG_FILE, pair.source);
		free(newPairNode);
		return;
	}

	newPairNode->next = NULL;

	// if list is empty set newPairNode as head
	if (*head_ref == NULL)
	{
		*head_ref = newPairNode;
		return;
	}

	// divert current last node to newPairNode
	struct PairNode *last = *head_ref;
	while (last->next != NULL)
		last = last->next;
	last->next = newPairNode;
}

// append a new node at the end
void appendProjectNode(struct ProjectNode **head_ref, struct Project project)
{
#if DEV_MODE
	writeFileW(LOG_FILE, L"appendProjectNode()");
#endif

	struct ProjectNode *newProjectNode = (struct ProjectNode *)malloc(sizeof(struct ProjectNode));

	if (!newProjectNode)
	{
		writeFileW(LOG_FILE, L"Failed to allocate memory for new project");
		return;
	}

	if (wcscpy_s(newProjectNode->project.name, MAX_LINE, project.name))
	{
		writeFileW(LOG_FILE, L"Failed copying new project name");
		writeFileW(LOG_FILE, project.name);
		free(newProjectNode);
		return;
	}

	if (wcscpy_s(newProjectNode->project.pair.source, MAX_LINE, project.pair.source))
	{
		writeFileW(LOG_FILE, L"Failed copying new project source");
		writeFileW(LOG_FILE, project.pair.source);
		free(newProjectNode);
		return;
	}

	if (wcscpy_s(newProjectNode->project.pair.destination, MAX_LINE, project.pair.destination))
	{
		writeFileW(LOG_FILE, L"Failed copying new project destination");
		writeFileW(LOG_FILE, project.pair.destination);
		free(newProjectNode);
		return;
	}

	newProjectNode->next = NULL;

	// if list is empty set newProjectNode as head
	if (*head_ref == NULL)
	{
		*head_ref = newProjectNode;
		return;
	}

	// point current last node to newProjectNode
	struct ProjectNode *last = *head_ref;
	while (last->next != NULL)
		last = last->next;
	last->next = newProjectNode;
}

// delete entire project by name
void deleteProject(struct ProjectNode **head_ref, wchar_t *projectName)
{
#if DEV_MODE
	writeFileW(LOG_FILE, L"deleteProject()");
#endif

	sortProjectNodes(head_ref);
	struct ProjectNode *current = *head_ref;
	int i = 0;

	do
	{
		if (wcscmp(current->project.name, projectName) == 0)
		{
			deleteProjectNode(head_ref, i);
			if (*head_ref != NULL)
				current = *head_ref;
			i = 0;
		}
		else
		{
			current = current->next;
			++i;
		}
	}
	while (*head_ref != NULL && current != NULL);
}

// delete folder pair by name
void deleteFolderPair(struct ProjectNode **head_ref, wchar_t *folderPair, wchar_t *projectName)
{
#if DEV_MODE
	writeFileW(LOG_FILE, L"deleteFolderPair()");
#endif

	sortProjectNodes(head_ref);
	size_t length = wcslen(folderPair);

	if (length > 0)
	{
		int pos = 0;
		while (pos < length && folderPair[pos] != '>')
			++pos;

		int sourceEnd = pos - 2;
		int destStart = pos + 2;
		wchar_t src[MAX_LINE] = {0};
		wchar_t dst[MAX_LINE] = {0};

		wcsncpy_s(src, MAX_LINE, folderPair, sourceEnd);
		wcsncpy_s(dst, MAX_LINE, folderPair + destStart, length);

		struct ProjectNode *current = *head_ref;
		int i = 0;

		while (*head_ref != NULL && current != NULL)
		{
			if (wcscmp(current->project.name, projectName) == 0 &&
				wcscmp(current->project.pair.source, src) == 0 &&
				wcscmp(current->project.pair.destination, dst) == 0)
			{
				deleteProjectNode(head_ref, i);
				return;
			}

			current = current->next;
			++i;
		}
	}
}

// delete folder pair by name
void deleteFilePair(struct PairNode **head_ref, wchar_t *filePair)
{
#if DEV_MODE
	writeFileW(LOG_FILE, L"deleteFilePair()");
#endif

	//sortProjectNodes(head_ref);
	size_t length = wcslen(filePair);

	if (length > 0)
	{
		int pos = 0;
		while (pos < length && filePair[pos] != '>')
			++pos;

		int sourceEnd = pos - 2;
		int destStart = pos + 2;
		wchar_t src[MAX_LINE] = {0};
		wchar_t dst[MAX_LINE] = {0};

		wcsncpy_s(src, MAX_LINE, filePair, sourceEnd);
		wcsncpy_s(dst, MAX_LINE, filePair + destStart, length);

		struct PairNode *current = *head_ref;
		int i = 0;

		while (*head_ref != NULL && current != NULL)
		{
			if (wcscmp(current->pair.source, src) == 0 &&
				wcscmp(current->pair.destination, dst) == 0)
			{
				deletePairNode(head_ref, i);
				return;
			}

			current = current->next;
			++i;
		}
	}
}

// replace existing folder pair
void replaceFolderPair(struct ProjectNode **head_ref, wchar_t *projectName, wchar_t *oldFolderPair, wchar_t *newSrc, wchar_t *newDst)
{
#if DEV_MODE
	writeFileW(LOG_FILE, L"replaceFolderPair()");
#endif

	sortProjectNodes(head_ref);
	size_t oldLength = wcslen(oldFolderPair);

	assert(oldLength > 0);

	if (oldLength > 0)
	{
		// extract old folder pair
		int pos = 0;
		while (pos < oldLength && oldFolderPair[pos] != '>')
			++pos;

		int oldSourceEnd = pos - 2;
		int oldDestStart = pos + 2;
		wchar_t oldSrc[MAX_LINE] = {0};
		wchar_t oldDst[MAX_LINE] = {0};

		wcsncpy_s(oldSrc, MAX_LINE, oldFolderPair, oldSourceEnd);
		wcsncpy_s(oldDst, MAX_LINE, oldFolderPair + oldDestStart, oldLength);

		// find & replace folder pair
		struct ProjectNode *current = *head_ref;

		while (*head_ref != NULL && current != NULL)
		{
			if (wcscmp(current->project.name, projectName) == 0 &&
				wcscmp(current->project.pair.source, oldSrc) == 0 &&
				wcscmp(current->project.pair.destination, oldDst) == 0)
			{
				wcscpy_s(current->project.pair.source, MAX_LINE, newSrc);
				wcscpy_s(current->project.pair.destination, MAX_LINE, newDst);
				return;
			}

			current = current->next;
		}
	}
}

// Given a reference (pointer to pointer) to the head of a list
// and a position, deletes the node at the given position
static void deletePairNode(struct PairNode **head_ref, int position)
{
	if (*head_ref == NULL)
		return;

	// store current head node
	struct PairNode *temp = *head_ref;

	// if head is to be removed
	if (position == 0)
	{
		*head_ref = temp->next;
		free(temp);
		return;
	}

	// find previous node of the node to be deleted
	for (int i = 0; temp != NULL && i < position - 1; ++i)
		temp = temp->next;

	// if position is off the end
	if (temp == NULL || temp->next == NULL)
		return;

	// node temp->next is the node to be deleted
	// store pointer to the next node after the node to be deleted
	struct PairNode *next = temp->next->next;

	// unlink the node from the list
	free(temp->next);

	// link to next node
	temp->next = next;
}

// Given a reference (pointer to pointer) to the head of a list
// and a position, deletes the node at the given position
static void deleteProjectNode(struct ProjectNode **head_ref, int position)
{
	if (*head_ref == NULL)
		return;

	// store current head node
	struct ProjectNode *temp = *head_ref;

	// head is to be removed
	if (position == 0)
	{
		*head_ref = temp->next;
		free(temp);
		return;
	}

	// find previous node of the node to be deleted
	for (int i = 0; temp != NULL && i < position - 1; ++i)
		temp = temp->next;

	// if position is off the end
	if (temp == NULL || temp->next == NULL)
		return;

	// node temp->next is the node to be deleted
	// store pointer to the next node after the node to be deleted
	struct ProjectNode *next = temp->next->next;

	// unlink the node from the list
	free(temp->next);

	// link to next node
	temp->next = next;
}

// delete the entire linked list
void deletePairList(struct PairNode **head_ref)
{
	// de-reference head_ref to get the real head
	struct PairNode *current = *head_ref;
	struct PairNode *next;

	while (current != NULL)
	{
		next = current->next;
		free(current);
		current = next;
	}

	*head_ref = NULL;
}

#if 0
// delete the entire linked list
static void deleteProjectList(struct ProjectNode **head_ref)
{
	// de-reference head_ref to get the real head
	struct ProjectNode *current = *head_ref;
	struct ProjectNode *next;

	while (current != NULL)
	{
		next = current->next;
		free(current);
		current = next;
	}

	*head_ref = NULL;
}

// count nodes in list
static int countPairNodes(struct PairNode *head)
{
#if DEV_MODE
	writeFileW(LOG_FILE, L"countPairNodes()");
#endif

	int count = 0;
	struct PairNode *current = head;

	while (current != NULL)
	{
		++count;
		current = current->next;
	}
	return count;
}
#endif

// count nodes in list
int countProjectNodes(struct ProjectNode *head)
{
#if DEV_MODE
	writeFileW(LOG_FILE, L"countProjectNodes()");
#endif

	int count = 0;
	struct ProjectNode *current = head;

	while (current != NULL)
	{
		++count;
		current = current->next;
	}
	return count;
}

// sort list
void sortProjectNodes(struct ProjectNode **head_ref)
{
#if DEV_MODE
	writeFileW(LOG_FILE, L"sortProjectNodes()");
#endif

	if (*head_ref == NULL)
	{
		writeFileW(LOG_FILE, L"Can't sort empty list");
		return;
	}

	struct ProjectNode *head = *head_ref;

	if (head->next == NULL)
	{
		writeFileW(LOG_FILE, L"Can't sort only 1 entry");
		return;
	}

	bool changed;

	do
	{
		changed = false;

		// swap head & second nodes
		if (wcscmp(head->project.name, head->next->project.name) > 0)
		{
			struct ProjectNode *temp = head->next;

			head->next = head->next->next;
			temp->next = head;
			*head_ref = temp;
			head = *head_ref;
			changed = true;
		}

		struct ProjectNode *previous = head;
		struct ProjectNode *current = head->next;

		if (current->next == NULL)
		{
			writeFileW(LOG_FILE, L"Can't sort more, only 2 entries");
			return;
		}

		// swap folder pairs
		while (current != NULL && current->next != NULL)
		{
			if (wcscmp(current->project.name, current->next->project.name) > 0 ||
				(wcscmp(current->project.name, current->next->project.name) == 0 &&
					wcscmp(current->project.pair.source, current->next->project.pair.source) > 0))
			{
				struct ProjectNode *temp = current->next;

				if (current->next->next != NULL)
					current->next = current->next->next;
				else
					current->next = NULL;
				temp->next = current;
				previous->next = temp;

				changed = true;
			}
			else
				current = current->next;

			previous = previous->next;
		}
	}
	while (changed);
}

void previewProject(HWND hwnd, struct ProjectNode **head_ref, struct PairNode **pairs, wchar_t *projectName)
{
#if DEV_MODE
	writeFileW(LOG_FILE, L"previewProject()");
#endif

	struct ProjectNode *current = *head_ref;

	do
	{
		if (wcscmp(current->project.name, projectName) == 0)
		{
			struct Project project;
			wcscpy_s(project.name, MAX_LINE, projectName);
			wcscpy_s(project.pair.source, MAX_LINE, current->project.pair.source);
			wcscpy_s(project.pair.destination, MAX_LINE, current->project.pair.destination);

			previewFolderPair(hwnd, pairs, project);
		}
		current = current->next;
	}
	while (*head_ref != NULL && current != NULL);
}

void previewFolderPair(HWND hwnd, struct PairNode **pairs, struct Project project)
{
#if DEV_MODE
	writeFileW(LOG_FILE, L"previewFolderPair()");
#endif

	listFolderContent(hwnd, pairs, project.pair.source);
}
