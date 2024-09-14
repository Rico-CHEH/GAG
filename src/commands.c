#include <direct.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "commands.h"

GAG_State get_gag_dir(char working_dir[1000]) {
    _getcwd(working_dir, 1000);
    char *ptr;

    if ((ptr = strstr(working_dir, ".gag")) != NULL) {
        printf("The working_dir is inside the .gag directory\n");

        // We remove the \.gag out of the working_dir string
        *(ptr - 1) = '\0';
        printf("The working dir is %s\n", working_dir);
        return INSIDE_REPO;
    }

    struct stat s = {0};
    char gag_dir[1005 + 1] = "\0";
    strcpy_s(gag_dir, sizeof(gag_dir), working_dir);
    while ((ptr = strrchr(working_dir, '\\')) != NULL &&
           strcat_s(gag_dir, sizeof(gag_dir), "\\.gag") == 0) {
        stat(gag_dir, &s);

        if (s.st_mode & S_IFDIR) {
            printf("The working dir is %s\n", working_dir);
            return PRESENT;
        }

        *ptr = '\0';
        gag_dir[ptr - working_dir] = '\0';
        printf("Working dir has been updated to %s\n", working_dir);
    }

    printf("There is no working dir\n");
    working_dir = NULL;
    return NOT_PRESENT;
}

int gag_init() {
    char buffer[1000];
    GAG_State state = get_gag_dir(buffer);
    if (state != NOT_PRESENT) {
        perror(
            "GAG Repository already exists cannot initialize inside existing "
            "repository\n");
        return 1;
    }
    //
    // Create necessary files
    _mkdir(".gag");  // Yes I am on Windows SHUT UP
    _mkdir(".gag\\objects");
    _mkdir(".gag\\refs");

    // Note, currently erases pre-existing file
    FILE *head_file;
    fopen_s(&head_file, ".gag/HEAD", "w+");

    if (head_file == NULL) {
        perror("Failed to create .gag/HEAD file.\n");
        return 1;
    }

    fprintf(head_file, "ref: refs/heads/main\n");
    fclose(head_file);

    printf("Initialized GAG directory\n");
    return 0;
}

int gag_catfile(char *hash) {
    char dir[3];
    char *file_name = (hash + 2);
    dir[0] = hash[0];
    dir[1] = hash[1];
    dir[2] = '\0';
    printf("The file string is: %s\n", file_name);
    printf("The dir string is: %s\n", dir);

    // TODO change git to gag
    char file_path[55] = ".git/objects/";
    strcat_s(file_path, sizeof(file_path), dir);
    strcat_s(file_path, sizeof(file_path), "/");
    strcat_s(file_path, sizeof(file_path), file_name);
    printf("The file_path string is: %s with size %llu\n", file_path,
           strlen(file_path));

    FILE *file;
    fopen_s(&file, file_path, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char *file_contents = (char *)malloc((file_size + 1) * sizeof(char));
    if (file_contents == NULL) {
        perror("Memory allocation error for file_contents");
        fclose(file);
        return 1;
    }

    fread(file_contents, 1, file_size, file);
    file_contents[file_size] = '\0';
    fclose(file);

    printf("File contents:\n%s\n", file_contents);

    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    memset(&strm, 0, sizeof(strm));

    if (inflateInit(&strm) != Z_OK) {
        perror("Failed to initialize zlib\n");
        free(file_contents);
        return 1;
    }

    strm.next_in = (unsigned char *)file_contents;
    strm.avail_in = file_size + 1;

    size_t decompressed_size = 16;
    unsigned char *decompressed_data =
        (unsigned char *)malloc(decompressed_size * sizeof(char));

    if (decompressed_data == NULL) {
        perror("Memory allocation error for decompressed_data\n");
        free(file_contents);
        return 1;
    }

    strm.next_out = decompressed_data;
    strm.avail_out = decompressed_size;
    int ret = inflate(&strm, Z_NO_FLUSH);

    if (ret != Z_OK && ret != Z_STREAM_END) {
        fprintf(stderr, "Decompression error: %d\n", ret);
        inflateEnd(&strm);
        free(file_contents);
        free(decompressed_data);
        return 1;
    }

    char *endptr;
    printf("Decompressed data: %s\n", decompressed_data);

    decompressed_size = strtol((char *)decompressed_data + 5, &endptr,
                               10);  // "blob".len() == 5

    // Gets index of next character to '\0'
    int index = (unsigned char *)endptr - decompressed_data + 1;
    printf("Length of Decompressed File: %lld\n", decompressed_size);
    assert(decompressed_size >= 0);

    unsigned char *new_buffer = realloc(
        decompressed_data, (16 + decompressed_size) * sizeof(unsigned char));
    if (new_buffer == NULL) {
        perror("Memory reallocation error for new_buffer\n");
        inflateEnd(&strm);
        free(file_contents);
        free(decompressed_data);
        return 1;
    }

    decompressed_data = new_buffer;

    strm.next_out = decompressed_data + 16;
    strm.avail_out = decompressed_size;

    ret = inflate(&strm, Z_NO_FLUSH);

    if (ret != Z_OK && ret != Z_STREAM_END) {
        fprintf(stderr, "Decompression error: %d\n", ret);
        inflateEnd(&strm);
        free(file_contents);
        free(decompressed_data);
        return 1;
    }

    printf("Fully Decompressed File:\n%s", decompressed_data + index);

    inflateEnd(&strm);
    free(file_contents);
    free(decompressed_data);
    return 0;
}
