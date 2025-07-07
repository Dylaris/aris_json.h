#define CJSON_IMPLEMENTATION
#include "cjson.h"

int main(void)
{
    cjson_t cj;
    cjson_init(&cj, stdout, "   ");

    cjson_object_begin(&cj);
        cjson_key(&cj, "null");
        cjson_null(&cj);

        cjson_key(&cj, "string");
        cjson_string(&cj, "hello");

        cjson_key(&cj, "number");
        cjson_number(&cj, 1.24);

        cjson_key(&cj, "boolean");
        cjson_boolean(&cj, true);

        cjson_key(&cj, "array");
        cjson_array_begin(&cj);
            cjson_null(&cj);
            cjson_string(&cj, "hello");
            cjson_number(&cj, 1.24);
            cjson_boolean(&cj, true);
        
            cjson_object_begin(&cj);
                cjson_key(&cj, "null");
                cjson_null(&cj);
                   
                cjson_key(&cj, "string");
                cjson_string(&cj, "hello");
                   
                cjson_key(&cj, "number");
                cjson_number(&cj, 1.24);
                   
                cjson_key(&cj, "boolean");
                cjson_boolean(&cj, true);
                   
                cjson_key(&cj, "array");
                cjson_array_begin(&cj);
                    cjson_null(&cj);
                    cjson_string(&cj, "hello");
                    cjson_number(&cj, 1.24);
                    cjson_boolean(&cj, true);
                cjson_array_end(&cj);
            cjson_object_end(&cj);
        cjson_array_end(&cj);

        cjson_key(&cj, "object");
        cjson_object_begin(&cj);
            cjson_key(&cj, "null");
            cjson_null(&cj);
        
            cjson_key(&cj, "string");
            cjson_string(&cj, "hello");
        
            cjson_key(&cj, "number");
            cjson_number(&cj, 1.24);
        
            cjson_key(&cj, "boolean");
            cjson_boolean(&cj, true);
        
            cjson_key(&cj, "array");
            cjson_array_begin(&cj);
                cjson_null(&cj);
                cjson_string(&cj, "hello");
                cjson_number(&cj, 1.24);
                cjson_boolean(&cj, true);
            cjson_array_end(&cj);
        cjson_object_end(&cj);
    cjson_object_end(&cj);

    cjson_fini(&cj);
    return 0;
}
