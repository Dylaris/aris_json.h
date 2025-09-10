#define ARIS_JSON_IMPLEMENTATION
#define ARIS_JSON_STRIP_PREFIX
#define ARIS_JSON_ENABLE_DESERIALIZATION
#include "aris_json.h"

const char *object = "{\"name\": \"Jack\", \"age\": 20, \"student\": false, }";

int main(void)
{
    aris_json_context ctx;
    json_init(&ctx);

    if (!json_parse(&ctx, object, strlen(object))) return 1;

    const aris_json_value *root = json_get_root(&ctx);
    const aris_json_value *name = json_get_value(root, "name");
    if (json_is_string(name)) printf("name: %s\n", json_to_string(name));

    const aris_json_value *age = json_get_value(root, "age");
    if (json_is_number(age)) printf("age: %d\n", (int)json_to_number(age));

    json_fini(&ctx);
    return 0;
}
