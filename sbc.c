#define ZST_SBC_NO_PREFIX
#define ZST_SBC_IMPLEMENTATION
#include "zst_sbc.h"

#define CC "clang"

int main(int argc, char **argv)
{
    sbc_rebuild_self(CC, argc, argv);

    sbc_cmd_t cmd = {0};
    sbc_cmd_append(&cmd, CC);
    sbc_cmd_append(&cmd, "-o", "cjson", "cjson.c");
    sbc_cmd_run(&cmd);

    return 0;
}

