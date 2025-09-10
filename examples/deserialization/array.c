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
    const aris_json_value *root = json_context_get_root(&ctx);
    for (size_t i = 0; i < json_array_get_size(root); i++) {
        const aris_json_value *value = json_array_get_value(root, i);
        json_print_value(value);
    }

    json_fini(&ctx);
    return 0;
}
