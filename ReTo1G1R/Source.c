#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#include <libxml/parser.h>
#include <regex.h>

#pragma comment(lib, "iconv")
#pragma comment(lib, "libxml2")
#pragma comment(lib, "regex")

#define DEBUG 0
#define log(fmt, ...) do { if (DEBUG) printf(fmt, __VA_ARGS__); } while (0)

int main(int argc, char** argv) {
	char* input_filename;
	if (argc >= 2) {
		input_filename = argv[1];
	} else {
		input_filename = "..\\..\\wii.dat";
	}

	xmlDocPtr document = xmlParseFile(input_filename);
	xmlNodePtr root = xmlDocGetRootElement(document);

	for (xmlNodePtr node = root->children; node != NULL; node = node->next) {
		log("game->name : <%s> %d\n", node->name, node->type);

		if (strcmp(node->name, "game") == 0) {
			char* name = xmlGetProp(node, "name");
			char region[100];
			char languages[100];

			log("game->name : <%s>\n", name);

			regex_t re;
			regmatch_t match[2];

			int status = regcomp(&re, "\\((\\w+)\\)", REG_EXTENDED);
			status = regexec(&re, name, 2, match, 0);
			if (status == 0) {
				int len = match[1].rm_eo - match[1].rm_so;
				strncpy(region, name + match[1].rm_so, len);
				region[len] = 0;
				log("\t Region: %s\n", region);
			}
			regfree(&re);

			status = regcomp(&re, "\\((\\w+(,\\w+)+)\\)", REG_EXTENDED);
			status = regexec(&re, name, 2, match, 0);
			if (status == 0) {
				int len = match[1].rm_eo - match[1].rm_so;
				strncpy(languages, name + match[1].rm_so, len);
				languages[len] = 0;
				log("\t Languages: ");

				char* language = strtok(languages, ",");
				while (language != NULL) {
					log("=%s= ", language);

					xmlNodeAddContent(node, "\t");
					xmlNodePtr release = xmlNewChild(node, NULL, "release", NULL);
					xmlSetProp(release, "name", name);
					xmlSetProp(release, "region", region);
					xmlSetProp(release, "language", language);
					xmlNodeAddContent(node, "\n\t");

					language = strtok(NULL, ",");
				}
				log("\n");
			} else {
				xmlNodeAddContent(node, "\t");
				xmlNodePtr release = xmlNewChild(node, NULL, "release", NULL);
				xmlSetProp(release, "name", name);
				xmlSetProp(release, "region", region);
				xmlNodeAddContent(node, "\n\t");
			}
			regfree(&re);
		}
	}

//	xmlDocDump(stdout, document);

	char* output_filename = calloc(1, strlen(input_filename) + 10);
	sprintf(output_filename, "%s_mod.dat", input_filename);
	xmlSaveFile(output_filename, document);
	xmlFreeDoc(document);
	free(output_filename);
}
