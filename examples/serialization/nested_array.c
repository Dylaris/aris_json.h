#define JSON_IMPLEMENTATION
#include "json.h"

int main(void)
{
    Json_Context ctx;
    json_init(&ctx);

    json_object_begin(&ctx);
        json_key(&ctx, "outside array");
        json_array_begin(&ctx);
            for (int i = 0; i < 3; i++) {
                json_number(&ctx, (double)i);
            }
            json_array_begin(&ctx);
                for (int i = -3; i < 0; i++) {
                    json_number(&ctx, (double)i);
                }
            json_array_end(&ctx);
        json_array_end(&ctx);
    json_object_end(&ctx);

    json_dump(&ctx);

    json_fini(&ctx);
    return 0;
}
