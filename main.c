#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#include <libxml/parser.h>
#include <regex.h>

#pragma comment(lib, "iconv")
#pragma comment(lib, "libxml2")
#pragma comment(lib, "regex")

#define DEBUG 0
#define log(fmt, ...) do { if (DEBUG) printf(fmt, ##__VA_ARGS__); } while (0)
#define info(fmt, ...) do { printf(fmt, ##__VA_ARGS__); } while (0)

#define REGIONS "Asia|Australia|Austria|Belgium|Brazil|Canada|China|Croatia|Denmark|Europe|Finland"\
	"|France|Germany|Greece|India|Ireland|Israel|Italy|Japan|Korea|Latin America|Netherlands|Norway"\
	"|Poland|Portugal|Russia|Scandinavia|Seven|South Africa|Spain|Sweden|Switzerland|Taiwan|Turkey"\
	"|UK|USA|World"
#define MATCH_TITLE "(.*) \\(((, )?(" REGIONS "))+\\)"
#define MATCH_REGION "\\((((, )?(" REGIONS "))+)\\)"
#define MATCH_LANG "\\((\\w+(,\\w+)+)\\)"

int processProgress(int position, int count) {
	info("\r progress: %2d%%", position*100/count);
}

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
		info("arg err: input.dat [output.dat]\n");
		exit(1);
	}
	char* output_filename;
	if (argc >= 3) {
		output_filename = argv[2];
	} else {
		output_filename = calloc(1, strlen(input_filename) + 10);
		sprintf(output_filename, "%s_mod.dat", input_filename);
	}

	time_t time_start, time_end;
	time(&time_start);

	xmlDocPtr document = xmlParseFile(input_filename);
	if (!document) {
		info("cannot open %s\n", input_filename);
		exit(1);
	}

	int status;
	regex_t reg_title, reg_region, reg_lang;
	regmatch_t match[2];
	char str_buffer[255];
	char title2[255];
	int progress_position = 0, progress_count = 0;

	status = regcomp(&reg_title, MATCH_TITLE, REG_EXTENDED);
	status = regcomp(&reg_region, MATCH_REGION, REG_EXTENDED);
	status = regcomp(&reg_lang, MATCH_LANG, REG_EXTENDED);

	xmlNodePtr root = xmlDocGetRootElement(document);
	progress_count = xmlChildElementCount(root);

	// to improve the performance, we'll find the game name/region/language
	// and insert them temporarily in tree
	for (xmlNodePtr node = root->children; node != NULL; node = node->next) {
		if (strcmp(node->name, "game") != 0) {
			continue;
		}
		char* name = xmlGetProp(node, "name");

		status = regexec(&reg_title, name, 2, match, 0);
		if (status == 0) {
			int len = match[1].rm_eo - match[1].rm_so;
			strncpy(str_buffer, name + match[1].rm_so, len);
			str_buffer[len] = 0;
			xmlSetProp(node, "x_title", str_buffer);
		} else {
			info("error: cannot find title for <%s>\n", name);
			exit(1);
		}

		status = regexec(&reg_region, name, 2, match, 0);
		if (status == 0) {
			int len = match[1].rm_eo - match[1].rm_so;
			strncpy(str_buffer, name + match[1].rm_so, len);
			str_buffer[len] = 0;
			xmlSetProp(node, "x_region", str_buffer);
		} else {
			info("error: cannot find language for <%s>\n", name);
			exit(1);
		}

		status = regexec(&reg_lang, name, 2, match, 0);
		if (status == 0) {
			int len = match[1].rm_eo - match[1].rm_so;
			strncpy(str_buffer, name + match[1].rm_so, len);
			str_buffer[len] = 0;
			xmlSetProp(node, "x_lang", str_buffer);
		}
	}

	// here we'll use the previous properties to insert releases and clone relations
	// then we'll delete the temporary properties
	for (xmlNodePtr node = root->children; node != NULL; node = node->next) {
		if (strcmp(node->name, "game") != 0) {
			continue;
		}

		processProgress(progress_position++, progress_count);

		char* name = xmlGetProp(node, "name");
		char* title = xmlGetProp(node, "x_title");
		char* region = xmlGetProp(node, "x_region");
		char* languages = xmlGetProp(node, "x_lang");

		if (languages != NULL) {
			char* language = strtok(languages, ",");
			while (language != NULL) {
				insertRelease(node, name, region, language);
				language = strtok(NULL, ",");
			}
		} else {
			insertRelease(node, name, region, NULL);
		}

		char* cloneof = xmlGetProp(node, "cloneof");
		if (cloneof == NULL) {
			for (xmlNodePtr node2 = node->next; node2 != NULL; node2 = node2->next) {
				if (strcmp(node2->name, "game") != 0) {
					continue;
				}

				char* cloneof2 = xmlGetProp(node2, "cloneof");
				if (cloneof2 != NULL) {
					continue;
				}

				char* title2 = xmlGetProp(node2, "x_title");
				if (strcmp(title, title2) == 0) {
					xmlSetProp(node2, "cloneof", name);
				}
			}
		}

		xmlUnsetProp(node, "x_title");
		xmlUnsetProp(node, "x_region");
		xmlUnsetProp(node, "x_lang");
	}

	xmlSaveFile(output_filename, document);

	xmlFreeDoc(document);
	regfree(&reg_title);
	regfree(&reg_region);
	regfree(&reg_lang);

	time(&time_end);
	double time_diff = difftime(time_end, time_start);
	printf("\r done in %.0fs   \n", time_diff);
}
