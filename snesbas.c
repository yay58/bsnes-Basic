// SNES Basic Fork Compiler v1.0 (c)2025
// GPL v2 license applies

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "statements.h"
#include "keywords.h"
#include <math.h>

#define BB_VERSION_INFO "snes Basic v1.0 (c)2025\n"

extern int bank;
extern int bs;
extern int numconstants;
extern int playfield_index[];
extern int line;

// SNES OAM / VRAM state
int vram_offset[4] = {0};
int oam_index = 0;

int main(int argc, char *argv[])
{
    char **statement;
    int i,j,k;
    int unnamed = 0;
    int defcount = 0;
    char *c;
    char single;
    char code[512];
    char displaycode[512];
    FILE *header = NULL;
    int multiplespace = 0;
    char *includes_file = "snes_includes.inc";
    char *filename = "snesbasic_variable_redefs.h";
    char *path = 0;
    char def[500][100];
    char defr[500][100];
    char finalcode[512];
    char *codeadd;
    char mycode[512];
    int defi = 0;

    // Command line parsing
    while((i=getopt(argc,argv,"i:r:v"))!=-1)
    {
        switch(i)
        {
            case 'i': path = optarg; break;
            case 'r': filename = optarg; break;
            case 'v': printf("%s",BB_VERSION_INFO); exit(0);
            case '?': fprintf(stderr,"usage: %s -r <varfile> -i <includes>\n",argv[0]); exit(1);
        }
    }

    fprintf(stderr,BB_VERSION_INFO);

    printf("game_start:\n");
    header_open(header);
    init_includes(path);

    playfield_index[0]=0;

    // Allocate statement array
    statement = (char **)malloc(sizeof(char*)*200);
    for(i=0;i<200;i++) statement[i] = (char*)malloc(sizeof(char)*200);

    // Main compilation loop
    while(1)
    {
        for(i=0;i<200;i++) memset(statement[i],0,200);

        c = fgets(code,512,stdin);
        if(!c) break;
        incline();
        strcpy(displaycode,code);
        strcpy(mycode,code);

        // Parse defines
        int k_def_search;
        for(k_def_search=0;k_def_search<495;k_def_search++)
            if(code[k_def_search]==' ') break;

        if(k_def_search<495 && code[k_def_search]==' ' &&
           (k_def_search+4<499) &&
           code[k_def_search+1]=='d' && code[k_def_search+2]=='e' &&
           code[k_def_search+3]=='f' && code[k_def_search+4]==' ')
        {
            int current_pos = k_def_search + 5;
            if(defi>=499){fprintf(stderr,"(%d) ERROR: Max defines reached\n",bbgetline()); exit(1);}
            for(j=0;current_pos<499 && code[current_pos]!=' ' && code[current_pos]!='\0' && code[current_pos]!='\n' && code[current_pos]!='\r'; current_pos++)
                if(j<99) def[defi][j++] = code[current_pos]; else {fprintf(stderr,"(%d) Define name too long\n",bbgetline()); exit(1);}
            def[defi][j]=0;
            if(j==0){fprintf(stderr,"(%d) Empty define\n",bbgetline()); exit(1);}
            if(!(current_pos<497 && code[current_pos]==' ' && code[current_pos+1]=='=' && code[current_pos+2]==' '))
                {fprintf(stderr,"(%d) Malformed define\n",bbgetline()); exit(1);}
            current_pos+=3;
            for(j=0;current_pos<499 && code[current_pos]!='\0' && code[current_pos]!='\n' && code[current_pos]!='\r'; current_pos++)
                if(j<99) defr[defi][j++] = code[current_pos]; else {fprintf(stderr,"(%d) Define replacement too long\n",bbgetline()); exit(1);}
            defr[defi][j]=0;
            removeCR(defr[defi]);
            printf(";PARSED_DEFINE: .%s. = .%s.\n",def[defi],defr[defi]);
            defi++;
        }
        else if(defi)
        {
            for(int def_idx=0;def_idx<defi;def_idx++)
            {
                codeadd=NULL;
                finalcode[0]=0;
                defcount=0;
                while(1)
                {
                    if(defcount++>500){fprintf(stderr,"(%d) Infinite define expansion\n",bbgetline()); exit(1);}
                    codeadd = strstr(mycode,def[def_idx]);
                    if(!codeadd) break;
                    strncpy(finalcode,mycode,codeadd-mycode);
                    finalcode[codeadd-mycode]=0;
                    strcat(finalcode,defr[def_idx]);
                    strcat(finalcode,codeadd+strlen(def[def_idx]));
                    strcpy(mycode,finalcode);
                }
            }
        }
        if(strcmp(mycode,code)) strcpy(code,mycode);

        // Lexical preprocessing
        i=0;j=0;k=0; multiplespace=0;
        while(code[i]!='\0')
        {
            single = code[i++];
            if(single==' '){if(!multiplespace){j++;k=0;} multiplespace++;} 
            else {multiplespace=0; if(k<199) statement[j][k++]=single;}
        }
        if(j>190) fprintf(stderr,"(%d) Warning: long line\n",bbgetline());

        if(statement[0][0]=='\0') sprintf(statement[0],"L%d",unnamed++);
        else if(strchr(statement[0],'.')){fprintf(stderr,"(%d) Invalid label char\n",bbgetline()); exit(1);}

        if(strncmp(statement[0],"end",3)) printf("%s: ;;line %d;; %s\n",statement[0],line,displaycode);
        else doend();

        // **SNES-specific code generation**
        keywords(statement);

        if(numconstants==(MAXCONSTANTS-1)){fprintf(stderr,"(%d) Max constants exceeded\n",bbgetline()); exit(1);}
    }

    bank=bbank();
    bs=bbs();

    // SNES OAM/VRAM init
    printf("; SNES OAM / VRAM setup\n");
    printf("    lda #0\n    sta $2104 ; reset OAM addr\n");

    // ROM space reporting
    printf("if ECHOFIRST\n");
    printf("    echo \"ROM space left: ...\"\n");
    printf("endif\nECHOFIRST=1\n");

    header_write(header,filename);
    create_includes(includes_file);

    fprintf(stderr,"SNES Basic compilation complete.\n");
    return 0;
}
