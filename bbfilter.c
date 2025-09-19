// SNES Basic DASM Error Filter v1.0 (c)2025
// Filters out unhelpful "Unresolved Symbols" lists
// and reports the actual BASIC line causing errors.

#define BUFSIZE 4096

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void dasm_error_glue(char *linebuffer);
void report_basic_line_from_asm(char *asm_filename, long int asm_line_number, char *asm_code);
void removeCR(char *string);

int main(int argc, char **argv)
{
    char linebuffer[BUFSIZE];
    int match = 0;

    while(fgets(linebuffer, BUFSIZE, stdin) != NULL)
    {
        if(strncmp(linebuffer, "--- Unresolved Symbol List", 26) == 0)
        {
            match = 1;
            continue;
        }
        if((strncmp(linebuffer,"--- ",4) == 0) && strstr(linebuffer,"Unresolved Symbols") != NULL)
        {
            match = 0;
            continue;
        }
        if(match == 1) continue;

        fputs(linebuffer, stdout);
        dasm_error_glue(linebuffer);
    }
    return 0;
}

// Parse DASM error lines and report the offending BASIC line
void dasm_error_glue(char *linebuffer)
{
    char *mnemonic_error = NULL;
    char *asm_filename = NULL;
    char *asm_line_number_str = NULL;
    char *asm_code = NULL;
    char *temp_point = NULL;
    long int asm_line_number = -1;

    if(!linebuffer) return;

    mnemonic_error = strstr(linebuffer,"error: Unknown Mnemonic");
    if(!mnemonic_error) return;

    removeCR(linebuffer);

    // Extract the offending assembly code string
    asm_code = strchr(linebuffer,'\'');
    if(!asm_code) return;
    asm_code++; // skip first quote
    temp_point = strchr(asm_code,'\'');
    if(!temp_point) return;
    *temp_point = 0;

    // Extract assembly line number
    asm_line_number_str = strchr(linebuffer,'(');
    if(!asm_line_number_str) return;
    asm_line_number_str++;
    temp_point = strchr(asm_line_number_str,')');
    if(!temp_point) return;
    *temp_point = 0;

    asm_line_number = atol(asm_line_number_str);
    if((asm_line_number == 0) && strcmp(asm_line_number_str,"0")!=0) return;
    if(asm_line_number < 0) return;

    // Extract assembly filename
    temp_point = strchr(linebuffer,'(');
    if(!temp_point) return;
    if(temp_point == linebuffer) return;
    temp_point--;
    *temp_point = 0;
    asm_filename = linebuffer;

    // Report the BASIC line that likely caused this error
    report_basic_line_from_asm(asm_filename, asm_line_number, asm_code);
}

// Scan ASM file and find the BASIC line that generated the error
void report_basic_line_from_asm(char *asm_filename, long int asm_line_number, char *asm_code)
{
    long int t;
    char linebuffer[BUFSIZE];
    char line_save_str[BUFSIZE];
    FILE *asm_filehandle = NULL;
    char *temp_point = NULL;
    char *bas_line_str = NULL;
    char *bas_str = NULL;
    long int last_tagged_line = -1;
    long int line_save_long = -1;

    if(!asm_filename || asm_line_number < 0) return;

    asm_filehandle = fopen(asm_filename,"rb");
    if(!asm_filehandle) return;

    t = 0;
    while(fgets(linebuffer, BUFSIZE, asm_filehandle) != NULL)
    {
        removeCR(linebuffer);
        t++;

        bas_line_str = strstr(linebuffer,";;line ");
        if(bas_line_str)
        {
            bas_line_str += 7;
            temp_point = strstr(bas_line_str,";;");
            if(!temp_point) continue;

            *temp_point = 0;
            if(atol(bas_line_str)<1) continue;

            line_save_long = atol(bas_line_str);
            bas_str = temp_point + 2;
            strncpy(line_save_str,bas_str,BUFSIZE);
            last_tagged_line = t;
        }

        if(t == asm_line_number && last_tagged_line > 0 && ((t - last_tagged_line) < 8))
        {
            printf("    Likely source: BASIC line %ld\n", line_save_long);
            printf("    %s\n", line_save_str);
            fclose(asm_filehandle);
            return;
        }
    }

    fclose(asm_filehandle);
}

void removeCR(char *string)
{
    char *CR;
    if(!string) return;
    CR = strrchr(string,0x0A);
    if(CR) *CR = 0;
    CR = strrchr(string,0x0D);
    if(CR) *CR = 0;
}
