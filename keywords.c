#include "keywords.h"
#include "statements.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

extern int ors;
extern int numelses;
extern int numthens;

// helper to detect boolean swap candidates
int swaptest(char *value) {
    return value && (!strcmp(value, "then") || !strcmp(value, "&&") || !strcmp(value, "||"));
}

// helper: duplicate statement array
static void copy_statement(char **dest, char **src) {
    for (int i = 0; i < 200; i++) {
        if (src[i]) strcpy(dest[i], src[i]);
        else dest[i][0] = '\0';
    }
}

// helper: clear statement array
static void clear_statement(char **stmt) {
    for (int i = 0; i < 200; i++) stmt[i][0] = '\0';
}

// main keywords processor
void keywords(char **cstatement) {
    char **statement = malloc(200 * sizeof(char *));
    char **orstatement = malloc(200 * sizeof(char *));
    char **elstatement = malloc(200 * sizeof(char *));
    for (int i = 0; i < 200; i++) {
        statement[i] = malloc(200);
        orstatement[i] = malloc(200);
        elstatement[i] = malloc(200);
        statement[i][0] = orstatement[i][0] = elstatement[i][0] = '\0';
    }

    int door = 0, foundelse = 0;

    // process if-then-else & boolean operators
    for (int k = 0; k < 190; k++) {
        if (!cstatement[k+1][0]) continue;

        if (!strcmp(cstatement[k+1], "rem")) break; // skip rem

        // detect nested if / else
        if (!strcmp(cstatement[k+1], "if")) {
            for (int i = k+2; i < 200; i++) {
                if (!strcmp(cstatement[i], "if")) break;
                if (!strcmp(cstatement[i], "else")) foundelse = i;
            }

            // swap operands if needed (<, >, <=, >=)
            if ((strcmp(cstatement[k+3], ">") == 0 || strcmp(cstatement[k+3], "<=") == 0) &&
                swaptest(cstatement[k+5])) {
                char tmp[32];
                strcpy(tmp, cstatement[k+2]);
                strcpy(cstatement[k+2], cstatement[k+4]);
                strcpy(cstatement[k+4], tmp);
                if (strcmp(cstatement[k+3], ">") == 0) strcpy(cstatement[k+3], "<");
                else strcpy(cstatement[k+3], ">=");
            }

            // handle && -> then if
            if (!strcmp(cstatement[k+3], "&&")) {
                shiftdata(cstatement, k+3);
                sprintf(cstatement[k+3], "then%d", ors++);
                strcpy(cstatement[k+4], "if");
            } else if (!strcmp(cstatement[k+5], "&&")) {
                shiftdata(cstatement, k+5);
                sprintf(cstatement[k+5], "then%d", ors++);
                strcpy(cstatement[k+6], "if");
            }

            // handle || -> split statement
            if (!strcmp(cstatement[k+3], "||") || !strcmp(cstatement[k+5], "||")) {
                int idx = (!strcmp(cstatement[k+3], "||") ? 3 : 5);
                int start = 2 + idx;
                clear_statement(orstatement);
                for (int i = 2; i < 200 - start; i++) strcpy(orstatement[i], cstatement[start+i-2]);
                strcpy(cstatement[idx], "then");
                sprintf(orstatement[0], "%dOR", ors++);
                strcpy(orstatement[1], "if");
                door = 1;
                break; // handle recursively
            }
        }
    }

    // handle else
    if (foundelse) {
        char **pass2elstatement = door ? orstatement : cstatement;
        for (int i = 1; i < 200; i++)
            if (!strcmp(pass2elstatement[i], "else")) { foundelse = i; break; }

        for (int i = foundelse; i < 200; i++)
            strcpy(elstatement[i - foundelse], pass2elstatement[i]);

        if (islabelelse(pass2elstatement)) {
            strcpy(pass2elstatement[foundelse++], ":");
            strcpy(pass2elstatement[foundelse++], "goto");
            sprintf(pass2elstatement[foundelse++], "skipelse%d", numelses);
        }
        for (int i = foundelse; i < 200; i++) pass2elstatement[i][0] = '\0';

        if (!islabelelse(elstatement)) {
            strcpy(elstatement[1], "goto");
            strcpy(elstatement[2], elstatement[1]);
        }
        if (door) {
            for (int i = 1; i < 200; i++)
                if (!strcmp(cstatement[i], "else")) {
                    for (int k = i; k < 200; k++) cstatement[k][0] = '\0';
                    break;
                }
        }
    }

    // recurse if door / else
    if (door) { keywords(orstatement); }
    if (foundelse) { keywords(elstatement); }

    // copy final statement back
    copy_statement(statement, cstatement);

    // label numbering
    if (!strcmp(statement[0], "then")) sprintf(statement[0], "%dthen", numthens++);

    // dispatch statements
    invalidate_Areg();
    for (int loop = 0; loop < 200; loop++) {
        if (!statement[1][0]) break;

        if (!strcmp(statement[1], "def")) break;
        else if (!strcmp(statement[0], "end")) endfunction();
        else if (!strcmp(statement[1], "includesfile")) create_includes(statement[2]);
        else if (!strcmp(statement[1], "include")) add_includes(statement[2]);
        else if (!strcmp(statement[1], "inline")) add_inline(statement[2]);
        else if (!strcmp(statement[1], "function")) function(statement);
        else if (!strcmp(statement[1], "if")) { doif(statement); break; }
        else if (!strcmp(statement[1], "goto")) dogoto(statement);
        else if (!strcmp(statement[1], "bank")) newbank(atoi(statement[2]));
        else if (!strcmp(statement[1], "sdata")) sdata(statement);
        else if (!strcmp(statement[1], "data")) data(statement);
        else if (!strcmp(statement[1], "on") && !strcmp(statement[3], "go")) ongoto(statement);
        else if (!strcmp(statement[1], "const")) doconst(statement);
        else if (!strcmp(statement[1], "dim")) dim(statement);
        else if (!strcmp(statement[1], "for")) dofor(statement);
        else if (!strcmp(statement[1], "next")) next(statement);
        else if (!strcmp(statement[1], "gosub")) gosub(statement);
        else if (!strcmp(statement[1], "pfpixel")) pfpixel(statement);
        else if (!strcmp(statement[1], "pfhline")) pfhline(statement);
        else if (!strcmp(statement[1], "pfclear")) pfclear(statement);
        else if (!strcmp(statement[1], "pfvline")) pfvline(statement);
        else if (!strcmp(statement[1], "pfscroll")) pfscroll(statement);
        else if (!strcmp(statement[1], "drawscreen")) drawscreen();
        else if (!strcmp(statement[1], "rerand")) rerand();
        else if (!strcmp(statement[1], "asm")) doasm();
        else if (!strcmp(statement[1], "pop")) dopop();
        else if (!strcmp(statement[1], "rem")) { rem(statement); break; }
        else if (!strcmp(statement[1], "set")) set(statement);
        else if (!strcmp(statement[1], "return")) doreturn(statement);
        else if (!strcmp(statement[1], "reboot")) doreboot();
        else if (!strcmp(statement[1], "vblank")) vblank();
        else if (!strncmp(statement[1], "player", 6)) player(statement);
        else if (!strcmp(statement[2], "=")) dolet(statement);
        else if (!strcmp(statement[1], "let")) dolet(statement);
        else if (!strcmp(statement[1], "dec")) dec(statement);
        else if (!strcmp(statement[1], "macro")) domacro(statement);
        else if (!strcmp(statement[1], "push")) do_push(statement);
        else if (!strcmp(statement[1], "pull")) do_pull(statement);
        else if (!strcmp(statement[1], "stack")) do_stack(statement);
        else if (!strcmp(statement[1], "callmacro")) callmacro(statement);
        else if (!strcmp(statement[1], "extra")) doextra(statement[1]);
        else {
            char errorcode[100];
            sprintf(errorcode, "Error: Unknown keyword: %s\n", statement[1]);
            prerror(errorcode);
            exit(1);
        }
    }

    if (foundelse) printf(".skipelse%d\n", numelses++);

    free(statement);
    free(orstatement);
    free(elstatement);
}
