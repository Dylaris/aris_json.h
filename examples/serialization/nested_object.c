#define ARIS_JSON_IMPLEMENTATION
#define ARIS_JSON_STRIP_PREFIX
#include "aris_json.h"

int main(void)
{
    aris_json_context ctx;
    json_init(&ctx);

    json_object_begin(&ctx);
        json_key(&ctx, "outside object");
        json_object_begin(&ctx);
            json_key(&ctx, "boolean");
            json_boolean(&ctx, false);

            json_key(&ctx, "inside object");
            json_object_begin(&ctx);
                json_key(&ctx, "boolean");
                json_boolean(&ctx, true);
            json_object_end(&ctx);
        json_object_end(&ctx);
    json_object_end(&ctx);

    json_dump(&ctx);

    json_fini(&ctx);
    return 0;
}
