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
	"|France|Germany|Greece|India|Italy|Japan|Korea|Latin America|Netherlands|Norway|Poland|Portugal"\
	"|Russia|Scandinavia|Seven|South Africa|Spain|Sweden|Switzerland|UK|USA"
#define MATCH_NAME "(.*) \\(((, )?(" REGIONS "))+\\)"
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
		error("%s <dat file>\n", argv[0]);
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

	regex_t reg_name;
	regex_t reg_region;
	regex_t reg_lang;
	int status = regcomp(&reg_name, MATCH_NAME, REG_EXTENDED);
	status = regcomp(&reg_region, MATCH_REGION, REG_EXTENDED);
	status = regcomp(&reg_lang, MATCH_LANG, REG_EXTENDED);
	char title[255];
	char region[100];
	char languages[100];
	char title2[255];
	int progress_position = 0, progress_count = 0;

	xmlNodePtr root = xmlDocGetRootElement(document);
	progress_count = xmlChildElementCount(root);
	for (xmlNodePtr node = root->children; node != NULL; node = node->next) {
		if (strcmp(node->name, "game") == 0) {
			processProgress(progress_position++, progress_count);

			regmatch_t match[2];

			char* name = xmlGetProp(node, "name");

			status = regexec(&reg_region, name, 2, match, 0);
			if (status == 0) {
				int len = match[1].rm_eo - match[1].rm_so;
				strncpy(region, name + match[1].rm_so, len);
				region[len] = 0;
			} else {
				info("error: cannot find language for <%s>\n", name);
				exit(1);
			}

			status = regexec(&reg_lang, name, 2, match, 0);
			if (status == 0) {
				int len = match[1].rm_eo - match[1].rm_so;
				strncpy(languages, name + match[1].rm_so, len);
				languages[len] = 0;

				char* language = strtok(languages, ",");
				while (language != NULL) {
					insertRelease(node, name, region, language);
					language = strtok(NULL, ",");
				}
			} else {
				insertRelease(node, name, region, NULL);
			}


			char* cloneof = xmlGetProp(node, "cloneof");
			if (cloneof != NULL) {
				continue;
			}
			status = regexec(&reg_name, name, 2, match, 0);
			if (status == 0) {
				int len = match[1].rm_eo - match[1].rm_so;
				strncpy(title, name + match[1].rm_so, len);
				title[len] = 0;
				//error("%s\n", title);
			}

			for (xmlNodePtr node2 = node->next; node2 != NULL; node2 = node2->next) {
				if (strcmp(node2->name, "game") == 0) {
					char* cloneof2 = xmlGetProp(node2, "cloneof");
					if (cloneof2 != NULL) {
						continue;
					}
					char* name2 = xmlGetProp(node2, "name");

					status = regexec(&reg_name, name2, 2, match, 0);
					if (status == 0) {
						int len = match[1].rm_eo - match[1].rm_so;
						strncpy(title2, name2 + match[1].rm_so, len);
						title2[len] = 0;
					}

					if (strcmp(title, title2) == 0) {
						xmlSetProp(node2, "cloneof", name);
					}
				}
			}
		}
	}

//	xmlDocDump(stdout, document);

	xmlSaveFile(output_filename, document);

	xmlFreeDoc(document);
	regfree(&reg_name);
	regfree(&reg_region);
	regfree(&reg_lang);

	time(&time_end);
	double time_diff = difftime(time_end, time_start);
	printf("\r done in %.0fs   \n", time_diff);
}
