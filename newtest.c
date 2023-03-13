#include <stdio.h>
#include <stdbool.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s [-v] [-r N] [file_name]\n", argv[0]);
        return 1;
    }

    char *file_name;
    int repeat_count = 1;
    bool verbose = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0) {
            verbose = true;
        } else if (strcmp(argv[i], "-r") == 0) {
            if (i + 1 < argc) {
                repeat_count = atoi(argv[i + 1]);
                i++;
            } else {
                printf("Error: -r option requires a numeric argument.\n");
                return 1;
            }
        } else {
            file_name = argv[i];
        }
    }

    FILE *fp;
    int data;

    for (int i = 0; i < repeat_count; i++) {
        fp = fopen(file_name, "r"); // open file in read mode
        if (fp == NULL) {
            printf("Error opening file.\n");
            return 1;
        }

        while (fscanf(fp, "%d", &data) != EOF) { // read until end of file
            if (verbose) {
                printf("Data value: %d\n", data);
            }
        }

        fclose(fp); // close file
    }

    return 0;
}
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s [file_name]\n", argv[0]);
        return 1;
    }

    char *file_name = argv[1];
    FILE *fp;
    char type;
    char operation;
    char block;
    int offset;
    union {
        int decimal;
        float floating;
        unsigned int hex;
    } value;
    int repeat;

    fp = fopen(file_name, "r"); // open file in read mode
    if (fp == NULL) {
        printf("Error opening file.\n");
        return 1;
    }

    while (fscanf(fp, " %c %c %c %d", &operation, &block, &type, &offset) != EOF) {
        if (type == 'd') { // decimal
            fscanf(fp, "%d", &value.decimal);
            fscanf(fp, "%d", &repeat);
            for(int i = 0; i < repeat; i++) {
                printf("%c %c %d %d\n", operation, block, offset, value.decimal);
            }
        } else if (type == 'x') { // hex
            fscanf(fp, "%x", &value.hex);
            fscanf(fp, "%d", &repeat);
            for(int i = 0; i < repeat; i++) {
                printf("%c %c %d 0x%x\n", operation, block, offset, value.hex);
            }
        } else if (type == 'f') { // float
            fscanf(fp, "%f", &value.floating);
            fscanf(fp, "%d", &repeat);
            for(int i = 0; i < repeat; i++) {
                printf("%c %c %d %f\n", operation, block, offset, value.floating);
            }
        } else {
            printf("Invalid value type: %c\n", type);
        }
    }

    fclose(fp); // close file

    return 0;
}
