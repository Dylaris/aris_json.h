// BRIEF:
//
// This file implements some functions to serialize JSON using C.
//
// USAGE:
//   
// Write this before you use it:
//     ```
//       #define CJSON_IMPLEMENTATION
//       #include "cjson.h"
//     ```
//
// LICENSE:
//
// See end of file for license information.

#ifndef CJSON_H
#define CJSON_H

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
    char *key;
    cjson_value_t value;
    struct cjson_node_t *next;
};

typedef struct cjson_object_t cjson_object_t;
typedef struct cjson_array_t cjson_array_t;
typedef struct cjson_node_t cjson_node_t;

/////////////////////////////////////////////
////// public function
/////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

void cjson_start(const char *path);
void cjson_end(void);
cjson_node_t *cjson_node(char *key, cjson_value_t value);
char *cjson_key(const char *key);
cjson_value_t cjson_string(const char *value);
cjson_value_t cjson_number(float value);
cjson_value_t cjson_boolean(bool value);
cjson_value_t cjson_array(unsigned count, ...);
cjson_value_t cjson_object(unsigned count, ...);
cjson_value_t cjson_none(void);

#ifdef __cplusplus
}
#endif

#endif // CJSON_H

#ifdef CJSON_IMPLEMENTATION

/////////////////////////////////////////////
////// global variable
/////////////////////////////////////////////

static cjson_value_t __this;    // current json object
static FILE *__outfp;           // output file pointer

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
        fwrite(INDENT, strlen(INDENT), 1, __outfp);
    }
}

static void dump_value(int level, cjson_value_t *value, bool indent)
{
    if (indent) dump_indent(level);

    switch (value->type) {
    case OBJECT: {
        fwrite("{\n", 2, 1, __outfp);
        cjson_object_t *object = value->as.object;
        for (unsigned i = 0; i < object->count; i++) {
            cjson_node_t *node = object->nodes[i];
            dump_node(level+1, node);
        }
        dump_indent(level);
        fwrite("}", 1, 1, __outfp);
    } break;

    case ARRAY: {
        fwrite("[\n", 2, 1, __outfp);
        cjson_array_t *array = value->as.array;
        for (unsigned i = 0; i < array->count; i++) {
            dump_value(level+1, &array->values[i], true);
            if (i == array->count - 1) {
                fwrite("\n", 1, 1, __outfp);
            } else {
                fwrite(",\n", 2, 1, __outfp);
            }
        }
        dump_indent(level);
        fwrite("]", 1, 1, __outfp);
    } break;

    case STRING: {
        fwrite("\"", 1, 1, __outfp);
        fwrite(value->as.string, 
               strlen(value->as.string), 1, __outfp);
        fwrite("\"", 1, 1, __outfp);
    } break;

    case NUMBER: {
        char str[50];
        snprintf(str, sizeof(str), "%.5g", value->as.number);
        fwrite(str, strlen(str), 1, __outfp);
    } break;

    case BOOLEAN: {
        if (value->as.boolean) {
            fwrite("true", 4, 1, __outfp);
        } else {
            fwrite("false", 5, 1, __outfp);
        }
    } break;

    case NONE: {
        fwrite("???", 3, 1, __outfp);
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
    fwrite("\"", 1, 1, __outfp);
    fwrite(node->key, strlen(node->key), 1, __outfp);
    fwrite("\": ", 3, 1, __outfp);

    dump_value(level, &node->value, false);

    fwrite(",\n", 2, 1, __outfp);
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
    case NONE:
        break;

    default:
        fprintf(stderr, "ERROR: unknown type\n");
        exit(1);
    }
}

static void free_node(cjson_node_t *node)
{
    free(node->key);
    free_value(&node->value);
    free(node);
}

/////////////////////////////////////////////
////// public function
/////////////////////////////////////////////

void cjson_start(const char *path)
{
    __outfp = fopen(path, "w");
    if (!__outfp) {
        fprintf(stderr, "ERROR: failed to open file '%s' to write\n", path);
        exit(1);
    }
    __this.type = OBJECT;
}

void cjson_end(void)
{
    dump_value(0, &__this, true);

    free_value(&__this);
    __this.type = NONE;
    __this.as.object = NULL;

    fclose(__outfp);
}

cjson_node_t *cjson_node(char *key, cjson_value_t value)
{
    cjson_node_t *node = malloc(sizeof(cjson_node_t));
    assert(node != NULL);
    node->key = key;
    node->value = value;
    node->next = NULL;

    return node;
}

char *cjson_key(const char *key)
{
    // TODO: consider the case that key has already exist
    assert(key != NULL);
    return strdup(key);
}

cjson_value_t cjson_string(const char *value)
{
    assert(value != NULL);

    return (cjson_value_t){
        .type = STRING,
        .as.string = strdup(value),
    };
}

cjson_value_t cjson_number(float value)
{
    return (cjson_value_t){
        .type = NUMBER,
        .as.number = value,
    };
}

cjson_value_t cjson_boolean(bool value)
{
    return (cjson_value_t){
        .type = BOOLEAN,
        .as.boolean = value,
    };
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

    // Record the whole json using '__this'
    __this.as.object = object;

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

cjson_value_t cjson_none(void)
{
    return (cjson_value_t){
        .type = NONE,
    };
}

#endif // CJSON_IMPLEMENTATION

// ------------------------------------------------------------------------------
// This software is available under MIT License
// ------------------------------------------------------------------------------
// Copyright (c) 2025 Dylaris
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
