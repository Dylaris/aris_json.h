#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "third_party/nob.h"

#define BUILD_FOLDER "build/"
#define SRC_FOLDER   "examples/"

static const char *srcs[] = {
    SRC_FOLDER"serialization/object_array.c",
    SRC_FOLDER"serialization/array.c",
    SRC_FOLDER"serialization/all.c",
    SRC_FOLDER"serialization/object.c",
    SRC_FOLDER"serialization/nested_array.c",
    SRC_FOLDER"serialization/nested_object.c",
    SRC_FOLDER"deserialization/array.c",
    SRC_FOLDER"deserialization/object.c",
    SRC_FOLDER"deserialization/merge_json.c",
};

static const char *exes[] = {
    BUILD_FOLDER"serialization/object_array",
    BUILD_FOLDER"serialization/array",
    BUILD_FOLDER"serialization/all",
    BUILD_FOLDER"serialization/object",
    BUILD_FOLDER"serialization/nested_array",
    BUILD_FOLDER"serialization/nested_object",
    BUILD_FOLDER"deserialization/array",
    BUILD_FOLDER"deserialization/object",
    BUILD_FOLDER"deserialization/merge_json",
};

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    if (!mkdir_if_not_exists(BUILD_FOLDER)) return 1;
    if (!mkdir_if_not_exists(BUILD_FOLDER"serialization/")) return 1;
    if (!mkdir_if_not_exists(BUILD_FOLDER"deserialization/")) return 1;

    for (size_t i = 0; i < ARRAY_LEN(srcs); i++) {
        Cmd cmd = {0};
        cmd_append(&cmd, "cc",
            "-Wall", "-Wextra",
            "-Wno-unused-function",
            "-ggdb",
            "-I", "./", "-I", "./third_party",
            "-o", exes[i], srcs[i]);
        if (!cmd_run(&cmd)) return 1;
    }

    if (!nob_copy_file(SRC_FOLDER"deserialization/test1.json", BUILD_FOLDER"deserialization/test1.json")) return 1;
    if (!nob_copy_file(SRC_FOLDER"deserialization/test2.json", BUILD_FOLDER"deserialization/test2.json")) return 1;

    return 0;
}

