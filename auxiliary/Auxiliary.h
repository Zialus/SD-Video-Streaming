#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <unistd.h>

void split(const std::string& s, char delim, std::vector<std::string>& elems) {
	std::stringstream ss;
	ss.str(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
}


std::vector<std::string> split(const std::string& s, char delim) {
	std::vector<std::string> elems;
	split(s, delim, elems);
	return elems;
}


int AddString(char*** strings, size_t* count, const char* newStr) {
	char* copy;
	char** p;

	if (newStr == nullptr) {
		if ((p = (char**) realloc(*strings, (*count + 1) * sizeof(char*))) == nullptr) {
			return 0;
		}
		*strings = p;
		(*strings)[(*count)++] = nullptr;
	} else {

		if (strings == nullptr || (copy = (char*) malloc(strlen(newStr) + 1)) == nullptr) {
			return 0;
		}

		strcpy(copy, newStr);

		if ((p = (char**) realloc(*strings, (*count + 1) * sizeof(char*))) == nullptr) {
			free(copy);
			return 0;
		}

		*strings = p;
		(*strings)[(*count)++] = copy;
	}
	return 1;
}

void PrintStrings(char** strings, size_t count) {
	printf("BEGIN\n");
	if (strings != nullptr) {
		while ((count--) != 0) {
			printf("  %s\n", *strings++);
		}
	}
	printf("END\n");
}
