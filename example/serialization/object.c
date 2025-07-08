#define CJSON_IMPLEMENTATION
#include "cjson.h"

int main(void)
{
    cjson_t cj = {0};
    cjson_init(&cj, stdout, "\t");

    cjson_object_begin(&cj);
        cjson_key(&cj, "string");
        cjson_string(&cj, "hello");

        cjson_key(&cj, "number");
        cjson_number(&cj, 1.2);

        cjson_key(&cj, "boolean");
        cjson_boolean(&cj, false);

        cjson_key(&cj, "null");
        cjson_null(&cj);
    cjson_object_end(&cj);

    cjson_dump(&cj);

    cjson_fini(&cj);
    return 0;
}
