#define JSON_IMPLEMENTATION
#define JSON_ENABLE_DESERIALIZATION
#include "json.h"

const char *array = "[1, 2, 3, true, false, \"hello\"]";

int main(void)
{
    Json_Context ctx;
    json_init(&ctx);

    if (!json_parse(&ctx, array, strlen(array))) return 1;
    const Json_Value *root = json_context_get_root(&ctx);
    for (size_t i = 0; i < json_array_get_size(root); i++) {
        const Json_Value *value = json_array_get_value(root, i);
        json_print_value(value);
    }

    json_fini(&ctx);
    return 0;
}
