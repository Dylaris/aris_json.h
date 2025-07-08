// BRIEF:
//
// This file implements some functions to serialize and deserialize JSON using C.
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

/////////////////////////////////////////////
////// constant
/////////////////////////////////////////////

#define MAX_KEY_LEN 128

/////////////////////////////////////////////
////// enum
/////////////////////////////////////////////

typedef enum {
    CJSON_VALUE_NULL = 0,
    CJSON_VALUE_OBJECT,
    CJSON_VALUE_ARRAY,
    CJSON_VALUE_STRING,
    CJSON_VALUE_NUMBER,
    CJSON_VALUE_BOOLEAN,
} cjson_value_type_t;

typedef enum {
    CJSON_SCOPE_NULL = 0,
    CJSON_SCOPE_OBJECT,
    CJSON_SCOPE_ARRAY,
} cjson_scope_type_t;

typedef enum {
    CJSON_OK,
    CJSON_DOUBLE_KEY,
    CJSON_INVALID_KEY,
    CJSON_KEY_OVERFLOW,
    CJSON_SCOPE_OVERFLOW,
    CJSON_SCOPE_UNDERFLOW,
} cjson_errno_t;

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
    FILE *output;
    char *indent;
    struct {
        unsigned count;
        unsigned capacity;
        cjson_pair_t *items;
    } scopes;
    cjson_scope_type_t scope_type;
    char key[MAX_KEY_LEN];  // Record the current member key
    cjson_value_t *root;    // Record the first scope
    cjson_errno_t code;     // Error code
    char *error_key;        // Store the key when error occurred
} cjson_t;

/////////////////////////////////////////////
////// public function
/////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

void cjson_init(cjson_t *cj, FILE *fp, const char *indent);
void cjson_dump(cjson_t *cj);
void cjson_fini(cjson_t *cj);
cjson_value_t *cjson_query(cjson_value_t *root, const char *key);
bool cjson_parse(cjson_t *cj, const char *input, unsigned size);
void cjson_append_element(cjson_t *cj, char *key, cjson_value_t value);

void cjson_key(cjson_t *cj, const char *key);
void cjson_string(cjson_t *cj, const char *value);
void cjson_number(cjson_t *cj, float value);
void cjson_boolean(cjson_t *cj, bool value);
void cjson_null(cjson_t *cj);
void cjson_array_begin(cjson_t *cj);
void cjson_array_end(cjson_t *cj);
void cjson_object_begin(cjson_t *cj);
void cjson_object_end(cjson_t *cj);

#ifdef __cplusplus
}
#endif

#endif // CJSON_H

#ifdef CJSON_IMPLEMENTATION

#define STB_C_LEXER_IMPLEMENTATION
#include "stb_c_lexer.h"

/////////////////////////////////////////////
////// private function
/////////////////////////////////////////////

#define cjson_dyna_append(da, item)                                     \
    do {                                                                \
        if ((da)->capacity <= (da)->count) {                            \
            (da)->capacity = (da)->capacity==0 ? 16 : 2*(da)->capacity; \
            (da)->items = realloc((da)->items,                          \
                    sizeof(*(da)->items)*(da)->capacity);               \
            assert((da)->items != NULL);                                \
        }                                                               \
        (da)->items[(da)->count++] = (item);                            \
    } while (0)

static void cjson__dump_pair(cjson_t *cj, int level, cjson_pair_t *pair, bool comma);
static void cjson__dump_value(cjson_t *cj, int level, cjson_value_t *value, bool indent);
static void cjson__dump_indent(cjson_t *cj, int level);
static void cjson__free_pair(cjson_pair_t *pair);
static void cjson__free_value(cjson_value_t *value);
static cjson_pair_t cjson__pair(char *key, cjson_value_t value);
static cjson_pair_t *cjson__current_scope(cjson_t *cj);
static cjson_pair_t cjson__pop_scope(cjson_t *cj);
static void cjson__push_scope(cjson_t *cj, cjson_pair_t scope);
static bool cjson__is_in_array(cjson_t *cj);
static void cjson__set_error(cjson_t *cj, cjson_errno_t code);
static long peek(stb_lexer *lex);
static long advance(stb_lexer *lex);
static bool consume(stb_lexer *lex, long expected, const char *msg);
static bool parse_value(cjson_t *cj, stb_lexer *lex);
static bool parse_array(cjson_t *cj, stb_lexer *lex);
static bool parse_object(cjson_t *cj, stb_lexer *lex);

static bool parse_array(cjson_t *cj, stb_lexer *lex)
{
    if (!consume(lex, '[', "array should start with '['")) return false;
    cjson_array_begin(cj);

    // Handle empty array
    if (peek(lex) == ']') {
        advance(lex);   // consume ']'
        cjson_array_end(cj);
        return true;
    }

    while (true) {
        if (!parse_value(cj, lex)) return false;

        if (peek(lex) != ',') break;
        advance(lex);   // consume ','
        if (peek(lex) == ']') break;    // Allowing trailing comma at the end of array
    }

    if (!consume(lex, ']', "array should end with ']'")) return false;
    cjson_array_end(cj);

    return true;
}

static bool parse_object(cjson_t *cj, stb_lexer *lex)
{
    if (!consume(lex, '{', "object should start with '{'")) return false;
    cjson_object_begin(cj);

    // Handle empty object
    if (peek(lex) == '}') {
        advance(lex);   // consume '}'
        cjson_object_end(cj);
        return true;
    }

    while (true) {
        // Parse key
        if (!consume(lex, CLEX_dqstring, "key should be a string")) return false;
        cjson_key(cj, lex->string);

        // Parse colon separator
        if (!consume(lex, ':', "lack of ':' in a pair")) return false;

        // Parse value
        if (!parse_value(cj, lex)) return false;

        if (peek(lex) != ',') break;
        advance(lex);   // consume ','
        if (peek(lex) == '}') break;    // Allowing trailing comma at the end of object
    }

    if (!consume(lex, '}', "object should end with '}'")) return false;
    cjson_object_end(cj);

    return true;
}

static bool parse_value(cjson_t *cj, stb_lexer *lex)
{
    long token = advance(lex);
    switch (token) {
    case CLEX_dqstring:
        cjson_string(cj, lex->string);
        return true;
        
    case CLEX_intlit:
        cjson_number(cj, (float)lex->int_number);
        return true;
        
    case CLEX_floatlit:
        cjson_number(cj, (float)lex->real_number);
        return true;
        
    case CLEX_id:
        if (strcmp(lex->string, "null") == 0) {
            cjson_null(cj);
            return true;
        } 
        if (strcmp(lex->string, "true") == 0) {
            cjson_boolean(cj, true);
            return true;
        } 
        if (strcmp(lex->string, "false") == 0) {
            cjson_boolean(cj, false);
            return true;
        }
        return false;
        
    case '{':
        lex->parse_point--;     // Put back the token for parse_object to consume
        return parse_object(cj, lex);
        
    case '[':
        lex->parse_point--;     // Put back the token for parse_array to consume
        return parse_array(cj, lex);
        
    default:
        return false;
    }
}

static const char *token_kind(long token)
{
    switch (token) {
    case CLEX_id: return "identifier";
    case CLEX_dqstring: return "double quote string";
    case CLEX_sqstring: return "single quote string";
    case CLEX_charlit: return "character";
    case CLEX_intlit: return "integer";
    case CLEX_floatlit: return "float";
    default:
        if (token >= 0 && token < 256) {
            static char tmp_buf[16];
            snprintf(tmp_buf, sizeof(tmp_buf), "%c", (char)token);
            return tmp_buf;
        } else {
            return "unknown token";
        }
    }
}

static long peek(stb_lexer *lex)
{
    char *saved_point = lex->parse_point;
    assert(stb_c_lexer_get_token(lex));
    long token = lex->token;
    lex->parse_point = saved_point;
    return token;
}

static long advance(stb_lexer *lex)
{
    assert(stb_c_lexer_get_token(lex));
    return lex->token;
}

static bool consume(stb_lexer *lex, long expected, const char *msg)
{
    long token = advance(lex);
    if (token != expected) {
        stb_lex_location loc = {0};
        stb_c_lexer_get_location(lex, lex->where_firstchar, &loc);
        fprintf(stderr, "ERROR: %s (expected '%s' but found '%s') at %d:%d\n",
                msg, token_kind(expected), token_kind(token),
                loc.line_number, loc.line_offset);
        return false;
    }
    return true;
}

static void cjson__set_error(cjson_t *cj, cjson_errno_t code)
{
    cj->code = code;

    switch (code) {
    case CJSON_OK:
        break;

    case CJSON_DOUBLE_KEY:
        fprintf(stderr, "ERROR: double key '%s'!\n", cj->error_key);
        break;

    case CJSON_INVALID_KEY:
        fprintf(stderr, "ERROR: invalid key '%s'!\n", cj->error_key);
        break;

    case CJSON_KEY_OVERFLOW:
        fprintf(stderr, "ERROR: key overflow '%s' (maxsize = %d)!\n",
                cj->error_key, MAX_KEY_LEN);
        break;

    case CJSON_SCOPE_OVERFLOW:
        fprintf(stderr, "ERROR: scope overflow!\n");
        break;

    case CJSON_SCOPE_UNDERFLOW:
        fprintf(stderr, "ERROR: scope underflow!\n");
        break;

    default:
        fprintf(stderr, "ERROR: unknown code!\n");
        break;
    }
}

static void cjson__dump_indent(cjson_t *cj, int level)
{
    if (cj->code != CJSON_OK) return;
    for (int i = 0; i < level; i++) {
        fwrite(cj->indent, strlen(cj->indent), 1, cj->output);
    }
}

static void cjson__dump_value(cjson_t *cj, int level, cjson_value_t *value, bool indent)
{
    if (cj->code != CJSON_OK) return;

    if (indent) cjson__dump_indent(cj, level);

    switch (value->type) {
    case CJSON_VALUE_OBJECT: {
        fwrite("{\n", 2, 1, cj->output);
        for (unsigned i = 0; i < value->as.object.count; i++) {
            cjson_pair_t *pair = &value->as.object.items[i];
            bool comma = true;
            if (i == value->as.object.count - 1) comma = false;
            cjson__dump_pair(cj, level+1, pair, comma);
        }
        cjson__dump_indent(cj, level);
        fwrite("}", 1, 1, cj->output);
    } break;

    case CJSON_VALUE_ARRAY: {
        fwrite("[\n", 2, 1, cj->output);
        for (unsigned i = 0; i < value->as.array.count; i++) {
            cjson__dump_value(cj, level+1, &value->as.array.items[i], true);
            if (i == value->as.array.count - 1) {
                fwrite("\n", 1, 1, cj->output);
            } else {
                fwrite(",\n", 2, 1, cj->output);
            }
        }
        cjson__dump_indent(cj, level);
        fwrite("]", 1, 1, cj->output);
    } break;

    case CJSON_VALUE_STRING: {
        fwrite("\"", 1, 1, cj->output);
        fwrite(value->as.string, 
               strlen(value->as.string), 1, cj->output);
        fwrite("\"", 1, 1, cj->output);
    } break;

    case CJSON_VALUE_NUMBER: {
        char str[50];
        snprintf(str, sizeof(str), "%.5g", value->as.number);
        fwrite(str, strlen(str), 1, cj->output);
    } break;

    case CJSON_VALUE_BOOLEAN: {
        if (value->as.boolean) {
            fwrite("true", 4, 1, cj->output);
        } else {
            fwrite("false", 5, 1, cj->output);
        }
    } break;

    case CJSON_VALUE_NULL: {
        fwrite("null", 4, 1, cj->output);
    } break;

    default:
        fprintf(stderr, "ERROR: unknown type\n");
        exit(1);
    }
}

static void cjson__dump_pair(cjson_t *cj, int level, cjson_pair_t *pair, bool comma)
{
    if (cj->code != CJSON_OK) return;

    cjson__dump_indent(cj, level);

    // dump key
    fwrite("\"", 1, 1, cj->output);
    fwrite(pair->key, strlen(pair->key), 1, cj->output);
    fwrite("\": ", 3, 1, cj->output);

    cjson__dump_value(cj, level, &pair->value, false);

    if (comma) {
        fwrite(",\n", 2, 1, cj->output);
    } else {
        fwrite("\n", 2, 1, cj->output);
    }
}

static void cjson__free_value(cjson_value_t *value)
{
    switch (value->type) {
    case CJSON_VALUE_OBJECT: {
        for (unsigned i = 0; i < value->as.object.count; i++) {
            cjson__free_pair(&value->as.object.items[i]);
        }
        if (value->as.object.items) free(value->as.object.items);
    } break;

    case CJSON_VALUE_ARRAY: {
        for (unsigned i = 0; i < value->as.array.count; i++) {
            cjson__free_value(&value->as.array.items[i]);
        }
        if (value->as.array.items) free(value->as.array.items);
    } break;

    case CJSON_VALUE_STRING: {
        if (value->as.string) free(value->as.string);
    } break;

    case CJSON_VALUE_NUMBER:
    case CJSON_VALUE_BOOLEAN:
    case CJSON_VALUE_NULL:
        break;

    default:
        fprintf(stderr, "ERROR: unknown type\n");
        exit(1);
    }
}

static void cjson__free_pair(cjson_pair_t *pair)
{
    if (pair->key) free(pair->key);
    cjson__free_value(&pair->value);
}

static bool cjson__is_in_array(cjson_t *cj)
{
    if (cj->code != CJSON_OK) return false;

    cjson_pair_t *scope = cjson__current_scope(cj);
    // No scope means it is in root object, which has no key,
    // similar to 'in array'
    return !scope || scope->value.type == CJSON_VALUE_ARRAY;
}

static cjson_pair_t *cjson__current_scope(cjson_t *cj)
{
    if (cj->code != CJSON_OK) return NULL;

    if (cj->scopes.count == 0) return NULL;
    return &cj->scopes.items[cj->scopes.count - 1];
}

static cjson_pair_t cjson__pop_scope(cjson_t *cj)
{
    if (cj->code != CJSON_OK) return (cjson_pair_t){0};

    if (cj->scopes.count == 0) cjson__set_error(cj, CJSON_SCOPE_UNDERFLOW);
    cjson_pair_t res = cj->scopes.items[--cj->scopes.count];

    if (cj->scopes.count > 0) {
        cjson_pair_t *scope = cjson__current_scope(cj);
        if (scope->value.type == CJSON_VALUE_ARRAY) {
            cj->scope_type = CJSON_SCOPE_ARRAY;
        } else {
            cj->scope_type = CJSON_SCOPE_OBJECT;
        }
    }

    return res;
}

static void cjson__push_scope(cjson_t *cj, cjson_pair_t scope)
{
    cjson_dyna_append(&cj->scopes, scope);
    if (scope.value.type == CJSON_VALUE_ARRAY) {
        cj->scope_type = CJSON_SCOPE_ARRAY;
    } else {
        cj->scope_type = CJSON_SCOPE_OBJECT;
    }
}

static cjson_pair_t cjson__pair(char *key, cjson_value_t value)
{
    return (cjson_pair_t){
        .key = key,
        .value = value,
    };
}

/////////////////////////////////////////////
////// public function
/////////////////////////////////////////////

bool cjson_parse(cjson_t *cj, const char *input, unsigned size)
{
    assert(cj != NULL && input != NULL);
    if (size == 0) return false;

    stb_lexer lex = {0};
    static char string_store[4096];
    
    stb_c_lexer_init(&lex, input, input + size, string_store, sizeof(string_store));

    long token = peek(&lex);
    if (token == '{') {
        return parse_object(cj, &lex);
    } else if (token == '[') {
        return parse_array(cj, &lex);
    } else {
        return false;
    }
}

void cjson_init(cjson_t *cj, FILE *fp, const char *indent)
{
    assert(cj != NULL && fp != NULL && indent != NULL);

    cj->output = fp;
    cj->indent = strdup(indent);
    cj->scopes.count = 0;
    cj->scopes.capacity = 0;
    cj->scopes.items = NULL;
    cj->scope_type = CJSON_SCOPE_NULL;
    cj->root = NULL;
    cj->code = CJSON_OK;
    cj->error_key = NULL;
}

void cjson_dump(cjson_t *cj)
{
    assert(cj != NULL);
    if (cj->code != CJSON_OK) return;

    cjson__dump_value(cj, 0, cj->root, true);
}

void cjson_fini(cjson_t *cj)
{
    assert(cj != NULL);
    if (cj->code != CJSON_OK) return;

    cjson__free_value(cj->root);
    if (cj->error_key) free(cj->error_key);
    if (cj->indent) free(cj->indent);
    if (cj->scopes.items) free(cj->scopes.items);

    memset(cj, 0, sizeof(cjson_t));
}

cjson_value_t *cjson_query(cjson_value_t *root, const char *key)
{
    assert(root != NULL && key != NULL);

    assert(root->type == CJSON_VALUE_OBJECT);
    for (unsigned i = 0; i < root->as.object.count; i++) {
        cjson_pair_t *pair = &root->as.object.items[i];
        if (!pair->key) continue;
        if (strcmp(pair->key, key) == 0) return &pair->value;
    }

    return NULL;
}

void cjson_append_element(cjson_t *cj, char *key, cjson_value_t value)
{
    cjson_pair_t *scope = cjson__current_scope(cj);
    if (cj->scope_type == CJSON_SCOPE_OBJECT) {
        cjson_pair_t pair = cjson__pair(key, value);
        cjson_dyna_append(&scope->value.as.object, pair);
    } else if (cj->scope_type == CJSON_SCOPE_ARRAY) {
        cjson_dyna_append(&scope->value.as.array, value);
    }
}

void cjson_key(cjson_t *cj, const char *key)
{
    assert(cj != NULL && key != NULL);
    if (cj->code != CJSON_OK) return;

    if (!key) {
        cj->error_key = strdup(key);
        cjson__set_error(cj, CJSON_INVALID_KEY);
        return;
    }

    if (strlen(key) >= MAX_KEY_LEN) {
        cj->error_key = strdup(key);
        cjson__set_error(cj, CJSON_KEY_OVERFLOW);
        return;
    }

    cjson_pair_t *scope = cjson__current_scope(cj); 
    if (scope && scope->value.type == CJSON_VALUE_OBJECT) {
        // Key has been already exist
        if (cjson_query(&scope->value, key)) {
            cj->error_key = strdup(key);
            cjson__set_error(cj, CJSON_DOUBLE_KEY);
            return;
        }
    }

    strncpy(cj->key, key, sizeof(cj->key));
    cj->key[MAX_KEY_LEN - 1] = '\0';
}

void cjson_string(cjson_t *cj, const char *value)
{
    assert(cj != NULL);
    if (cj->code != CJSON_OK) return;

    cjson_value_t pair_value = {
        .type = CJSON_VALUE_STRING,
        .as.string = value ? strdup(value) : NULL,
    };
    char *pair_key = cjson__is_in_array(cj) ? NULL : strdup(cj->key);
    cjson_append_element(cj, pair_key, pair_value);
}

void cjson_number(cjson_t *cj, float value)
{
    assert(cj != NULL);
    if (cj->code != CJSON_OK) return;

    cjson_value_t pair_value = {
        .type = CJSON_VALUE_NUMBER,
        .as.number = value,
    };
    char *pair_key = cjson__is_in_array(cj) ? NULL : strdup(cj->key);
    cjson_append_element(cj, pair_key, pair_value);
}

void cjson_boolean(cjson_t *cj, bool value)
{
    assert(cj != NULL);
    if (cj->code != CJSON_OK) return;

    cjson_value_t pair_value = {
        .type = CJSON_VALUE_BOOLEAN,
        .as.boolean = value,
    };
    char *pair_key = cjson__is_in_array(cj) ? NULL : strdup(cj->key);
    cjson_append_element(cj, pair_key, pair_value);
}

void cjson_null(cjson_t *cj)
{
    assert(cj != NULL);
    if (cj->code != CJSON_OK) return;

    cjson_value_t pair_value = {
        .type = CJSON_VALUE_NULL,
    };
    char *pair_key = cjson__is_in_array(cj) ? NULL : strdup(cj->key);
    cjson_append_element(cj, pair_key, pair_value);
}

void cjson_array_begin(cjson_t *cj)
{
    assert(cj != NULL);
    if (cj->code != CJSON_OK) return;

    cjson_value_t pair_value = {
        .type = CJSON_VALUE_ARRAY,
        .as.array.count = 0,
        .as.array.capacity = 0,
        .as.array.items = NULL,
    };
    char *pair_key = cjson__is_in_array(cj) ? NULL : strdup(cj->key);
    cjson__push_scope(cj, cjson__pair(pair_key, pair_value));
    if (!cj->root) cj->root = &cj->scopes.items[0].value;
}

void cjson_array_end(cjson_t *cj)
{
    assert(cj != NULL);
    if (cj->code != CJSON_OK) return;

    if (cj->scopes.count == 1) return;

    cjson_pair_t array = cjson__pop_scope(cj);
    cjson_append_element(cj, array.key, array.value);
}

void cjson_object_begin(cjson_t *cj)
{
    assert(cj != NULL);
    if (cj->code != CJSON_OK) return;

    cjson_value_t pair_value = {
        .type = CJSON_VALUE_OBJECT,
        .as.object.count = 0,
        .as.object.capacity = 0,
        .as.object.items = NULL,
    };
    char *pair_key = cjson__is_in_array(cj) ? NULL : strdup(cj->key);
    cjson__push_scope(cj, cjson__pair(pair_key, pair_value));
    if (!cj->root) cj->root = &cj->scopes.items[0].value;
}

void cjson_object_end(cjson_t *cj)
{
    assert(cj != NULL);
    if (cj->code != CJSON_OK) return;

    if (cj->scopes.count == 1) return;

    cjson_pair_t object = cjson__pop_scope(cj);
    cjson_append_element(cj, object.key, object.value);
}

#undef cjson_dyna_append

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
