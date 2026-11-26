#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

static inline bool is_ident(int c) {
    return isalnum(c) || c == '_' || c == '$';
}

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        fprintf(stderr, "Usage: %s <input> <output> <var_name>\n", argv[0]);
        return 1;
    }

    FILE *in = fopen(argv[1], "rb");
    FILE *out = fopen(argv[2], "w");
    if (!in || !out) 
    {
        fprintf(stderr, "Error opening files!\n");
        if (in) fclose(in);
        if (out) fclose(out);
        return 1;
    }

    fseek(in, 0, SEEK_END);
    long size = ftell(in);
    fseek(in, 0, SEEK_SET);

    char *src = malloc(size + 1);
    if (!src) {
        fclose(in);
        fclose(out);
        return 1;
    }
    fread(src, 1, size, in);
    src[size] = '\0';
    fclose(in);

    fprintf(out, "const char %s[] = {\n", argv[3]);

    long i = 0;
    char in_string = 0;
    bool in_line_comment = false;
    bool in_block_comment = false;
    int last_out = -1;
    int col = 0;

    while (i < size) {
        char c = src[i];
        char next = (i + 1 < size) ? src[i + 1] : '\0';

        // 1. Single-line comments
        if (in_line_comment) {
            if (c == '\n' || c == '\r') in_line_comment = false;
            i++;
            continue;
        }

        // 2. Block comments
        if (in_block_comment) {
            if (c == '*' && next == '/') {
                in_block_comment = false;
                i += 2;
                continue;
            }
            i++;
            continue;
        }

        // 3. String literals (preserve content exactly)
        if (in_string) {
            fprintf(out, "0x%02x, ", (unsigned char)c);
            last_out = c;
            if (++col >= 16) { fprintf(out, "\n"); col = 0; }

            if (c == '\\' && next) {
                fprintf(out, "0x%02x, ", (unsigned char)next);
                last_out = next;
                if (++col >= 16) { fprintf(out, "\n"); col = 0; }
                i += 2;
                continue;
            } else if (c == in_string) {
                in_string = 0;
            }
            i++;
            continue;
        }

        // 4. Comment detection
        if (c == '/' && next == '/') {
            in_line_comment = true;
            i += 2;
            continue;
        }
        if (c == '/' && next == '*') {
            in_block_comment = true;
            i += 2;
            continue;
        }

        // 5. String delimiter detection
        if (c == '"' || c == '\'' || c == '`') {
            in_string = c;
            fprintf(out, "0x%02x, ", (unsigned char)c);
            last_out = c;
            if (++col >= 16) { fprintf(out, "\n"); col = 0; }
            i++;
            continue;
        }

        // 6. Whitespace collapsing (keep space only between identifiers)
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            long j = i + 1;
            while (j < size && (src[j] == ' ' || src[j] == '\t' || src[j] == '\r' || src[j] == '\n')) j++;
            char next_char = (j < size) ? src[j] : '\0';

            if ((is_ident(last_out) && is_ident(next_char)) ||
                (last_out == '+' && next_char == '+') ||
                (last_out == '-' && next_char == '-')) {
                fprintf(out, "0x%02x, ", (unsigned char)' ');
                last_out = ' ';
                if (++col >= 16) { fprintf(out, "\n"); col = 0; }
            }
            i = j;
            continue;
        }

        // 7. Regular code characters
        fprintf(out, "0x%02x, ", (unsigned char)c);
        last_out = c;
        if (++col >= 16) { fprintf(out, "\n"); col = 0; }
        i++;
    }

    fprintf(out, "0x00\n};\n");
    free(src);
    fclose(out);
    return 0;
}