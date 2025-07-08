#define CJSON_IMPLEMENTATION
#include "cjson.h"

const char *object = "{\"name\": \"Jack\", \"age\": 20, \"student\": false, }";

int main(void)
{
    cjson_t cj = {0};
    cjson_init(&cj, stdout, "\t");

    if (!cjson_parse(&cj, object, strlen(object))) return 1;

    cjson_value_t *name = cjson_query(cj.root, "name");
    assert(name != NULL);
    printf("name: %s\n", name->as.string);

    cjson_value_t *age = cjson_query(cj.root, "age");
    assert(age != NULL);
    printf("age: %d\n", (int)age->as.number);

    cjson_fini(&cj);
    return 0;
}
