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
    CJSON_NULL = 0,
    CJSON_OBJECT,
    CJSON_ARRAY,
    CJSON_STRING,
    CJSON_NUMBER,
    CJSON_BOOLEAN,
} cjson_value_type_t;

typedef enum {
    CJSON_SCOPE_NULL = 0,
    CJSON_SCOPE_OBJECT,
    CJSON_SCOPE_ARRAY,
} cjson_scope_type_t;

#define INDENT "    "

/////////////////////////////////////////////
////// structure
/////////////////////////////////////////////

typedef struct cjson_pair_t cjson_pair_t;
typedef struct cjson_value_t cjson_value_t;

struct cjson_value_t {
    cjson_value_type_t type;
    union {
        char *string;
        float number;
        bool boolean;
        struct {
            unsigned count;
            unsigned capacity;
            cjson_pair_t *items; 
        } object;
        struct {
            unsigned count;
            unsigned capacity;
            cjson_value_t *items;
        } array;
    } as;
};

struct cjson_pair_t {
    char *key;
    cjson_value_t value;
};

typedef struct {
    unsigned count;
    unsigned capacity;
    cjson_value_t *items;
} cjson_scope_t;

typedef struct {
    FILE *output;
    char *indent;

    char *key;
    struct {
        unsigned count;
        unsigned capacity;
        cjson_pair_t *items;
    } scopes;
    cjson_scope_type_t scope_type;

    cjson_value_t *root;
} cjson_t;

/////////////////////////////////////////////
////// public function
/////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

void cjson_init(cjson_t *cj, FILE *fp, const char *indent);
void cjson_fini(cjson_t *cj);
cjson_pair_t cjson_pair(char *key, cjson_value_t value);
void cjson_key(cjson_t *cj, const char *key);
void cjson_string(cjson_t *cj, const char *value);
void cjson_number(cjson_t *cj, float value);
void cjson_boolean(cjson_t *cj, bool value);
void cjson_null(cjson_t *cj);
void cjson_array_begin(cjson_t *cj, const char *key);
void cjson_array_end(cjson_t *cj);
void cjson_object_begin(cjson_t *cj, const char *key);
void cjson_object_end(cjson_t *cj);

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

#define dyna_append(da, item)                                           \
    do {                                                                \
        if ((da)->capacity <= (da)->count) {                            \
            (da)->capacity = (da)->capacity==0 ? 16 : 2*(da)->capacity; \
            (da)->items = realloc((da)->items,                          \
                    sizeof(*(da)->items)*(da)->capacity);               \
            assert((da)->items != NULL);                                \
        }                                                               \
        (da)->items[(da)->count++] = (item);                            \
    } while (0)

static void dump_pair(cjson_t *cj, int level, cjson_pair_t *pair, bool comma);
static void dump_value(cjson_t *cj, int level, cjson_value_t *value, bool indent);
static void dump_indent(cjson_t *cj, int level);
static void free_pair(cjson_pair_t *pair);
static void free_value(cjson_value_t *value);

static void dump_indent(cjson_t *cj, int level)
{
    for (int i = 0; i < level; i++) {
        fwrite(cj->indent, strlen(cj->indent), 1, cj->output);
    }
}

static void dump_value(cjson_t *cj, int level, cjson_value_t *value, bool indent)
{
    if (indent) dump_indent(cj, level);

    switch (value->type) {
    case CJSON_OBJECT: {
        fwrite("{\n", 2, 1, cj->output);
        for (unsigned i = 0; i < value->as.object.count; i++) {
            cjson_pair_t *pair = &value->as.object.items[i];
            bool comma = true;
            if (i == value->as.object.count - 1) comma = false;
            dump_pair(cj, level+1, pair, comma);
        }
        dump_indent(cj, level);
        fwrite("}", 1, 1, cj->output);
    } break;

    case CJSON_ARRAY: {
        fwrite("[\n", 2, 1, cj->output);
        for (unsigned i = 0; i < value->as.array.count; i++) {
            dump_value(cj, level+1, &value->as.array.items[i], true);
            if (i == value->as.array.count - 1) {
                fwrite("\n", 1, 1, cj->output);
            } else {
                fwrite(",\n", 2, 1, cj->output);
            }
        }
        dump_indent(cj, level);
        fwrite("]", 1, 1, cj->output);
    } break;

    case CJSON_STRING: {
        fwrite("\"", 1, 1, cj->output);
        fwrite(value->as.string, 
               strlen(value->as.string), 1, cj->output);
        fwrite("\"", 1, 1, cj->output);
    } break;

    case CJSON_NUMBER: {
        char str[50];
        snprintf(str, sizeof(str), "%.5g", value->as.number);
        fwrite(str, strlen(str), 1, cj->output);
    } break;

    case CJSON_BOOLEAN: {
        if (value->as.boolean) {
            fwrite("true", 4, 1, cj->output);
        } else {
            fwrite("false", 5, 1, cj->output);
        }
    } break;

    case CJSON_NULL: {
        fwrite("null", 4, 1, cj->output);
    } break;

    default:
        fprintf(stderr, "ERROR: unknown type\n");
        exit(1);
    }
}

static void dump_pair(cjson_t *cj, int level, cjson_pair_t *pair, bool comma)
{
    dump_indent(cj, level);

    // dump key
    fwrite("\"", 1, 1, cj->output);
    fwrite(pair->key, strlen(pair->key), 1, cj->output);
    fwrite("\": ", 3, 1, cj->output);

    dump_value(cj, level, &pair->value, false);

    if (comma) {
        fwrite(",\n", 2, 1, cj->output);
    } else {
        fwrite("\n", 2, 1, cj->output);
    }
}

static void free_value(cjson_value_t *value)
{
    switch (value->type) {
    case CJSON_OBJECT: {
        cjson_pair_t *cur = &value->as.object.items[0];
        while (cur) {
            cjson_pair_t *tmp = NULL;
            free_pair(cur);
            cur = tmp;
        }
        if (value->as.object.items) free(value->as.object.items);
    } break;

    case CJSON_ARRAY: {
        for (unsigned i = 0; i < value->as.array.count; i++) {
            free_value(&value->as.array.items[i]);
        }
        if (value->as.array.items) free(value->as.array.items);
    } break;

    case CJSON_STRING: {
        if (value->as.string) free(value->as.string);
    } break;

    case CJSON_NUMBER:
    case CJSON_BOOLEAN:
    case CJSON_NULL:
        break;

    default:
        fprintf(stderr, "ERROR: unknown type\n");
        exit(1);
    }
}

static void free_pair(cjson_pair_t *pair)
{
    free(pair->key);
    free_value(&pair->value);
}

static cjson_pair_t *cjson__current_scope(cjson_t *cj)
{
    if (cj->scopes.count == 0) {
        fprintf(stderr, "ERROR: no scope!\n");
        exit(1);
    }
    return &cj->scopes.items[cj->scopes.count - 1];
}

static cjson_pair_t cjson__pop_scope(cjson_t *cj)
{
    if (cj->scopes.count == 0) {
        fprintf(stderr, "ERROR: scope underflow!\n");
        exit(1);
    }

    cjson_pair_t res = cj->scopes.items[--cj->scopes.count];

    if (cj->scopes.count > 0) {
        cjson_pair_t *scope = cjson__current_scope(cj);
        if (scope->value.type == CJSON_ARRAY) {
            cj->scope_type = CJSON_SCOPE_ARRAY;
        } else {
            cj->scope_type = CJSON_SCOPE_OBJECT;
        }
    }

    return res;
}

static void cjson__push_scope(cjson_t *cj, cjson_pair_t scope)
{
    dyna_append(&cj->scopes, scope);
    if (scope.value.type == CJSON_ARRAY) {
        cj->scope_type = CJSON_SCOPE_ARRAY;
    } else {
        cj->scope_type = CJSON_SCOPE_OBJECT;
    }
}

/////////////////////////////////////////////
////// public function
/////////////////////////////////////////////

void cjson_init(cjson_t *cj, FILE *fp, const char *indent)
{
    assert(cj != NULL && fp != NULL && indent != NULL);

    cj->output = fp;
    cj->indent = strdup(indent);
    cj->key = NULL;
    cj->scopes.count = 0;
    cj->scopes.capacity = 0;
    cj->scopes.items = NULL;
    cj->scope_type = CJSON_SCOPE_NULL;
    cj->root = NULL;
}

void cjson_fini(cjson_t *cj)
{
    dump_value(cj, 0, cj->root, true);
    if (cj->indent) free(cj->indent);
    if (cj->scopes.items) free(cj->scopes.items);
    memset(cj, 0, sizeof(cjson_t));
}

cjson_pair_t cjson_pair(char *key, cjson_value_t value)
{
    return (cjson_pair_t){
        .key = key,         // key is from cjson_key(), so do not need to strdup
        .value = value,
    };
}

void cjson_key(cjson_t *cj, const char *key)
{
    // TODO: consider the case that key has already exist
    assert(cj != NULL && key != NULL);
    cj->key = strdup(key);
}

#define CJSON_APPEND_VALUE()                                      \
    do {                                                          \
        cjson_pair_t *scope = cjson__current_scope(cj);           \
                                                                  \
        if (cj->scope_type == CJSON_SCOPE_OBJECT) {               \
            cjson_pair_t pair = cjson_pair(cj->key, item);        \
            dyna_append(&scope->value.as.object, pair);           \
        } else if (cj->scope_type == CJSON_SCOPE_ARRAY) {         \
            dyna_append(&scope->value.as.array, item);            \
        }                                                         \
    } while (0)

void cjson_string(cjson_t *cj, const char *value)
{
    assert(cj != NULL && value != NULL);

    cjson_value_t item = {
        .type = CJSON_STRING,
        .as.string = strdup(value),
    };
    CJSON_APPEND_VALUE();
}

void cjson_number(cjson_t *cj, float value)
{
    assert(cj != NULL);

    cjson_value_t item = {
        .type = CJSON_NUMBER,
        .as.number = value,
    };
    CJSON_APPEND_VALUE();
}

void cjson_boolean(cjson_t *cj, bool value)
{
    assert(cj != NULL);

    cjson_value_t item = {
        .type = CJSON_BOOLEAN,
        .as.boolean = value,
    };
    CJSON_APPEND_VALUE();
}

void cjson_null(cjson_t *cj)
{
    assert(cj != NULL);

    cjson_value_t item = {
        .type = CJSON_NULL,
    };
    CJSON_APPEND_VALUE();
}

void cjson_array_begin(cjson_t *cj, const char *key)
{
    cjson_value_t value = {
        .type = CJSON_ARRAY,
        .as.array.count = 0,
        .as.array.capacity = 0,
        .as.array.items = NULL,
    };
    cjson__push_scope(cj, cjson_pair(strdup(key), value));
}

void cjson_array_end(cjson_t *cj)
{
    cjson_pair_t array = cjson__pop_scope(cj);
    cj->key = array.key;
    cjson_value_t item = array.value;
    CJSON_APPEND_VALUE();
}

void cjson_object_begin(cjson_t *cj, const char *key)
{
    cjson_value_t value = {
        .type = CJSON_OBJECT,
        .as.object.count = 0,
        .as.object.capacity = 0,
        .as.object.items = NULL,
    };
    if (!key) {
        cjson__push_scope(cj, cjson_pair(NULL, value));
        cj->root = &cj->scopes.items[0].value;
    } else {
        cjson__push_scope(cj, cjson_pair(strdup(key), value));
    }
}

void cjson_object_end(cjson_t *cj)
{
    if (cj->scopes.count == 1) return;
    cjson_pair_t object = cjson__pop_scope(cj);
    cj->key = object.key;
    cjson_value_t item = object.value;
    CJSON_APPEND_VALUE();
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
