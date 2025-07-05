#define CJSON_IMPLEMENTATION
#include "cjson.h"

int main(void)
{
    cjson_start("output.json");

    cjson_object(
        5,
        cjson_node(
            cjson_key("string"),
            cjson_string("hello")
        ),
        cjson_node(
            cjson_key("number"),
            cjson_number(1.2)
        ),
        cjson_node(
            cjson_key("boolean"),
            cjson_boolean(true)
        ),
        cjson_node(
            cjson_key("array"),
            cjson_array(
                4,
                cjson_string("world"),
                cjson_number(20),
                cjson_boolean(false),
                cjson_array(
                    4,
                    cjson_string("nothing"),
                    cjson_string("what"),
                    cjson_string("null"),
                    cjson_object(
                        2,
                        cjson_node(
                            cjson_key("3-string"),
                            cjson_none()
                        ),
                        cjson_node(
                            cjson_key("3-number"),
                            cjson_none()
                        )
                    )
                )
            )
        ),
        cjson_node(
            cjson_key("object"),
            cjson_object(
                2,
                cjson_node(
                    cjson_key("2-string"),
                    cjson_string("hello")
                ),
                cjson_node(
                    cjson_key("2-number"),
                    cjson_number(2.4)
                )
            )
        )
    );

    cjson_end();

    return 0;
}
