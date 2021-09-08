#include "kitchen_sync.h"

static void deleteProjectNode(struct ProjectNode **, int);

void appendProjectNode(struct ProjectNode **head_ref, wchar_t *projectName, wchar_t *sourceFolder, wchar_t *destFolder)
{
	struct Project project = {0};
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
	} while (*head_ref != NULL && current != NULL);
}

// delete single folder pair by name
void deleteFolderPair(struct ProjectNode **head_ref, wchar_t *folderPair, wchar_t *projectName)
{
	sortProjectNodes(head_ref);
	size_t length = wcslen(folderPair);
	assert(length > 0);

	if (length > 0)
	{
		wchar_t src[MAX_LINE] = {0};
		wchar_t dst[MAX_LINE] = {0};
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

// replace existing folder pair with edited pair
void replaceFolderPair(struct ProjectNode **head_ref, wchar_t *oldFolderPair, wchar_t *projectName, wchar_t *newSrc, wchar_t *newDst)
{
	sortProjectNodes(head_ref);
	size_t length = wcslen(oldFolderPair);
	assert(length > 0);

	if (length > 0)
	{
		// extract old folder pair
		wchar_t src[MAX_LINE] = {0};
		wchar_t dst[MAX_LINE] = {0};
		splitPair(oldFolderPair, src, dst, length);

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
	} while (changed);
}
