#ifndef COMMANDS_H
#define COMMANDS_H

#include <assert.h>
#include <direct.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <zlib.h>

typedef enum {
    PRESENT,
    NOT_PRESENT,
    INSIDE_REPO,
} GAG_State;

GAG_State get_gag_dir(char working_dir[1000]);
int gag_init();
int gag_catfile(char* hash);

#endif
