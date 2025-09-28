#define JSON_IMPLEMENTATION
#define JSON_ENABLE_DESERIALIZATION
#include "json.h"

const char *object = "{\"name\": \"Jack\", \"age\": 20, \"student\": false, }";

int main(void)
{
    Json_Context ctx;
    json_init(&ctx);

    if (!json_parse(&ctx, object, strlen(object))) return 1;

    const Json_Value *root = json_context_get_root(&ctx);
    const Json_Value *name = json_object_get_value(root, "name");
    if (json_is_string(name)) printf("name: %s\n", json_to_string(name));

    const Json_Value *age = json_object_get_value(root, "age");
    if (json_is_number(age)) printf("age: %d\n", (int)json_to_number(age));

    json_fini(&ctx);
    return 0;
}
