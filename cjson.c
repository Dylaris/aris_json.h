#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <assert.h>

typedef enum {
    OBJECT,
    ARRAY,
    STRING,
    NUMBER,
    BOOLEAN,
    NONE,
} cjson_type_t;

#define INDENT "    "

/////////////////////////////////////////////
////// structure
/////////////////////////////////////////////

struct cjson_object_t;
struct cjson_array_t;

typedef struct {
    cjson_type_t type;
    union {
        struct cjson_object_t *object;
        struct cjson_array_t *array;
        char *string;
        float number;
        bool boolean;
    } as;
} cjson_value_t;

struct cjson_object_t {
    unsigned count;
    struct cjson_node_t **nodes;
};

struct cjson_array_t {
    unsigned count;
    cjson_value_t *values;
};

struct cjson_node_t {
    const char *key;
    cjson_value_t value;
    struct cjson_node_t *next;
};

typedef struct cjson_object_t cjson_object_t;
typedef struct cjson_array_t cjson_array_t;
typedef struct cjson_node_t cjson_node_t;

/////////////////////////////////////////////
////// global variable
/////////////////////////////////////////////

static cjson_value_t this;
static FILE *output_fp;

/////////////////////////////////////////////
////// private function
/////////////////////////////////////////////

static void dump_node(int level, cjson_node_t *node);
static void dump_value(int level, cjson_value_t *value, bool indent);
static void dump_indent(int level);
static void free_node(cjson_node_t *node);
static void free_value(cjson_value_t *value);

static void dump_indent(int level)
{
    for (int i = 0; i < level; i++) {
        fwrite(INDENT, strlen(INDENT), 1, output_fp);
    }
}

static void dump_value(int level, cjson_value_t *value, bool indent)
{
    if (indent) dump_indent(level);

    switch (value->type) {
    case OBJECT: {
        fwrite("{\n", 2, 1, output_fp);
        cjson_object_t *object = value->as.object;
        for (unsigned i = 0; i < object->count; i++) {
            cjson_node_t *node = object->nodes[i];
            dump_node(level+1, node);
        }
        dump_indent(level);
        fwrite("}", 1, 1, output_fp);
    } break;

    case ARRAY: {
        fwrite("[\n", 2, 1, output_fp);
        cjson_array_t *array = value->as.array;
        for (unsigned i = 0; i < array->count; i++) {
            dump_value(level+1, &array->values[i], true);
            if (i == array->count - 1) {
                fwrite("\n", 1, 1, output_fp);
            } else {
                fwrite(",\n", 2, 1, output_fp);
            }
        }
        dump_indent(level);
        fwrite("]", 1, 1, output_fp);
    } break;

    case STRING: {
        fwrite("\"", 1, 1, output_fp);
        fwrite(value->as.string, 
               strlen(value->as.string), 1, output_fp);
        fwrite("\"", 1, 1, output_fp);
    } break;

    case NUMBER: {
        char str[50];
        snprintf(str, sizeof(str), "%.5g", value->as.number);
        fwrite(str, strlen(str), 1, output_fp);
    } break;

    case BOOLEAN: {
        if (value->as.boolean) {
            fwrite("true", 4, 1, output_fp);
        } else {
            fwrite("false", 5, 1, output_fp);
        }
    } break;

    default:
        fprintf(stderr, "ERROR: unknown type\n");
        exit(1);
    }
}

static void dump_node(int level, cjson_node_t *node)
{
    dump_indent(level);

    // dump key
    fwrite("\"", 1, 1, output_fp);
    fwrite(node->key, strlen(node->key), 1, output_fp);
    fwrite("\": ", 3, 1, output_fp);

    dump_value(level, &node->value, false);

    fwrite(",\n", 2, 1, output_fp);
}

static void free_value(cjson_value_t *value)
{
    switch (value->type) {
    case OBJECT: {
        cjson_object_t *object = value->as.object;

        cjson_node_t *cur = object->nodes[0];
        while (cur) {
            cjson_node_t *tmp = cur->next;
            free_node(cur);
            cur = tmp;
        }

        if (object->nodes) free(object->nodes);
        free(object);
    } break;

    case ARRAY: {
        cjson_array_t *array = value->as.array;
        for (unsigned i = 0; i < array->count; i++) {
            free_value(&array->values[i]);
        }
        if (array->values) free(array->values);
        free(array);
    } break;

    case STRING: {
        if (value->as.string) free(value->as.string);
    } break;

    case NUMBER:
    case BOOLEAN:
        break;

    default:
        fprintf(stderr, "ERROR: unknown type\n");
        exit(1);
    }
}

static void free_node(cjson_node_t *node)
{
    free_value(&node->value);
    free(node);
}

/////////////////////////////////////////////
////// public function
/////////////////////////////////////////////

void cjson_start(const char *path)
{
    output_fp = fopen(path, "w");
    if (!output_fp) {
        fprintf(stderr, "ERROR: failed to open file '%s' to write\n", path);
        exit(1);
    }
    this.type = OBJECT;
}

void cjson_end(void)
{
    dump_value(0, &this, true);

    free_value(&this);
    this.type = NONE;
    this.as.object = NULL;

    fclose(output_fp);
}

cjson_node_t *cjson_node(const char *key, cjson_value_t value)
{
    cjson_node_t *node = malloc(sizeof(cjson_node_t));
    assert(node != NULL);
    node->key = key;
    node->value = value;
    node->next = NULL;

    return node;
}

const char *cjson_key(const char *key)
{
    // TODO: consider the case that key has already exist
    return key;
}

cjson_value_t cjson_string(const char *value)
{
    assert(value != NULL);

    cjson_value_t res = {
        .type = STRING,
        .as.string = strdup(value),
    };
    return res;
}

cjson_value_t cjson_number(float value)
{
    cjson_value_t res = {
        .type = NUMBER,
        .as.number = value,
    };
    return res;
}

cjson_value_t cjson_boolean(bool value)
{
    cjson_value_t res = {
        .type = BOOLEAN,
        .as.boolean = value,
    };
    return res;
}

cjson_value_t cjson_array(unsigned count, ...)
{
    if (count == 0) {
        return (cjson_value_t) {
            .type = ARRAY,
            .as.array = NULL
        };
    }

    cjson_array_t *array = malloc(sizeof(cjson_array_t));
    assert(array != NULL);
    array->count = count;
    array->values = malloc(count*sizeof(cjson_value_t));
    assert(array->values != NULL); 

    va_list args;
    va_start(args, count);

    for (unsigned i = 0; i < count; i++) {
        array->values[i] = va_arg(args, cjson_value_t);
    }

    va_end(args);

    cjson_value_t res = {
        .type = ARRAY,
        .as.array = array,
    };
    return res;
}

cjson_value_t cjson_object(unsigned count, ...)
{
    if (count == 0) {
        return (cjson_value_t) {
            .type = OBJECT,
            .as.object = NULL
        };
    }

    cjson_object_t *object = malloc(sizeof(cjson_object_t));
    assert(object != NULL);
    object->count = count;
    object->nodes = malloc(count*sizeof(cjson_node_t*));
    assert(object->nodes != NULL); 

    va_list args;
    va_start(args, count);

    for (unsigned i = 0; i < count; i++) {
        object->nodes[i] = va_arg(args, cjson_node_t*);
    }

    va_end(args);

    // Record the whole json using 'this'
    this.as.object = object;

    // Link the nodes in current object context 
    for (unsigned i = 0; i < count-1; i++) {
        object->nodes[i]->next = object->nodes[i+1];
    }
    object->nodes[count-1]->next = NULL;

    cjson_value_t res = {
        .type = OBJECT,
        .as.object = object,
    };
    return res;
}

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
                            cjson_string("hello")
                        ),
                        cjson_node(
                            cjson_key("3-number"),
                            cjson_number(2.4)
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
