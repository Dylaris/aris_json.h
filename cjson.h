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
    CJSON_NULL,
    CJSON_OBJECT,
    CJSON_ARRAY,
    CJSON_STRING,
    CJSON_NUMBER,
    CJSON_BOOLEAN,
} cjson_type_t;

#define INDENT "    "

/////////////////////////////////////////////
////// structure
/////////////////////////////////////////////

typedef struct cjson_pair_t cjson_pair_t;
typedef struct cjson_value_t cjson_value_t;

struct cjson_value_t {
    cjson_type_t type;
    union {
        char *string;
        float number;
        bool boolean;
        struct {
            unsigned count;
            cjson_pair_t *pairs; 
        } object;
        struct {
            unsigned count;
            cjson_value_t *values;
        } array;
    } as;
};

struct cjson_pair_t {
    char *key;
    cjson_value_t value;
    cjson_pair_t *next;
};

/////////////////////////////////////////////
////// public function
/////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

void cjson_start(const char *path);
void cjson_end(void);
cjson_pair_t cjson_pair(char *key, cjson_value_t value);
char *cjson_key(const char *key);
cjson_value_t cjson_string(const char *value);
cjson_value_t cjson_number(float value);
cjson_value_t cjson_boolean(bool value);
cjson_value_t cjson_array(unsigned count, ...);
cjson_value_t cjson_object(unsigned count, ...);
cjson_value_t cjson_null(void);

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

static void dump_pair(int level, cjson_pair_t *pair, bool comma);
static void dump_value(int level, cjson_value_t *value, bool indent);
static void dump_indent(int level);
static void free_pair(cjson_pair_t *pair);
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
    case CJSON_OBJECT: {
        fwrite("{\n", 2, 1, __outfp);
        for (unsigned i = 0; i < value->as.object.count; i++) {
            cjson_pair_t *pair = &value->as.object.pairs[i];
            bool comma = true;
            if (i == value->as.object.count - 1) comma = false;
            dump_pair(level+1, pair, comma);
        }
        dump_indent(level);
        fwrite("}", 1, 1, __outfp);
    } break;

    case CJSON_ARRAY: {
        fwrite("[\n", 2, 1, __outfp);
        for (unsigned i = 0; i < value->as.array.count; i++) {
            dump_value(level+1, &value->as.array.values[i], true);
            if (i == value->as.array.count - 1) {
                fwrite("\n", 1, 1, __outfp);
            } else {
                fwrite(",\n", 2, 1, __outfp);
            }
        }
        dump_indent(level);
        fwrite("]", 1, 1, __outfp);
    } break;

    case CJSON_STRING: {
        fwrite("\"", 1, 1, __outfp);
        fwrite(value->as.string, 
               strlen(value->as.string), 1, __outfp);
        fwrite("\"", 1, 1, __outfp);
    } break;

    case CJSON_NUMBER: {
        char str[50];
        snprintf(str, sizeof(str), "%.5g", value->as.number);
        fwrite(str, strlen(str), 1, __outfp);
    } break;

    case CJSON_BOOLEAN: {
        if (value->as.boolean) {
            fwrite("true", 4, 1, __outfp);
        } else {
            fwrite("false", 5, 1, __outfp);
        }
    } break;

    case CJSON_NULL: {
        fwrite("null", 4, 1, __outfp);
    } break;

    default:
        fprintf(stderr, "ERROR: unknown type\n");
        exit(1);
    }
}

static void dump_pair(int level, cjson_pair_t *pair, bool comma)
{
    dump_indent(level);

    // dump key
    fwrite("\"", 1, 1, __outfp);
    fwrite(pair->key, strlen(pair->key), 1, __outfp);
    fwrite("\": ", 3, 1, __outfp);

    dump_value(level, &pair->value, false);

    if (comma) {
        fwrite(",\n", 2, 1, __outfp);
    } else {
        fwrite("\n", 2, 1, __outfp);
    }
}

static void free_value(cjson_value_t *value)
{
    switch (value->type) {
    case CJSON_OBJECT: {
        cjson_pair_t *cur = &value->as.object.pairs[0];
        while (cur) {
            cjson_pair_t *tmp = cur->next;
            free_pair(cur);
            cur = tmp;
        }
        if (value->as.object.pairs) free(value->as.object.pairs);
    } break;

    case CJSON_ARRAY: {
        for (unsigned i = 0; i < value->as.array.count; i++) {
            free_value(&value->as.array.values[i]);
        }
        if (value->as.array.values) free(value->as.array.values);
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
    __this.type = CJSON_OBJECT;
}

void cjson_end(void)
{
    dump_value(0, &__this, true);

    free_value(&__this);

    fclose(__outfp);
}

cjson_pair_t cjson_pair(char *key, cjson_value_t value)
{
    assert(key != NULL);

    return (cjson_pair_t){
        .key = key,         // key is from cjson_key(), so do not need to strdup
        .value = value,
        .next = NULL,
    };
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
        .type = CJSON_STRING,
        .as.string = strdup(value),
    };
}

cjson_value_t cjson_number(float value)
{
    return (cjson_value_t){
        .type = CJSON_NUMBER,
        .as.number = value,
    };
}

cjson_value_t cjson_boolean(bool value)
{
    return (cjson_value_t){
        .type = CJSON_BOOLEAN,
        .as.boolean = value,
    };
}

cjson_value_t cjson_array(unsigned count, ...)
{
    cjson_value_t res = {
        .type = CJSON_ARRAY,
        .as.array.count = 0,
        .as.array.values = NULL,
    };

    if (count == 0) return res;

    res.as.array.count = count;
    res.as.array.values = malloc(count*sizeof(cjson_value_t));
    assert(res.as.array.values != NULL); 

    va_list args;
    va_start(args, count);

    for (unsigned i = 0; i < count; i++) {
        res.as.array.values[i] = va_arg(args, cjson_value_t);
    }

    va_end(args);

    return res;
}

cjson_value_t cjson_object(unsigned count, ...)
{
    cjson_value_t res = {
        .type = CJSON_OBJECT,
        .as.object.count = 0,
        .as.object.pairs = NULL,
    };

    if (count == 0) return res;

    res.as.object.count = count;
    res.as.object.pairs = malloc(count*sizeof(cjson_pair_t));
    assert(res.as.object.pairs != NULL); 

    va_list args;
    va_start(args, count);

    for (unsigned i = 0; i < count; i++) {
        res.as.object.pairs[i] = va_arg(args, cjson_pair_t);
    }

    va_end(args);

    // Record the whole json using '__this'
    __this.as.object = res.as.object;

    // Link the pairs in current object context 
    for (unsigned i = 0; i < count-1; i++) {
        res.as.object.pairs[i].next = &res.as.object.pairs[i+1];
    }
    res.as.object.pairs[count-1].next = NULL;

    return res;
}

cjson_value_t cjson_null(void)
{
    return (cjson_value_t){
        .type = CJSON_NULL,
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
