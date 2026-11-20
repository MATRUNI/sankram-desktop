#include <stdio.h>

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        fprintf(stderr, "Usage: %s <input> <output> <var_name>\n", argv[0]);
        return 1;
    }
    FILE *in = fopen(argv[1],"rb");
    FILE *out = fopen(argv[2],"w");
    if (!in || !out) 
    {
        fprintf(stderr, "Error opening files!\n");
        if (in) fclose(in);
        if (out) fclose(out);
        return 1;
    }
    fprintf(out,"const char %s[] = {\n",argv[3]);
    int ch;
    while ((ch = fgetc(in)) != EOF) 
    {
        fprintf(out, "0x%02x, ", ch);
    }

    fprintf(out,"0x00\n};\n");
    
    fclose(in);
    fclose(out);
    return 0;
}