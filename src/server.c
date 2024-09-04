#include <assert.h>
#include <direct.h>  // Necessesary to make directories
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <zlib.h>

// Returns NULL if none of the parent dirs do not have .gag directory
// Otherwise returns the full path of said parent dir
void get_gag_dir(char working_dir[1000]) {
    _getcwd(working_dir, 1000);

    if (strstr(working_dir, ".gag") != NULL) {
        printf("The working_dir is inside the .gag directory\n");
        working_dir = "";
        return;
    }

    struct stat s = {0};
    char *ptr;
    char gag_dir[1005 + 1] = "\0";
    strcpy_s(gag_dir, sizeof(gag_dir), working_dir);
    while ((ptr = strrchr(working_dir, '\\')) != NULL && strcat_s(gag_dir, sizeof(gag_dir), "\\.gag") == 0) {
        stat(gag_dir, &s);

        if (s.st_mode & S_IFDIR) {
            printf("The working dir is %s\n", working_dir);
            return;
        }

        *ptr = '\0';
        gag_dir[ptr - working_dir] = '\0';
        printf("Working dir has been updated to %s\n", working_dir);
    }

    printf("There is no working dir\n");
    working_dir = NULL;
    return;
}

int main(int argc, char *argv[]) {
    // Flush the buffers
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    char buf[1000];
    get_gag_dir(buf);

    if (argc < 2) {
        perror("No command provided.\n");
        return 1;
    }

    if (strcmp(argv[1], "init") == 0) {
        if (argc != 2) {
            perror("Wrong format for gag init\n");
            return 1;
        }
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

    } else if (strcmp(argv[1], "cat-file") == 0) {
        if (argc != 4 || strcmp(argv[2], "-p") != 0) {
            perror("Wrong format. Expected: gag cat-file -p <hash>\n");
            return 1;

        } else if (strlen(argv[3]) != 40) {
            perror("Not a valid hash\n");
            return 1;
        }

        char *hash = argv[3];
        char dir[3];
        char *file_name = (hash + 2);
        dir[0] = hash[0];
        dir[1] = hash[1];
        dir[2] = '\0';
        printf("The file string is: %s\n", file_name);
        printf("The dir string is: %s\n", dir);

        // TODO change git to gag
        char file_path[4 + 1 + 7 + 1 + 2 + 1 + 38 + 1] = ".git/objects/";
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
        unsigned char *decompressed_data = (unsigned char *)malloc(decompressed_size * sizeof(char));

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

        decompressed_size = strtol((char*) decompressed_data + 5, &endptr, 10);  // "blob".len() == 5

        // Gets index of next character to '\0'
        int index = (unsigned char*)endptr - decompressed_data + 1;
        printf("Length of Decompressed File: %lld\n", decompressed_size);
        assert(decompressed_size >= 0);

        unsigned char* new_buffer = realloc(decompressed_data, (16 + decompressed_size) * sizeof(unsigned char));
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

    } else {
        // Note, does not indicate what is the command
        perror("Unknown command\n");
        return 2;
    }

    return 0;
}
