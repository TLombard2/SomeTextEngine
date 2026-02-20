#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include "ansiref.h"

#define MAX_VAR 256

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
int getLineNums (FILE *f, size_t zu, char *s, long *offsets) {
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
    char *color;

    getLineNums(file, size, s, lineOffsets);

    bool inIf = false;
    bool executing = true;
    long lastInputPos;
    bool inputMatched = false;
    bool varCheck = false;

    char *txt;
    char input[256];

    long preLinePos;
    while(1) {
        preLinePos = ftell(file);
        if (fgets(s, size, file) == NULL) break;
        s[strcspn(s, "\n")] = '\0'; // Removes \n to be added manually - prevents last line from not having a \n
        
        /// IF STATEMENT
        if ((txt = strstr(s, "endif")) != NULL) { // endif
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

        if ((txt = strstr(s, "elseif")) != NULL && inIf == true) { // elseif
            if ((txt = strstr(s, "input")) != NULL) {
                memmove(txt, txt + strlen("input "), strlen(txt));
                if (strcmp(input, txt) == 0) {
                    executing = true;
                    inputMatched = true;
                } else {
                    executing = false;
                }
            } else if ((txt = strstr(s, "var")) != NULL) {
                memmove(txt, txt + strlen("var "), strlen(txt));
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
                            varCheck = true;
                        } else {
                            executing = false;
                        }
                    }
                }
            }
            continue;
        }

        if ((txt = strstr(s, "if")) != NULL && inIf == false) { // if
            inIf = true;
            if ((txt = strstr(s, "input")) != NULL) { // input
                memmove(txt, txt + strlen("input "), strlen(txt));
                 if (strcmp(input, txt) == 0) {
                    executing = true;
                    inputMatched = true;
                } else {
                    executing = false;
                }
            } else if (((txt = strstr(s, "var")) != NULL)) { // var
                memmove(txt, txt + strlen("var "), strlen(txt)); // remove var
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
                            varCheck = true;
                        } else {
                            executing = false;
                        }
                    }
                }
            }
            continue;
        } else if ((txt = strstr(s, "if")) != NULL && inIf == true) {
            continue;
        }

        if (!executing) {
            continue;
        }

        // Change text color
        if ((txt = strstr(s, "color")) != NULL) {
            memmove(txt, txt + strlen("color "), strlen(txt));
            if ((strcmp(txt, "red")) == 0) {
                color = _RED;
            } else if ((strcmp(txt, "blue")) == 0) {
                color = _BLUE;
            } else if ((strcmp(txt, "green")) == 0) {
                color = _GREEN;
            }
        }
        // Text handling
        if ((txt = strstr(s, "text")) != NULL) {
            // Add if a word is all upper case then set color to pink.
            printf("%s%s" _RESET "\n", color, txt + strlen("text "));
        }

        // Input handling
        if ((txt = strstr(s, "input")) != NULL) {
            scanf("%255s", input);
            lastInputPos = preLinePos;
        }

        // Variable handling
        if ((txt = strstr(s, "var"))!= NULL) {
            memmove(txt, txt + strlen("var "), strlen(txt)); // remove var
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
        if ((txt = strstr(s, "goto")) != NULL) {
            memmove(txt, txt + strlen("goto "), strlen(txt));
            char *endptr;
            int num = strtol(txt, &endptr, 10);
            inIf = false;
            inputMatched = false;
            fseek(file, lineOffsets[num - 1], SEEK_SET);
            continue;
        }

        // scene handling
        if ((txt = strstr(s, "scene")) != NULL) {
            memmove(txt, txt + strlen("scene "), strlen(txt));
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