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

enum CJson_ValueType {
    CJSON_VALUE_NULL = 0,
    CJSON_VALUE_OBJECT,
    CJSON_VALUE_ARRAY,
    CJSON_VALUE_STRING,
    CJSON_VALUE_NUMBER,
    CJSON_VALUE_BOOLEAN,
};

enum CJson_ScopeType {
    CJSON_SCOPE_NULL = 0,
    CJSON_SCOPE_OBJECT,
    CJSON_SCOPE_ARRAY,
};

enum CJson_ErrCode {
    CJSON_OK = 0,
    CJSON_DOUBLE_KEY,
    CJSON_INVALID_KEY,
    CJSON_KEY_OVERFLOW,
    CJSON_SCOPE_OVERFLOW,
    CJSON_SCOPE_UNDERFLOW,
};

/////////////////////////////////////////////
////// structure
/////////////////////////////////////////////

struct CJson_Pair;
struct CJson_Value;

struct CJson_Value {
    enum CJson_ValueType type;
    union {
        char *string;
        float number;
        bool boolean;
        struct {
            unsigned count;
            unsigned capacity;
            struct CJson_Pair *items; 
        } object;
        struct {
            unsigned count;
            unsigned capacity;
            struct CJson_Value *items;
        } array;
    } as;
};

struct CJson_Pair {
    char *key;
    struct CJson_Value value;
};

struct CJson_Context {
    FILE *output;
    char *indent;
    struct {
        unsigned count;
        unsigned capacity;
        struct CJson_Pair *items;
    } scopes;
    enum CJson_ScopeType scope_type;
    char key[MAX_KEY_LEN];      // Record the current member key
    struct CJson_Value *root;   // Record the first scope
    enum CJson_ErrCode code;    // Error code
    char *error_key;            // Store the key when error occurred
};

/////////////////////////////////////////////
////// typedef
/////////////////////////////////////////////

typedef enum CJson_ValueType CJson_ValueType;
typedef enum CJson_ScopeType CJson_ScopeType;
typedef enum CJson_ErrCode CJson_ErrCode;

typedef struct CJson_Value CJson_Value;
typedef struct CJson_Pair CJson_Pair;
typedef struct CJson_Context CJson_Context;

/////////////////////////////////////////////
////// public function
/////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

void cjson_init(CJson_Context *cj, FILE *fp, const char *indent);
void cjson_dump(CJson_Context *cj);
void cjson_fini(CJson_Context *cj);
CJson_Value *cjson_query(CJson_Value *root, const char *key);
bool cjson_parse(CJson_Context *cj, const char *input, unsigned size);

void cjson_key(CJson_Context *cj, const char *key);
void cjson_string(CJson_Context *cj, const char *value);
void cjson_number(CJson_Context *cj, float value);
void cjson_boolean(CJson_Context *cj, bool value);
void cjson_null(CJson_Context *cj);
void cjson_array_begin(CJson_Context *cj);
void cjson_array_end(CJson_Context *cj);
void cjson_object_begin(CJson_Context *cj);
void cjson_object_end(CJson_Context *cj);

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

static void cjson__dump_pair(CJson_Context *cj, int level, CJson_Pair *pair, bool comma);
static void cjson__dump_value(CJson_Context *cj, int level, CJson_Value *value, bool indent);
static void cjson__dump_indent(CJson_Context *cj, int level);
static void cjson__free_pair(CJson_Pair *pair);
static void cjson__free_value(CJson_Value *value);
static CJson_Pair cjson__pair(char *key, CJson_Value value);
static CJson_Pair *cjson__current_scope(CJson_Context *cj);
static CJson_Pair cjson__pop_scope(CJson_Context *cj);
static void cjson__push_scope(CJson_Context *cj, CJson_Pair scope);
static bool cjson__is_in_array(CJson_Context *cj);
static void cjson__set_error(CJson_Context *cj, CJson_ErrCode code);
static void cjson__append_element(CJson_Context *cj, char *key, CJson_Value value);
static long peek(stb_lexer *lex);
static long advance(stb_lexer *lex);
static bool consume(stb_lexer *lex, long expected, const char *msg);
static bool parse_value(CJson_Context *cj, stb_lexer *lex);
static bool parse_array(CJson_Context *cj, stb_lexer *lex);
static bool parse_object(CJson_Context *cj, stb_lexer *lex);

static void cjson__append_element(CJson_Context *cj, char *key, CJson_Value value)
{
    CJson_Pair *scope = cjson__current_scope(cj);
    if (cj->scope_type == CJSON_SCOPE_OBJECT) {
        CJson_Pair pair = cjson__pair(key, value);
        cjson_dyna_append(&scope->value.as.object, pair);
    } else if (cj->scope_type == CJSON_SCOPE_ARRAY) {
        cjson_dyna_append(&scope->value.as.array, value);
    }
}

static bool parse_array(CJson_Context *cj, stb_lexer *lex)
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

static bool parse_object(CJson_Context *cj, stb_lexer *lex)
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

static bool parse_value(CJson_Context *cj, stb_lexer *lex)
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

static void cjson__set_error(CJson_Context *cj, CJson_ErrCode code)
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

static void cjson__dump_indent(CJson_Context *cj, int level)
{
    if (cj->code != CJSON_OK) return;
    for (int i = 0; i < level; i++) {
        fwrite(cj->indent, strlen(cj->indent), 1, cj->output);
    }
}

static void cjson__dump_value(CJson_Context *cj, int level, CJson_Value *value, bool indent)
{
    if (cj->code != CJSON_OK) return;

    if (indent) cjson__dump_indent(cj, level);

    switch (value->type) {
    case CJSON_VALUE_OBJECT: {
        fwrite("{\n", 2, 1, cj->output);
        for (unsigned i = 0; i < value->as.object.count; i++) {
            CJson_Pair *pair = &value->as.object.items[i];
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

static void cjson__dump_pair(CJson_Context *cj, int level, CJson_Pair *pair, bool comma)
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
        fwrite("\n", 1, 1, cj->output);
    }
}

static void cjson__free_value(CJson_Value *value)
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

static void cjson__free_pair(CJson_Pair *pair)
{
    if (pair->key) free(pair->key);
    cjson__free_value(&pair->value);
}

static bool cjson__is_in_array(CJson_Context *cj)
{
    if (cj->code != CJSON_OK) return false;

    CJson_Pair *scope = cjson__current_scope(cj);
    // No scope means it is in root object, which has no key,
    // similar to 'in array'
    return !scope || scope->value.type == CJSON_VALUE_ARRAY;
}

static CJson_Pair *cjson__current_scope(CJson_Context *cj)
{
    if (cj->code != CJSON_OK) return NULL;

    if (cj->scopes.count == 0) return NULL;
    return &cj->scopes.items[cj->scopes.count - 1];
}

static CJson_Pair cjson__pop_scope(CJson_Context *cj)
{
    if (cj->code != CJSON_OK) return (CJson_Pair){0};

    if (cj->scopes.count == 0) cjson__set_error(cj, CJSON_SCOPE_UNDERFLOW);
    CJson_Pair res = cj->scopes.items[--cj->scopes.count];

    if (cj->scopes.count > 0) {
        CJson_Pair *scope = cjson__current_scope(cj);
        if (scope->value.type == CJSON_VALUE_ARRAY) {
            cj->scope_type = CJSON_SCOPE_ARRAY;
        } else {
            cj->scope_type = CJSON_SCOPE_OBJECT;
        }
    }

    return res;
}

static void cjson__push_scope(CJson_Context *cj, CJson_Pair scope)
{
    cjson_dyna_append(&cj->scopes, scope);
    if (scope.value.type == CJSON_VALUE_ARRAY) {
        cj->scope_type = CJSON_SCOPE_ARRAY;
    } else {
        cj->scope_type = CJSON_SCOPE_OBJECT;
    }
}

static CJson_Pair cjson__pair(char *key, CJson_Value value)
{
    return (CJson_Pair){
        .key = key,
        .value = value,
    };
}

/////////////////////////////////////////////
////// public function
/////////////////////////////////////////////

bool cjson_parse(CJson_Context *cj, const char *input, unsigned size)
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

void cjson_init(CJson_Context *cj, FILE *fp, const char *indent)
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

void cjson_dump(CJson_Context *cj)
{
    assert(cj != NULL);
    if (cj->code != CJSON_OK) return;

    cjson__dump_value(cj, 0, cj->root, true);
}

void cjson_fini(CJson_Context *cj)
{
    assert(cj != NULL);
    if (cj->code != CJSON_OK) return;

    cjson__free_value(cj->root);
    if (cj->error_key) free(cj->error_key);
    if (cj->indent) free(cj->indent);
    if (cj->scopes.items) free(cj->scopes.items);

    memset(cj, 0, sizeof(CJson_Context));
}

CJson_Value *cjson_query(CJson_Value *root, const char *key)
{
    assert(root != NULL && key != NULL);

    assert(root->type == CJSON_VALUE_OBJECT);
    for (unsigned i = 0; i < root->as.object.count; i++) {
        CJson_Pair *pair = &root->as.object.items[i];
        if (!pair->key) continue;
        if (strcmp(pair->key, key) == 0) return &pair->value;
    }

    return NULL;
}

void cjson_key(CJson_Context *cj, const char *key)
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

    CJson_Pair *scope = cjson__current_scope(cj); 
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

void cjson_string(CJson_Context *cj, const char *value)
{
    assert(cj != NULL);
    if (cj->code != CJSON_OK) return;

    CJson_Value pair_value = {
        .type = CJSON_VALUE_STRING,
        .as.string = value ? strdup(value) : NULL,
    };
    char *pair_key = cjson__is_in_array(cj) ? NULL : strdup(cj->key);
    cjson__append_element(cj, pair_key, pair_value);
}

void cjson_number(CJson_Context *cj, float value)
{
    assert(cj != NULL);
    if (cj->code != CJSON_OK) return;

    CJson_Value pair_value = {
        .type = CJSON_VALUE_NUMBER,
        .as.number = value,
    };
    char *pair_key = cjson__is_in_array(cj) ? NULL : strdup(cj->key);
    cjson__append_element(cj, pair_key, pair_value);
}

void cjson_boolean(CJson_Context *cj, bool value)
{
    assert(cj != NULL);
    if (cj->code != CJSON_OK) return;

    CJson_Value pair_value = {
        .type = CJSON_VALUE_BOOLEAN,
        .as.boolean = value,
    };
    char *pair_key = cjson__is_in_array(cj) ? NULL : strdup(cj->key);
    cjson__append_element(cj, pair_key, pair_value);
}

void cjson_null(CJson_Context *cj)
{
    assert(cj != NULL);
    if (cj->code != CJSON_OK) return;

    CJson_Value pair_value = {
        .type = CJSON_VALUE_NULL,
    };
    char *pair_key = cjson__is_in_array(cj) ? NULL : strdup(cj->key);
    cjson__append_element(cj, pair_key, pair_value);
}

void cjson_array_begin(CJson_Context *cj)
{
    assert(cj != NULL);
    if (cj->code != CJSON_OK) return;

    CJson_Value pair_value = {
        .type = CJSON_VALUE_ARRAY,
        .as.array.count = 0,
        .as.array.capacity = 0,
        .as.array.items = NULL,
    };
    char *pair_key = cjson__is_in_array(cj) ? NULL : strdup(cj->key);
    cjson__push_scope(cj, cjson__pair(pair_key, pair_value));
    if (!cj->root) cj->root = &cj->scopes.items[0].value;
}

void cjson_array_end(CJson_Context *cj)
{
    assert(cj != NULL);
    if (cj->code != CJSON_OK) return;

    if (cj->scopes.count == 1) return;

    CJson_Pair array = cjson__pop_scope(cj);
    cjson__append_element(cj, array.key, array.value);
}

void cjson_object_begin(CJson_Context *cj)
{
    assert(cj != NULL);
    if (cj->code != CJSON_OK) return;

    CJson_Value pair_value = {
        .type = CJSON_VALUE_OBJECT,
        .as.object.count = 0,
        .as.object.capacity = 0,
        .as.object.items = NULL,
    };
    char *pair_key = cjson__is_in_array(cj) ? NULL : strdup(cj->key);
    cjson__push_scope(cj, cjson__pair(pair_key, pair_value));
    if (!cj->root) cj->root = &cj->scopes.items[0].value;
}

void cjson_object_end(CJson_Context *cj)
{
    assert(cj != NULL);
    if (cj->code != CJSON_OK) return;

    if (cj->scopes.count == 1) return;

    CJson_Pair object = cjson__pop_scope(cj);
    cjson__append_element(cj, object.key, object.value);
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
