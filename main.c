#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include "ansiref.h"
#include <ctype.h>

#define MAX_VAR 256

// Case insensitive substring search to replace strstr
char *ci_strstr(const char *haystack, const char *needle) {
    if (!*needle) return (char *)haystack;
    for (; *haystack; haystack++) {
        const char *h = haystack, *n = needle;
        while (*h && *n && tolower((unsigned char)*h) == tolower((unsigned char)*n)) {
            h++; n++;
        }
        if (!*n) return (char *)haystack;
    }
    return NULL;
}

typedef struct var {
    int val;
    char *s;
} var;

size_t getFileSize(FILE *file, const char* filename) {
    long zu;
    if (fseek(file, 0, SEEK_END) == 0) { // Moves pointer to end of file
        zu = ftell(file); 
    } else {
        fprintf(stderr, "getFileSize failure for %s: %s\n", filename, strerror(errno));
        return 0;
    }
    rewind(file);
    if (zu != -1) {
        return zu;
    } else {
        fprintf(stderr, "getFileSize failure for %s: %s\n", filename, strerror(errno));
        return 0;
    }
}

FILE* openFile(const char *filename) {
    FILE *f;
    f = fopen(filename, "rb");
    if (f != NULL) {
        return f;
    } else {
        fprintf(stderr, "openFile failure for %s: %s\n", filename, strerror(errno));
        return NULL;
    }
}

// Gets an array of line nums for use with goto
void getLineNums (FILE *f, size_t zu, char *s, long *offsets) {
    long pos = ftell(f);
    int lineCount = 0;
    while (fgets(s, zu, f) != NULL) {
        offsets[lineCount++] = pos;
        pos = ftell(f);
    }
    rewind(f);
}

int main () {
    FILE *file;
    size_t size;
    const char* filename = "src/main.txt"; // Every game starts with a main.txt
    file = openFile(filename);
    size = getFileSize(file, filename);
    char *s = malloc(size + 1);
    long lineOffsets[1024];
    var pairs[MAX_VAR] = {0};
    char *color = _GREEN;

    getLineNums(file, size, s, lineOffsets);

    bool inIf = false;
    bool executing = true;
    long lastInputPos;
    bool inputMatched = false;

    char *txt;
    char input[256];

    long preLinePos;
    while(1) {
        preLinePos = ftell(file);
        if (fgets(s, size, file) == NULL) break;
        s[strcspn(s, "\r\n")] = '\0'; // Removing \r handling CRLF
        
        /// IF STATEMENT
        if ((txt = ci_strstr(s, "endif")) != NULL) { // endif
            inIf = false;
            if (executing == false && inputMatched == false) {
                printf(_RED"Try Again.\n"_RESET);
                executing = true;
                fseek(file, lastInputPos, SEEK_SET);
                continue;
            } else {
                executing = true;
                continue;
            }
        }

        if ((txt = ci_strstr(s, "elseif")) != NULL && inIf == true) { // elseif
            if (inputMatched) {
                executing = false;
                continue;
            }
            if ((txt = ci_strstr(s, "input")) != NULL) {
                size_t skip = strlen("input ");
                memmove(txt, txt + skip, strlen(txt + skip) + 1);
                if (ci_strstr(input, txt) != NULL) {
                    executing = true;
                    inputMatched = true;
                } else {
                    executing = false;
                }
            } else if ((txt = ci_strstr(s, "var")) != NULL) {
                size_t skip = strlen("var ");
                memmove(txt, txt + skip, strlen(txt + skip) + 1);
                char *str1;
                char *str2;
                str1 = strtok(txt, " ");
                str2 = strtok(NULL, " ");
                char *endptr;
                int val = strtol(str2, &endptr, 10);
                for (int i = 0; i < MAX_VAR; i++) {
                    if (pairs[i].s != NULL && strcmp(pairs[i].s, str1) == 0) {
                        if (val == pairs[i].val) {
                            executing = true;
                        } else {
                            executing = false;
                        }
                    }
                }
            }
            continue;
        }

        if ((txt = ci_strstr(s, "if")) != NULL && inIf == false) { // if
            inIf = true;
            if ((txt = ci_strstr(s, "input")) != NULL) { // input
                size_t skip = strlen("input ");
                memmove(txt, txt + skip, strlen(txt + skip) + 1);

                // Find earliest matching keyword in the input string
                char *match = ci_strstr(input, txt);
                int bestOffset = match ? (int)(match - input) : -1;
                long bestBranchPos = match ? ftell(file) : -1;

                // Forward-scan elseif input branches for a better match
                char scanBuf[512];
                while (1) {
                    if (fgets(scanBuf, sizeof(scanBuf), file) == NULL) break;
                    scanBuf[strcspn(scanBuf, "\r\n")] = '\0';
                    if (strstr(scanBuf, "endif") != NULL) break;
                    char *stxt;
                    if ((stxt = strstr(scanBuf, "elseif")) != NULL &&
                        (stxt = strstr(scanBuf, "input")) != NULL) {
                        size_t eskip = strlen("input ");
                        memmove(stxt, stxt + eskip, strlen(stxt + eskip) + 1);
                        char *ematch = ci_strstr(input, stxt);
                        if (ematch != NULL) {
                            int offset = (int)(ematch - input);
                            if (bestOffset == -1 || offset < bestOffset) {
                                bestOffset = offset;
                                bestBranchPos = ftell(file);
                            }
                        }
                    }
                }

                if (bestOffset != -1) {
                    fseek(file, bestBranchPos, SEEK_SET);
                    executing = true;
                    inputMatched = true;
                } else {
                    inIf = false;
                    executing = true;
                    printf(_RED "Try Again.\n" _RESET);
                    fseek(file, lastInputPos, SEEK_SET);
                }
            } else if (((txt = ci_strstr(s, "var")) != NULL)) { // var
                size_t skip = strlen("var ");
                memmove(txt, txt + skip, strlen(txt + skip) + 1); // remove var
                char *str1;
                char *str2;
                str1 = strtok(txt, " ");
                str2 = strtok(NULL, " ");
                char *endptr;
                int val = strtol(str2, &endptr, 10);
                for (int i = 0; i < MAX_VAR; i++) {
                    if (pairs[i].s != NULL && strcmp(pairs[i].s, str1) == 0) {
                        if (val == pairs[i].val) {
                            executing = true;
                        } else {
                            executing = false;
                        }
                    }
                }
            }
            continue;
        } else if ((txt = ci_strstr(s, "if")) != NULL && inIf == true) {
            continue;
        }

        if (!executing) {
            continue;
        }

        // Change text color
        if ((txt = ci_strstr(s, "color")) != NULL) {
            size_t skip = strlen("color ");
            memmove(txt, txt + skip, strlen(txt + skip) + 1);
            if ((strcmp(txt, "red")) == 0) {
                color = _RED;
            } else if ((strcmp(txt, "blue")) == 0) {
                color = _BLUE;
            } else if ((strcmp(txt, "green")) == 0) {
                color = _GREEN;
            }
        }
        // Text handling
        if ((txt = ci_strstr(s, "text")) != NULL) {
            // Add if a word is all upper case then set color to pink.
            printf("%s%s" _RESET "\n", color, txt + strlen("text "));
        }

        // Input handling
        if ((txt = ci_strstr(s, "input")) != NULL) {
            fgets(input, sizeof(input), stdin);
            input[strcspn(input, "\r\n")] = '\0';
            lastInputPos = preLinePos;
        }

        // Variable handling
        if ((txt = ci_strstr(s, "var"))!= NULL) {
            size_t skip = strlen("var ");
            memmove(txt, txt + skip, strlen(txt + skip) + 1); // remove var
            char *str1;
            char *str2;
            str1 = strtok(txt, " ");
            str2 = strtok(NULL, " ");
            char *endptr;
            int val = strtol(str2, &endptr, 10);
            // Check if var exists
            for (int i = 0; i < MAX_VAR; i++) {
                    if (pairs[i].s != NULL && strcmp(pairs[i].s, str1) == 0) {
                        pairs[i].val = val; // Set its new val
                        goto end;
                    }
                }
            // Make new var
            for (int i = 0; i < MAX_VAR; i++) {
                if (pairs[i].s == NULL) {
                    pairs[i].s = malloc(strlen(str1) + 1);
                    strcpy(pairs[i].s, str1);
                    pairs[i].val = val;
                    break;
                }
            }
            end:
            continue;
        }
        // goto handling
        if ((txt = ci_strstr(s, "goto")) != NULL) {
            size_t skip = strlen("goto ");
            memmove(txt, txt + skip, strlen(txt + skip) + 1);
            char *endptr;
            int num = strtol(txt, &endptr, 10);
            inIf = false;
            inputMatched = false;
            fseek(file, lineOffsets[num - 1], SEEK_SET);
            continue;
        }

        // scene handling
        if ((txt = ci_strstr(s, "scene")) != NULL) {
            size_t skip = strlen("scene ");
            memmove(txt, txt + skip, strlen(txt + skip) + 1);
            fclose(file);
            file = openFile(txt);
            size = getFileSize(file, txt);
            s = malloc(size + 1);
            getLineNums(file, size, s, lineOffsets);
            inIf = false;
            inputMatched = false;
            continue;
        }
    }
    free(s);
    fclose(file);
}