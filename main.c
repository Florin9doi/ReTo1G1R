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
#define log(fmt, ...) do { if (DEBUG) printf(fmt, ##__VA_ARGS__); } while (0)
#define error(fmt, ...) do { printf(fmt, ##__VA_ARGS__); } while (0)

#define REGIONS "Asia|Australia|Austria|Belgium|Brazil|Canada|China|Croatia|Denmark|Europe|Finland"\
	"|France|Germany|Greece|India|Italy|Japan|Korea|Latin America|Netherlands|Norway|Poland|Portugal"\
	"|Russia|Scandinavia|Seven|South Africa|Spain|Sweden|Switzerland|UK|USA"

int insertRelease(xmlNodePtr node, char* name, char* region, char* language) {
	xmlNodeAddContent(node, "\t");
	xmlNodePtr release = xmlNewChild(node, NULL, "release", NULL);
	xmlSetProp(release, "name", name);
	xmlSetProp(release, "region", region);
	if (language != NULL) {
		xmlSetProp(release, "language", language);
	}
	xmlNodeAddContent(node, "\n\t");
}

int main(int argc, char** argv) {
	char* input_filename;
	if (argc >= 2) {
		input_filename = argv[1];
	} else {
		error("%s <dat file>\n", argv[0]);
		exit(1);
	}

	xmlDocPtr document = xmlParseFile(input_filename);
	if (!document) {
		error("cannot open %s\n", input_filename);
		exit(1);
	}
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

			//int status = regcomp(&re, "\\((\\w+(, \\w+)*)\\)", REG_EXTENDED);
			int status = regcomp(&re, "\\((((, )?(" REGIONS "))+)\\)", REG_EXTENDED);
			status = regexec(&re, name, 2, match, 0);
			if (status == 0) {
				int len = match[1].rm_eo - match[1].rm_so;
				strncpy(region, name + match[1].rm_so, len);
				region[len] = 0;
				log("\t Region: %s\n", region);
			} else {
				error("error: cannot find language for <%s>\n", name);
				exit(1);
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
					insertRelease(node, name, region, language);
					language = strtok(NULL, ",");
				}
				log("\n");
			} else {
				insertRelease(node, name, region, NULL);
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
