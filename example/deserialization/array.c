#define CJSON_IMPLEMENTATION
#include "cjson.h"

const char *array = "[1, 2, 3, true, false, \"hello\"]";

int main(void)
{
    cjson_t cj = {0};
    cjson_init(&cj, stdout, "\t");

    if (!cjson_parse(&cj, array, strlen(array))) return 1;

    cjson_dump(&cj);

    cjson_fini(&cj);
    return 0;
}
