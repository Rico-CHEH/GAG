#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>

#include "commands.h"

int main(int argc, char *argv[]) {
    // Flush the buffers
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    if (argc < 2) {
        perror("No command provided.\n");
        return 1;
    }

    if (strcmp(argv[1], "init") == 0) {
        if (argc != 2) {
            perror("Wrong format for gag init\n");
            return 1;
        }

        if (gag_init() == 1) return 1;
    } else if (strcmp(argv[1], "cat-file") == 0) {
        if (argc != 4 || strcmp(argv[2], "-p") != 0) {
            perror("Wrong format. Expected: gag cat-file -p <hash>\n");
            return 1;

        } else if (strlen(argv[3]) != 40) {
            perror("Not a valid hash\n");
            return 1;
        }

        if (gag_catfile(argv[3]) == 1) {
            return 1;
        }
    } else {
        // Note, does not indicate what is the command
        perror("Unknown command\n");
        return 2;
    }

    return 0;
}
