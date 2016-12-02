#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int AddString(char*** strings, size_t* count, const char* newStr)
{
	char* copy;
	char** p;

	if (newStr == NULL) {
		if ((p = (char**) realloc(*strings, (*count + 1) * sizeof(char*))) == NULL)
		{
			return 0;
		}
		*strings = p;
		(*strings)[(*count)++] = NULL;
	} else {

		if (strings == NULL || (copy = (char*) malloc(strlen(newStr) + 1)) == NULL){
			return 0;
		}

		strcpy(copy, newStr);

		if ((p = (char**) realloc(*strings, (*count + 1) * sizeof(char*))) == NULL)
		{
			free(copy);
			return 0;
		}

		*strings = p;
		(*strings)[(*count)++] = copy;
	}
	return 1;
}

void PrintStrings(char** strings, size_t count)
{
	printf("BEGIN\n");
	if (strings != NULL)
	while (count--)
	printf("  %s\n", *strings++);
	printf("END\n");
}
