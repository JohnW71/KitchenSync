#include "kitchen_sync.h"

static void deletePairNode(struct PairNode **, int);
static void deleteProjectNode(struct ProjectNode **, int);

void addPair(struct PairNode **pairs, wchar_t *source, wchar_t *destination, LONGLONG size)
{
	struct Pair pair = { 0 };
	wcscpy_s(pair.source, MAX_LINE, source);
	wcscpy_s(pair.destination, MAX_LINE, destination);
	appendPairNode(pairs, pair, size);
}

void appendPairNode(struct PairNode **head_ref, struct Pair pair, LONGLONG filesize)
{
	struct PairNode *newPairNode = (struct PairNode *)malloc(sizeof(struct PairNode));

	if (!newPairNode)
	{
		wchar_t buf[MAX_LINE] = L"Failed to allocate memory for new pair";
		logger(buf);
		MessageBox(NULL, buf, L"Error", MB_ICONEXCLAMATION | MB_OK);
		return;
	}

	if (wcscpy_s(newPairNode->pair.source, MAX_LINE, pair.source))
	{
		logger(L"Failed copying new source");
		logger(pair.source);
		free(newPairNode);
		return;
	}

	if (wcscpy_s(newPairNode->pair.destination, MAX_LINE, pair.destination))
	{
		logger(L"Failed copying new destination");
		logger(pair.destination);
		free(newPairNode);
		return;
	}

	newPairNode->pair.filesize = filesize;
	newPairNode->previous = NULL;
	newPairNode->next = NULL;

	// if list is empty set newPairNode as head
	if (*head_ref == NULL)
	{
		*head_ref = newPairNode;
		return;
	}

	// point current last node to newPairNode
	struct PairNode *last = *head_ref;
	while (last->next != NULL)
		last = last->next;
	last->next = newPairNode;
	newPairNode->previous = last;
}

void appendProjectNode(struct ProjectNode **head_ref, wchar_t *projectName, wchar_t *sourceFolder, wchar_t *destFolder)
{
	struct Project project = { 0 };
	fillInProject(&project, projectName, sourceFolder, destFolder);

	struct ProjectNode *newProjectNode = (struct ProjectNode *)malloc(sizeof(struct ProjectNode));

	if (!newProjectNode)
	{
		wchar_t buf[MAX_LINE] = L"Failed to allocate memory for new project";
		logger(buf);
		MessageBox(NULL, buf, L"Error", MB_ICONEXCLAMATION | MB_OK);
		return;
	}

	if (wcscpy_s(newProjectNode->project.name, MAX_LINE, project.name))
	{
		logger(L"Failed copying new project name");
		logger(project.name);
		free(newProjectNode);
		return;
	}

	if (wcscpy_s(newProjectNode->project.pair.source, MAX_LINE, project.pair.source))
	{
		logger(L"Failed copying new project source");
		logger(project.pair.source);
		free(newProjectNode);
		return;
	}

	if (wcscpy_s(newProjectNode->project.pair.destination, MAX_LINE, project.pair.destination))
	{
		logger(L"Failed copying new project destination");
		logger(project.pair.destination);
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

// delete single folder pair by name
void deleteFolderPair(struct ProjectNode **head_ref, wchar_t *folderPair, wchar_t *projectName)
{
	sortProjectNodes(head_ref);
	size_t length = wcslen(folderPair);
	assert(length > 0);

	if (length > 0)
	{
		wchar_t src[MAX_LINE] = { 0 };
		wchar_t dst[MAX_LINE] = { 0 };
		splitPair(folderPair, src, dst, length);

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

// delete single file pair by name
void deleteFilePair(struct PairNode **head_ref, wchar_t *filePair)
{
	size_t length = wcslen(filePair);
	assert(length > 0);

	if (length > 0)
	{
		wchar_t src[MAX_LINE] = { 0 };
		wchar_t dst[MAX_LINE] = { 0 };
		splitPair(filePair, src, dst, length);

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

// replace existing folder pair with edited pair
void replaceFolderPair(struct ProjectNode **head_ref, wchar_t *projectName, wchar_t *folderPair, wchar_t *newSrc, wchar_t *newDst)
{
	sortProjectNodes(head_ref);
	size_t length = wcslen(folderPair);
	assert(length > 0);

	if (length > 0)
	{
		// extract old folder pair
		wchar_t src[MAX_LINE] = { 0 };
		wchar_t dst[MAX_LINE] = { 0 };
		splitPair(folderPair, src, dst, length);

		// find & replace folder pair
		struct ProjectNode *current = *head_ref;

		while (*head_ref != NULL && current != NULL)
		{
			if (wcscmp(current->project.name, projectName) == 0 &&
				wcscmp(current->project.pair.source, src) == 0 &&
				wcscmp(current->project.pair.destination, dst) == 0)
			{
				wcscpy_s(current->project.pair.source, MAX_LINE, newSrc);
				wcscpy_s(current->project.pair.destination, MAX_LINE, newDst);
				return;
			}

			current = current->next;
		}
	}
}

static void deletePairNode(struct PairNode **head_ref, int position)
{
	if (*head_ref == NULL)
		return;

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

	struct PairNode *next = temp->next->next;
	free(temp->next);
	temp->next = next;
}

static void deleteProjectNode(struct ProjectNode **head_ref, int position)
{
	if (*head_ref == NULL)
		return;

	struct ProjectNode *temp = *head_ref;

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

	struct ProjectNode *next = temp->next->next;
	free(temp->next);
	temp->next = next;
}

// delete the entire linked list
void deletePairList(struct PairNode **head_ref)
{
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
void deleteProjectList(struct ProjectNode **head_ref)
{
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
#endif

// count nodes in list
int countPairNodes(struct PairNode *head)
{
	int count = 0;
	struct PairNode *current = head;

	while (current != NULL)
	{
		++count;
		current = current->next;
	}
	return count;
}

// replace all occurrences of old project name with new name
void renameProject(struct ProjectNode **head_ref, wchar_t *oldName, wchar_t *newName)
{
	struct ProjectNode *current = *head_ref;

	while (current != NULL)
	{
		if (wcscmp(current->project.name, oldName) == 0)
			wcscpy_s(current->project.name, MAX_LINE, newName);
		current = current->next;
	}
}

int countProjectNodes(struct ProjectNode *head)
{
	int count = 0;
	struct ProjectNode *current = head;

	while (current != NULL)
	{
		++count;
		current = current->next;
	}
	return count;
}

int countProjectPairs(struct ProjectNode *head, wchar_t *projectName)
{
	int count = 0;
	struct ProjectNode *current = head;

	while (current != NULL)
	{
		if (wcscmp(current->project.name, projectName) == 0)
			++count;
		current = current->next;
	}
	return count;
}

void sortProjectNodes(struct ProjectNode **head_ref)
{
	if (*head_ref == NULL)
	{
		logger(L"Can't sort empty list");
		return;
	}

	struct ProjectNode *head = *head_ref;

	if (head->next == NULL)
	{
		logger(L"Can't sort only 1 entry");
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
			logger(L"Can't sort more, only 2 entries");
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

void sortPairNodes(struct PairNode **head_ref)
{
	if (*head_ref == NULL)
	{
		logger(L"Can't sort empty list");
		return;
	}

	struct PairNode *current = *head_ref;

	if (current->next == NULL)
	{
		logger(L"Can't sort only 1 entry");
		return;
	}

	bool changed;

	do
	{
		current = *head_ref;
		changed = false;

		// swap folder pairs
		while (current != NULL && current->next != NULL)
		{
			if (wcscmp(current->pair.source, current->next->pair.source) > 0 ||
				(wcscmp(current->pair.source, current->next->pair.source) == 0 &&
					wcscmp(current->pair.destination, current->next->pair.destination) > 0))
			{
				struct PairNode temp = { 0 };

				wcscpy_s(temp.pair.source, MAX_LINE, current->pair.source);
				wcscpy_s(temp.pair.destination, MAX_LINE, current->pair.destination);
				temp.pair.filesize = current->pair.filesize;

				wcscpy_s(current->pair.source, MAX_LINE, current->next->pair.source);
				wcscpy_s(current->pair.destination, MAX_LINE, current->next->pair.destination);
				current->pair.filesize = current->next->pair.filesize;

				wcscpy_s(current->next->pair.source, MAX_LINE, temp.pair.source);
				wcscpy_s(current->next->pair.destination, MAX_LINE, temp.pair.destination);
				current->next->pair.filesize = temp.pair.filesize;

				changed = true;
			}
			current = current->next;
		}
	} while (changed);
}
