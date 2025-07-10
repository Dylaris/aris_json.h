#define CJSON_IMPLEMENTATION
#include "cjson.h"

int main(void)
{
    CJson_Context cj = {0};
    cjson_init(&cj, stdout, "\t");

    cjson_array_begin(&cj);
        for (int i = 0; i < 4; i++) {
            cjson_number(&cj, (float)i);
        }
    cjson_array_end(&cj);

    cjson_dump(&cj);

    cjson_fini(&cj);
    return 0;
}
