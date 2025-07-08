#define ZST_SBC_NO_PREFIX
#define ZST_SBC_IMPLEMENTATION
#include "zst_sbc.h"

#define CC "clang"
#define EXAMPLE "example/"

#define DIR1 "./example/serialization/"
#define DIR2 "./example/deserialization/"

static const char *srcs[] = {
    DIR1"object_array.c",
    DIR1"array.c",
    DIR1"all.c",
    DIR1"object.c",
    DIR1"nested_array.c",
    DIR1"nested_object.c",

    DIR2"array.c",
    DIR2"object.c",
    DIR2"merge_json.c",
};

static const char *exes[] = {
    DIR1"object_array",
    DIR1"array",
    DIR1"all",
    DIR1"object",
    DIR1"nested_array",
    DIR1"nested_object",

    DIR2"array",
    DIR2"object",
    DIR2"merge_json",
};

int main(int argc, char **argv)
{
    sbc_rebuild_self(CC, argc, argv, "-I", "./build");

    for (size_t i = 0; i < sizeof(srcs)/sizeof(srcs[0]); i++) {
        sbc_cmd_t cmd = {0};
        sbc_cmd_append(&cmd, CC,
                             "-g",
                             "-I", "./",
                             "-I", "./third_party",
                             "-Wall", "-Wextra",
                             "-Wno-unused-function",
                             "-Wno-self-assign",
                             "-O2",
                             "-o", exes[i], srcs[i]);
        sbc_cmd_run(&cmd);
    }

    return 0;
}

