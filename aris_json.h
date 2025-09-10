/*
aris_json.h - v0.01 - Dylaris 2025
===================================================

BRIEF:
  A C library for serializing and deserializing json.

NOTICE:
  This implementation directly supports serialization,
  but for deserialization, 'stb_c_lexer.h' needs to be used.
  And it is not compatiable with C++.

USAGE:
  In exactly one source file, define the implementation macro
  before including this header:
  ```
    #define ARIS_JSON_IMPLEMENTATION
    #define ARIS_JSON_ENABLE_DESERIALIZATION (optional)
    #include "aris_json.h"
  ```
  In other files, just include the header without the macro.

LICENSE:
  See the end of this file for further details.
*/

#ifndef ARIS_JSON_H
#define ARIS_JSON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef enum aris_json_value_type {
    ARIS_JSON_VALUE_NULL = 0,
    ARIS_JSON_VALUE_OBJECT,
    ARIS_JSON_VALUE_ARRAY,
    ARIS_JSON_VALUE_STRING,
    ARIS_JSON_VALUE_NUMBER,
    ARIS_JSON_VALUE_BOOLEAN,
} aris_json_value_type;

typedef enum aris_json_scope_type {
    ARIS_JSON_SCOPE_NULL = 0,
    ARIS_JSON_SCOPE_OBJECT,
    ARIS_JSON_SCOPE_ARRAY,
} aris_json_scope_type;

typedef enum aris_json_error_code {
    ARIS_JSON_OK = 0,
    ARIS_JSON_DOUBLE_KEY,
    ARIS_JSON_NULL_KEY,
    ARIS_JSON_KEY_OVERFLOW,
    ARIS_JSON_INCORRECT_SCOPE,
    ARIS_JSON_NO_SCOPE,
} aris_json_error_code;

typedef struct aris_json_value aris_json_value;
typedef struct aris_json_pair aris_json_pair;

struct aris_json_value {
    aris_json_value_type type;
    union {
        char *string;
        double number;
        bool boolean;
        aris_json_pair *object; /* array of json_pair */
        aris_json_value *array; /* array of json_value */
    } as;
};

struct aris_json_pair {
    char *key;
    aris_json_value value;
};

typedef enum aris_output_mode {
    ARIS_JSON_FILE_OUTPUT = 1,
    ARIS_JSON_BUFFER_OUTPUT
} aris_output_mode;

typedef struct aris_json_opt {
    const char *indent;
    aris_output_mode mode;
    void (*write_to_buffer)(const char*, char*, size_t);
    char *output_buffer;
    size_t output_buffer_size;
    void (*write_to_file)(const char*, FILE*);
    FILE *output_file;
} aris_json_opt;

typedef struct aris_json_context {
    aris_json_scope_type scope_type; /* current scope type */
    aris_json_value *scopes;         /* array of json_value (object or array) */
    char *error_buffer;              /* store the latest error string */
    char *current_key;               /* store the current member key */
    aris_json_value *root;           /* root object */
    aris_json_error_code code;
    aris_json_opt opt;
} aris_json_context;

#define aris_json_init(ctx, ...) aris_json_init_opt(ctx, (aris_json_opt){__VA_ARGS__})
void aris_json_init_opt(aris_json_context *ctx, aris_json_opt opt);
void aris_json_fini(aris_json_context *ctx);
void aris_json_dump(aris_json_context *ctx);
void aris_json_default_write_to_buffer(const char *s, char *buffer, size_t size);
void aris_json_default_write_to_file(const char *s, FILE *file);
void aris_json_print_value(const aris_json_value *value);

/* serialization */
bool aris_json_key(aris_json_context *ctx, const char *key);
bool aris_json_string(aris_json_context *ctx, const char *value);
bool aris_json_number(aris_json_context *ctx, double value);
bool aris_json_boolean(aris_json_context *ctx, bool value);
bool aris_json_null(aris_json_context *ctx);
bool aris_json_object_begin(aris_json_context *ctx);
bool aris_json_object_end(aris_json_context *ctx);
bool aris_json_array_begin(aris_json_context *ctx);
bool aris_json_array_end(aris_json_context *ctx);

#ifdef ARIS_JSON_ENABLE_DESERIALIZATION
/* deserialization */
bool aris_json_parse(aris_json_context *ctx, const char *input, size_t size);
#endif /* ARIS_JSON_ENABLE_DESERIALIZATION */

/* query */
const aris_json_value *aris_json_object_get_value(const aris_json_value *root, const char *key);
const aris_json_pair *aris_json_object_get_pair(const aris_json_value *root, size_t idx);
size_t aris_json_object_get_size(const aris_json_value *root);
const aris_json_value *aris_json_array_get_value(const aris_json_value *root, size_t idx);
size_t aris_json_array_get_size(const aris_json_value *root);
#define aris_json_context_get_root(context) ((context)->root)
#define aris_json_is_number(value)  ((value)->type == ARIS_JSON_VALUE_NUMBER)
#define aris_json_is_string(value)  ((value)->type == ARIS_JSON_VALUE_STRING)
#define aris_json_is_boolean(value) ((value)->type == ARIS_JSON_VALUE_BOOLEAN)
#define aris_json_is_object(value)  ((value)->type == ARIS_JSON_VALUE_OBJECT)
#define aris_json_is_array(value)   ((value)->type == ARIS_JSON_VALUE_ARRAY)
#define aris_json_to_number(value)  ((value)->as.number)
#define aris_json_to_string(value)  ((value)->as.string)
#define aris_json_to_boolean(value) ((value)->as.boolean)

#endif /* ARIS_JSON_H */

#ifdef ARIS_JSON_IMPLEMENTATION

typedef struct aris_vec_tor_header {
    size_t size;
    size_t capacity;
} aris_vec_tor_header ;

#define aris_vec__header(vec) \
    ((aris_vec_tor_header*)((char*)(vec) - sizeof(aris_vec_tor_header)))
#define aris_vec__size(vec) ((vec) ? aris_vec__header(vec)->size : 0)
#define aris_vec__capacity(vec)  ((vec) ? aris_vec__header(vec)->capacity : 0)
#define aris_vec__push(vec, item)                                              \
    do {                                                                       \
        if (aris_vec__size(vec) + 1 > aris_vec__capacity(vec)) {               \
            size_t new_capacity, alloc_size;                                   \
            aris_vec_tor_header *new_header;                                   \
                                                                               \
            new_capacity = aris_vec__capacity(vec) == 0                        \
                           ? 16 : 2 * aris_vec__capacity(vec);                 \
            alloc_size = sizeof(aris_vec_tor_header) +                         \
                         new_capacity*sizeof(*(vec));                          \
                                                                               \
            if (vec) {                                                         \
                new_header = realloc(aris_vec__header(vec), alloc_size);       \
            } else {                                                           \
                new_header = malloc(alloc_size);                               \
                new_header->size = 0;                                          \
            }                                                                  \
            new_header->capacity = new_capacity;                               \
                                                                               \
            (vec) = (void*)((char*)new_header + sizeof(aris_vec_tor_header));  \
        }                                                                      \
                                                                               \
        (vec)[aris_vec__header(vec)->size++] = (item);                         \
    } while (0)
#define aris_vec__pop(vec) ((vec)[--aris_vec__header(vec)->size])
#define aris_vec__free(vec)                   \
    do {                                      \
        if (vec) free(aris_vec__header(vec)); \
        (vec) = NULL;                         \
    } while (0)
#define aris_vec__reset(vec) ((vec) ? aris_vec__header(vec)->size = 0 : 0)

#ifdef ARIS_JSON_ENABLE_DESERIALIZATION
#define STB_C_LEXER_IMPLEMENTATION
#include "stb_c_lexer.h"
#endif /* ARIS_JSON_ENABLE_DESERIALIZATION */

#define ARIS_JSON__ERROR_BUFFER_SIZE 1024
#define ARIS_JSON__KEY_MAX_SIZE      256

static void aris_json__write(aris_json_context *ctx, const char *s);
static void aris_json__set_error(aris_json_context *ctx, const char *key, aris_json_error_code code);
static aris_json_value *aris_json__get_current_scope(aris_json_context *ctx);
static void aris_json__append_element(aris_json_context *ctx, char *key, aris_json_value value);
static void aris_json__free_value(aris_json_value *value);
static void aris_json__free_pair(aris_json_pair *pair);
static void aris_json__push_scope(aris_json_context *ctx, aris_json_value scope);
static aris_json_value aris_json__pop_scope(aris_json_context *ctx);
static void aris_json__dump_pair(aris_json_context *ctx, size_t level, aris_json_pair *pair, bool comma);
static void aris_json__dump_value(aris_json_context *ctx, size_t level, aris_json_value *value, bool indent);
static void aris_json__dump_indent(aris_json_context *ctx, size_t level);
static bool aris_json_scope_begin(aris_json_context *ctx, aris_json_value scope);
static bool aris_json_scope_end(aris_json_context *ctx);
#ifdef ARIS_JSON_ENABLE_DESERIALIZATION
static long aris_json__peek(stb_lexer *lex);
static long aris_json__advance(stb_lexer *lex);
static bool aris_json__consume(stb_lexer *lex, long expected, const char *msg);
static bool aris_json__parse_value(aris_json_context *ctx, stb_lexer *lex);
static bool aris_json__parse_array(aris_json_context *ctx, stb_lexer *lex);
static bool aris_json__parse_object(aris_json_context *ctx, stb_lexer *lex);
#endif /* ARIS_JSON_ENABLE_DESERIALIZATION */

void aris_json_init_opt(aris_json_context *ctx, aris_json_opt opt)
{
    ctx->scopes = NULL;
    ctx->scope_type = ARIS_JSON_SCOPE_NULL;
    ctx->code = ARIS_JSON_NO_SCOPE;
    ctx->root = NULL;
    ctx->error_buffer= malloc(ARIS_JSON__ERROR_BUFFER_SIZE + 1);
    ctx->current_key = malloc(ARIS_JSON__KEY_MAX_SIZE + 1);
    if (!ctx->error_buffer || !ctx->current_key) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    aris_json__set_error(ctx, NULL, ARIS_JSON_NO_SCOPE);

    /* use the default configuration if it is not specified */
    if (!opt.indent)          opt.indent = "\t";
    if (!opt.mode)            opt.mode = ARIS_JSON_FILE_OUTPUT;
    if (!opt.write_to_buffer) opt.write_to_buffer = aris_json_default_write_to_buffer;
    if (!opt.write_to_file)   opt.write_to_file = aris_json_default_write_to_file;
    if (!opt.output_file)     opt.output_file = stdout;
    ctx->opt = opt;
}

void aris_json_fini(aris_json_context *ctx)
{
    /* ctx->root has the reference of ctx->scopes[0], so free the
       attached pairs first throuth root. */
    aris_json__free_value(ctx->root);
    ctx->root = NULL;
    aris_vec__free(ctx->scopes);
    ctx->scope_type = ARIS_JSON_SCOPE_NULL;
    ctx->code = ARIS_JSON_NO_SCOPE;
    if (ctx->error_buffer) free(ctx->error_buffer);
    if (ctx->current_key) free(ctx->current_key);
    ctx->error_buffer = NULL;
    ctx->current_key = NULL;
}

void aris_json_dump(aris_json_context *ctx)
{
    if (ctx->code != ARIS_JSON_OK) return;
    aris_json__dump_value(ctx, 0, ctx->root, true);
}

void aris_json_print_value(const aris_json_value *value)
{
    switch (value->type) {
    case ARIS_JSON_VALUE_NULL:
        printf("type: null, value: null\n");
        break;
    case ARIS_JSON_VALUE_OBJECT:
        printf("type: object, value: {...}\n");
        break;
    case ARIS_JSON_VALUE_ARRAY:
        printf("type: array, value: [...]\n");
        break;
    case ARIS_JSON_VALUE_STRING:
        printf("type: string, value: '%s'\n", value->as.string);
        break;
    case ARIS_JSON_VALUE_NUMBER:
        printf("type: number, value: '%.15g'\n", value->as.number);
        break;
    case ARIS_JSON_VALUE_BOOLEAN:
        printf("type: boolean, value: '%s'\n", value->as.boolean
                                               ? "true" : "false");
        break;
    default:
        break;
    }
}

void aris_json_default_write_to_buffer(const char *s, char *buffer, size_t size)
{
    static size_t pos = 0;
    size_t len = strlen(s);
    if (pos + len < size) {
        strncpy(buffer+pos, s, len);
        pos += len;
        buffer[pos] = '\0';
    }
}

void aris_json_default_write_to_file(const char *s, FILE *file)
{
    fwrite(s, strlen(s), 1, file);
}

bool aris_json_key(aris_json_context *ctx, const char *key)
{
    if (ctx->code != ARIS_JSON_OK) return false;

    if (!key) {
        aris_json__set_error(ctx, NULL, ARIS_JSON_NULL_KEY);
        return false;
    }
    if (strlen(key) > ARIS_JSON__KEY_MAX_SIZE) {
        aris_json__set_error(ctx, key, ARIS_JSON_KEY_OVERFLOW);
        return false;
    }

    if (ctx->scope_type == ARIS_JSON_SCOPE_NULL) {
        aris_json__set_error(ctx, key, ARIS_JSON_NO_SCOPE);
        return false;
    } else if (ctx->scope_type == ARIS_JSON_SCOPE_OBJECT) {
        /* check if the key already exists */
        aris_json_value *scope = aris_json__get_current_scope(ctx);
        if (aris_json_object_get_value(scope, key)) {
            aris_json__set_error(ctx, key, ARIS_JSON_DOUBLE_KEY);
            return false;
        } else {
            snprintf(ctx->current_key, ARIS_JSON__KEY_MAX_SIZE, "%s", key);
            return true;
        }
    } else {
        aris_json__set_error(ctx, NULL, ARIS_JSON_INCORRECT_SCOPE);
        return false;
    }
}

bool aris_json_string(aris_json_context *ctx, const char *value)
{
    if (ctx->code != ARIS_JSON_OK) return false;

    aris_json_value pair_value = {
        .type = ARIS_JSON_VALUE_STRING,
        .as.string = value ? strdup(value) : NULL
    };
    char *pair_key = ctx->scope_type == ARIS_JSON_SCOPE_ARRAY
                     ? NULL : strdup(ctx->current_key);
    aris_json__append_element(ctx, pair_key, pair_value);

    return true;
}

bool aris_json_number(aris_json_context *ctx, double value)
{
    if (ctx->code != ARIS_JSON_OK) return false;

    aris_json_value pair_value = {
        .type = ARIS_JSON_VALUE_NUMBER,
        .as.number = value
    };
    char *pair_key = ctx->scope_type == ARIS_JSON_SCOPE_ARRAY
                     ? NULL : strdup(ctx->current_key);
    aris_json__append_element(ctx, pair_key, pair_value);

    return true;
}

bool aris_json_boolean(aris_json_context *ctx, bool value)
{
    if (ctx->code != ARIS_JSON_OK) return false;

    aris_json_value pair_value = {
        .type = ARIS_JSON_VALUE_BOOLEAN,
        .as.boolean = value
    };
    char *pair_key = ctx->scope_type == ARIS_JSON_SCOPE_ARRAY
                     ? NULL : strdup(ctx->current_key);
    aris_json__append_element(ctx, pair_key, pair_value);

    return true;
}

bool aris_json_null(aris_json_context *ctx)
{
    if (ctx->code != ARIS_JSON_OK) return false;

    aris_json_value pair_value = {
        .type = ARIS_JSON_VALUE_NULL
    };
    char *pair_key = ctx->scope_type == ARIS_JSON_SCOPE_ARRAY
                     ? NULL : strdup(ctx->current_key);
    aris_json__append_element(ctx, pair_key, pair_value);

    return true;
}

bool aris_json_object_begin(aris_json_context *ctx)
{
    if (ctx->code != ARIS_JSON_OK && ctx->code != ARIS_JSON_NO_SCOPE) {
        return false;
    }
    aris_json_value scope = {
        .type = ARIS_JSON_VALUE_OBJECT,
        .as.object = NULL,
    };
    return aris_json_scope_begin(ctx, scope);
}

bool aris_json_object_end(aris_json_context *ctx)
{
    if (ctx->code != ARIS_JSON_OK) return false;
    return aris_json_scope_end(ctx);
}

bool aris_json_array_begin(aris_json_context *ctx)
{
    if (ctx->code != ARIS_JSON_OK && ctx->code != ARIS_JSON_NO_SCOPE) {
        return false;
    }
    aris_json_value scope = {
        .type = ARIS_JSON_VALUE_ARRAY,
        .as.array = NULL,
    };
    return aris_json_scope_begin(ctx, scope);
}

bool aris_json_array_end(aris_json_context *ctx)
{
    if (ctx->code != ARIS_JSON_OK) return false;
    return aris_json_scope_end(ctx);
}

#ifdef ARIS_JSON_ENABLE_DESERIALIZATION
bool aris_json_parse(aris_json_context *ctx, const char *input, size_t size)
{
    if (size == 0) return false;

    stb_lexer lex = {0};
    long token;
    static char string_store[4096];

    stb_c_lexer_init(&lex, input, input + size,
                     string_store, sizeof(string_store));

    token = aris_json__peek(&lex);
    if (token == '{') {
        return aris_json__parse_object(ctx, &lex);
    } else if (token == '[') {
        return aris_json__parse_array(ctx, &lex);
    } else {
        return false;
    }
}
#endif /* ARIS_JSON_ENABLE_DESERIALIZATION */

const aris_json_value *aris_json_object_get_value(const aris_json_value *root, const char *key)
{
    if (!key || !aris_json_is_object(root)) return NULL;

    for (size_t i = 0; i < aris_vec__size(root->as.object); i++) {
        aris_json_pair *pair = &root->as.object[i];
        if (pair->key && strcmp(pair->key, key) == 0) return &pair->value;
    }

    return NULL;
}

const aris_json_pair *aris_json_object_get_pair(const aris_json_value *root, size_t idx)
{
    if (!aris_json_is_object(root) || idx >= aris_vec__size(root->as.object)) return NULL;
    return &root->as.object[idx];
}

size_t aris_json_object_get_size(const aris_json_value *root)
{
    return aris_json_is_object(root) ? aris_vec__size(root->as.object) : 0;
}

const aris_json_value *aris_json_array_get_value(const aris_json_value *root, size_t idx)
{
    if (!aris_json_is_array(root) || idx >= aris_vec__size(root->as.array)) return NULL;
    return &root->as.array[idx];
}

size_t aris_json_array_get_size(const aris_json_value *root)
{
    return aris_json_is_array(root) ? aris_vec__size(root->as.array) : 0;
}

static void aris_json__write(aris_json_context *ctx, const char *s)
{
    if (ctx->opt.mode == ARIS_JSON_BUFFER_OUTPUT) {
        ctx->opt.write_to_buffer(s, ctx->opt.output_buffer,
                                 ctx->opt.output_buffer_size);
    } else {
        ctx->opt.write_to_file(s, ctx->opt.output_file);
    }
}

static void aris_json__set_error(aris_json_context *ctx, const char *key, aris_json_error_code code)
{
    ctx->code = code;

    switch (code) {
    case ARIS_JSON_OK:
        break;

    case ARIS_JSON_DOUBLE_KEY:
        snprintf(ctx->error_buffer, ARIS_JSON__ERROR_BUFFER_SIZE+1,
                 "ERROR: double key '%s'!\n", key);
        break;

    case ARIS_JSON_NULL_KEY:
        snprintf(ctx->error_buffer, ARIS_JSON__ERROR_BUFFER_SIZE+1,
                 "ERROR: null key!\n");
        break;

    case ARIS_JSON_KEY_OVERFLOW:
        snprintf(ctx->error_buffer, ARIS_JSON__ERROR_BUFFER_SIZE+1,
                 "ERROR: key overflow '%s' (maxsize = %d)!\n",
                 key, ARIS_JSON__KEY_MAX_SIZE);
        break;

    case ARIS_JSON_NO_SCOPE:
        snprintf(ctx->error_buffer, ARIS_JSON__ERROR_BUFFER_SIZE+1,
                 "ERROR: what was done without a scope!\n");
        break;

    case ARIS_JSON_INCORRECT_SCOPE:
        snprintf(ctx->error_buffer, ARIS_JSON__ERROR_BUFFER_SIZE+1,
                 "ERROR: what was done within incorrect scope!\n");
        break;

    default:
        snprintf(ctx->error_buffer, ARIS_JSON__ERROR_BUFFER_SIZE+1,
                 "ERROR: unknown code!\n");
        break;
    }
}

static aris_json_value *aris_json__get_current_scope(aris_json_context *ctx)
{
    if (ctx->code != ARIS_JSON_OK) return NULL;
    if (aris_vec__size(ctx->scopes) == 0) return NULL;
    return &ctx->scopes[aris_vec__size(ctx->scopes) - 1];
}

static void aris_json__append_element(aris_json_context *ctx, char *key, aris_json_value value)
{
    aris_json_value *scope = aris_json__get_current_scope(ctx);
    if (ctx->scope_type == ARIS_JSON_SCOPE_OBJECT) {
        aris_json_pair pair = {key, value};
        aris_vec__push(scope->as.object, pair);
    } else if (ctx->scope_type == ARIS_JSON_SCOPE_ARRAY) {
        aris_vec__push(scope->as.array, value);
    }
}

static void aris_json__free_value(aris_json_value *value)
{
    switch (value->type) {
    case ARIS_JSON_VALUE_OBJECT:
        for (size_t i = 0; i < aris_vec__size(value->as.object); i++) {
            aris_json__free_pair(&value->as.object[i]);
        }
        aris_vec__free(value->as.object);
        break;

    case ARIS_JSON_VALUE_ARRAY:
        for (size_t i = 0; i < aris_vec__size(value->as.array); i++) {
            aris_json__free_value(&value->as.array[i]);
        }
        aris_vec__free(value->as.array);
        break;

    case ARIS_JSON_VALUE_STRING:
        if (value->as.string) free(value->as.string);
        value->as.string = NULL;
        break;

    case ARIS_JSON_VALUE_NUMBER:
    case ARIS_JSON_VALUE_BOOLEAN:
    case ARIS_JSON_VALUE_NULL:
        break;

    default:
        break;
    }
}

static void aris_json__free_pair(aris_json_pair *pair)
{
    if (pair->key) free(pair->key);
    pair->key = NULL;
    aris_json__free_value(&pair->value);
}

static void aris_json__push_scope(aris_json_context *ctx, aris_json_value scope)
{
    aris_vec__push(ctx->scopes, scope);
    ctx->scope_type = scope.type == ARIS_JSON_VALUE_ARRAY
                      ? ARIS_JSON_SCOPE_ARRAY : ARIS_JSON_SCOPE_OBJECT;
}

static aris_json_value aris_json__pop_scope(aris_json_context *ctx)
{
    aris_json_value res = ctx->scopes[--aris_vec__header(ctx->scopes)->size];

    if (aris_vec__size(ctx->scopes) > 0) {
        aris_json_value *scope = aris_json__get_current_scope(ctx);
        ctx->scope_type = scope->type == ARIS_JSON_VALUE_ARRAY
                          ? ARIS_JSON_SCOPE_ARRAY : ARIS_JSON_SCOPE_OBJECT;
    }

    return res;
}

static void aris_json__dump_pair(aris_json_context *ctx, size_t level, aris_json_pair *pair, bool comma)
{
    aris_json__dump_indent(ctx, level);

    static char key[ARIS_JSON__KEY_MAX_SIZE + 4] = {0};
    snprintf(key, sizeof(key), "\"%s\": ", pair->key);
    aris_json__write(ctx, key);
    aris_json__dump_value(ctx, level, &pair->value, false);

    if (comma) {
        aris_json__write(ctx, ",\n");
    } else {
        aris_json__write(ctx, "\n");
    }
}

static void aris_json__dump_value(aris_json_context *ctx, size_t level, aris_json_value *value, bool indent)
{
    static char buffer[50];

    if (indent) aris_json__dump_indent(ctx, level);

    switch (value->type) {
    case ARIS_JSON_VALUE_OBJECT:
        aris_json__write(ctx, "{\n");
        for (size_t i = 0; i < aris_vec__size(value->as.object); i++) {
            aris_json_pair *pair = &value->as.object[i];
            bool comma = true;
            if (i == aris_vec__size(value->as.object) - 1) comma = false;
            aris_json__dump_pair(ctx, level+1, pair, comma);
        }
        aris_json__dump_indent(ctx, level);
        aris_json__write(ctx, "}");
        break;

    case ARIS_JSON_VALUE_ARRAY:
        aris_json__write(ctx, "[\n");
        for (size_t i = 0; i < aris_vec__size(value->as.array); i++) {
            aris_json__dump_value(ctx, level+1, &value->as.array[i], true);
            if (i == aris_vec__size(value->as.array) - 1) {
                aris_json__write(ctx, "\n");
            } else {
                aris_json__write(ctx, ",\n");
            }
        }
        aris_json__dump_indent(ctx, level);
        aris_json__write(ctx, "]");
        break;

    case ARIS_JSON_VALUE_STRING:
        aris_json__write(ctx, "\"");
        aris_json__write(ctx, value->as.string);
        aris_json__write(ctx, "\"");
        break;

    case ARIS_JSON_VALUE_NUMBER:
        snprintf(buffer, sizeof(buffer), "%.15g", value->as.number);
        aris_json__write(ctx, buffer);
        break;

    case ARIS_JSON_VALUE_BOOLEAN:
        if (value->as.boolean) {
            aris_json__write(ctx, "true");
        } else {
            aris_json__write(ctx, "false");
        }
        break;

    case ARIS_JSON_VALUE_NULL:
        aris_json__write(ctx, "null");
        break;

    default:
        break;
    }
}

static void aris_json__dump_indent(aris_json_context *ctx, size_t level)
{
    for (size_t i = 0; i < level; i++) {
        aris_json__write(ctx, ctx->opt.indent);
    }
}

static bool aris_json_scope_begin(aris_json_context *ctx, aris_json_value scope)
{
    aris_json__push_scope(ctx, scope);
    if (!ctx->root) {
        ctx->code = ARIS_JSON_OK;
        ctx->root = &ctx->scopes[0];
    }

    return true;
}

static bool aris_json_scope_end(aris_json_context *ctx)
{
    /* ctx->root has the reference of ctx->scopes[0] */
    if (aris_vec__size(ctx->scopes) == 1) return true;

    aris_json_value pair_value = aris_json__pop_scope(ctx);
    aris_json__append_element(ctx, ctx->scope_type == ARIS_JSON_SCOPE_OBJECT
                              ? strdup(ctx->current_key) : NULL, pair_value);

    return true;
}

#ifdef ARIS_JSON_ENABLE_DESERIALIZATION
static long aris_json__peek(stb_lexer *lex)
{
    char *saved_point;
    long token;

    saved_point = lex->parse_point;
    stb_c_lexer_get_token(lex);
    token = lex->token;
    lex->parse_point = saved_point;

    return token;
}

static long aris_json__advance(stb_lexer *lex)
{
    stb_c_lexer_get_token(lex);
    return lex->token;
}

static const char *token_kind(long token)
{
    switch (token) {
    case CLEX_id:       return "identifier";
    case CLEX_dqstring: return "double quote string";
    case CLEX_sqstring: return "single quote string";
    case CLEX_charlit:  return "character";
    case CLEX_intlit:   return "integer";
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

static bool aris_json__consume(stb_lexer *lex, long expected, const char *msg)
{
    long token = aris_json__advance(lex);
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

static bool aris_json__parse_value(aris_json_context *ctx, stb_lexer *lex)
{
    long token = aris_json__advance(lex);
    switch (token) {
    case CLEX_dqstring:
        aris_json_string(ctx, lex->string);
        return true;

    case CLEX_intlit:
        aris_json_number(ctx, (float)lex->int_number);
        return true;

    case CLEX_floatlit:
        aris_json_number(ctx, (float)lex->real_number);
        return true;

    case CLEX_id:
        if (strcmp(lex->string, "null") == 0) {
            aris_json_null(ctx);
            return true;
        }
        if (strcmp(lex->string, "true") == 0) {
            aris_json_boolean(ctx, true);
            return true;
        }
        if (strcmp(lex->string, "false") == 0) {
            aris_json_boolean(ctx, false);
            return true;
        }
        return false;

    case '{':
        lex->parse_point--; /* put back the token for parse_object to consume */
        return aris_json__parse_object(ctx, lex);

    case '[':
        lex->parse_point--; /* put back the token for parse_array to consume */
        return aris_json__parse_array(ctx, lex);

    default:
        return false;
    }
}

static bool aris_json__parse_array(aris_json_context *ctx, stb_lexer *lex)
{
    if (!aris_json__consume(lex, '[', "array should start with '['")) {
        return false;
    }

    aris_json_array_begin(ctx);

    /* handle empty array */
    if (aris_json__peek(lex) == ']') {
        aris_json__advance(lex);
        aris_json_array_end(ctx);
        return true;
    }

    while (true) {
        if (!aris_json__parse_value(ctx, lex)) return false;

        if (aris_json__peek(lex) != ',') break;

        /* allow trailing comma at the end of array */
        aris_json__advance(lex);
        if (aris_json__peek(lex) == ']') break;
    }
    if (!aris_json__consume(lex, ']', "array should end with ']'")) return false;

    aris_json_array_end(ctx);

    return true;
}

static bool aris_json__parse_object(aris_json_context *ctx, stb_lexer *lex)
{
    if (!aris_json__consume(lex, '{', "object should start with '{'")) {
        return false;
    }

    aris_json_object_begin(ctx);

    /* handle empty object */
    if (aris_json__peek(lex) == '}') {
        aris_json__advance(lex);
        aris_json_object_end(ctx);
        return true;
    }

    while (true) {
        /* parse key */
        if (!aris_json__consume(lex, CLEX_dqstring, "key should be a string")) {
            return false;
        }
        aris_json_key(ctx, lex->string);

        /* parse colon separator */
        if (!aris_json__consume(lex, ':', "lack of ':' in a pair")) {
            return false;
        }

        /* parse value */
        if (!aris_json__parse_value(ctx, lex)) return false;

        if (aris_json__peek(lex) != ',') break;

        /* allow trailing comma at the end of object */
        aris_json__advance(lex);
        if (aris_json__peek(lex) == '}') break;
    }
    if (!aris_json__consume(lex, '}', "object should end with '}'")) return false;

    aris_json_object_end(ctx);

    return true;
}
#endif /* ARIS_JSON_ENABLE_DESERIALIZATION */

#undef aris_vec__header
#undef aris_vec__size
#undef aris_vec__capacity
#undef aris_vec__push
#undef aris_vec__pop
#undef aris_vec__free
#undef aris_vec__reset

#endif /* ARIS_JSON_IMPLEMENTATION */

#ifdef ARIS_JSON_STRIP_PREFIX

#define json_init                    aris_json_init
#define json_fini                    aris_json_fini
#define json_dump                    aris_json_dump
#define json_print_value             aris_json_print_value
#define json_default_write_to_buffer aris_json_default_write_to_buffer
#define json_default_write_to_file   aris_json_default_write_to_file
#define json_key                     aris_json_key
#define json_string                  aris_json_string
#define json_number                  aris_json_number
#define json_boolean                 aris_json_boolean
#define json_null                    aris_json_null
#define json_object_begin            aris_json_object_begin
#define json_object_end              aris_json_object_end
#define json_array_begin             aris_json_array_begin
#define json_array_end               aris_json_array_end
#define json_parse                   aris_json_parse
#define json_is_number               aris_json_is_number
#define json_is_string               aris_json_is_string
#define json_is_boolean              aris_json_is_boolean
#define json_to_number               aris_json_to_number
#define json_to_string               aris_json_to_string
#define json_to_boolean              aris_json_to_boolean
#define json_object_get_value        aris_json_object_get_value
#define json_object_get_pair         aris_json_object_get_pair
#define json_object_get_size         aris_json_object_get_size
#define json_array_get_value         aris_json_array_get_value
#define json_array_get_size          aris_json_array_get_size
#define json_context_get_root        aris_json_context_get_root

#endif /* ARIS_JSON_STRIP_PREFIX */

/*
------------------------------------------------------------------------------
This software is available under MIT license.
------------------------------------------------------------------------------
Copyright (c) 2025 Dylaris
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
