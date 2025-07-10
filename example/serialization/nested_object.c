#define CJSON_IMPLEMENTATION
#include "cjson.h"

int main(void)
{
    CJson_Context cj = {0};
    cjson_init(&cj, stdout, "\t");

    cjson_object_begin(&cj);
        cjson_key(&cj, "outside object");
        cjson_object_begin(&cj);
            cjson_key(&cj, "boolean");
            cjson_boolean(&cj, false);

            cjson_key(&cj, "inside object");
            cjson_object_begin(&cj);
                cjson_key(&cj, "boolean");
                cjson_boolean(&cj, true);
            cjson_object_end(&cj);
        cjson_object_end(&cj);
    cjson_object_end(&cj);

    cjson_dump(&cj);

    cjson_fini(&cj);
    return 0;
}
