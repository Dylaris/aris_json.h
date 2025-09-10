#define ARIS_JSON_IMPLEMENTATION
#define ARIS_JSON_STRIP_PREFIX
#include "aris_json.h"

int main(void)
{
    aris_json_context ctx;
    json_init(&ctx);

    json_object_begin(&ctx);
        json_key(&ctx, "string");
        json_string(&ctx, "hello");

        json_key(&ctx, "number");
        json_number(&ctx, 1.2);

        json_key(&ctx, "boolean");
        json_boolean(&ctx, false);

        json_key(&ctx, "null");
        json_null(&ctx);
    json_object_end(&ctx);

    json_dump(&ctx);

    json_fini(&ctx);
    return 0;
}
