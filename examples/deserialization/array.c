#define ARIS_JSON_IMPLEMENTATION
#define ARIS_JSON_STRIP_PREFIX
#define ARIS_JSON_ENABLE_DESERIALIZATION
#include "aris_json.h"

const char *array = "[1, 2, 3, true, false, \"hello\"]";

int main(void)
{
    aris_json_context ctx;
    json_init(&ctx);

    if (!json_parse(&ctx, array, strlen(array))) return 1;

    json_dump(&ctx);

    json_fini(&ctx);
    return 0;
}
