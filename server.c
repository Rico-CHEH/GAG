#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>

int main(int argc, char *argv[]) {
    // Flush the buffers
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    if (argc < 2) {
        perror("No command provided.\n");
        return 1;
    }

    char *command = argv[1];

    if (argc == 2 && strcmp(command, "init") == 0) {
        // Create necessary files

        _mkdir(".gag");    // Yes I am on Windows SHUT UP
        _mkdir(".gag\\objects");
        _mkdir(".gag\\refs");

        FILE *head_file; 
        fopen_s(&head_file, ".gag/HEAD", "w+");  // Note, currently erases pre-existing file

        if (head_file == NULL) {
            perror("Failed to create .gag/HEAD file.\n");
            return 1;
        }

        fprintf(head_file, "ref: refs/heads/main\n");
        fclose(head_file);

        printf("Initialized GAG directory\n");
    }
    else {
        // Note, does not indicate what is the command
        perror("Unknown command\n");
        return 2;
    }

    return 0;
}
