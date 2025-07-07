#define ZST_SBC_NO_PREFIX
#define ZST_SBC_IMPLEMENTATION
#include "zst_sbc.h"

#define CC "clang"
#define EXAMPLE "example/"

int main(int argc, char **argv)
{
    sbc_rebuild_self(CC, argc, argv, "-I", "./build");

    sbc_cmd_t cmd = {0};
    sbc_cmd_append(&cmd, CC,
                         "-g",
                         "-I", "./",
                         "-Wall", "-Wextra",
                         "-O2",
                         "-o", EXAMPLE"c2json", EXAMPLE"c2json.c");
    sbc_cmd_run(&cmd);

    return 0;
}

