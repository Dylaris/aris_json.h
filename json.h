/*
json.h - v0.01 - Dylaris 2025
===================================================

BRIEF:
  A C library for serializing and deserializing json.

NOTICE:
  This implementation directly supports serialization,
  but for deserialization, 'stb_c_lexer.h' needs to be used.
  And it is not compatiable with C++. (no test)

USAGE:
  In exactly one source file, define the implementation macro
  before including this header:
  ```
    #define JSON_IMPLEMENTATION
    #define JSON_ENABLE_DESERIALIZATION (optional)
    #include "json.h"
  ```
  In other files, just include the header without the macro.

LICENSE:
  See the end of this file for further details.
*/

#ifndef JSON_H
#define JSON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef enum Json_Value_Type {
    JSON_VALUE_NULL = 0,
    JSON_VALUE_OBJECT,
    JSON_VALUE_ARRAY,
    JSON_VALUE_STRING,
    JSON_VALUE_NUMBER,
    JSON_VALUE_BOOLEAN,
} Json_Value_Type;

typedef enum Json_Scope_Type {
    JSON_SCOPE_NULL = 0,
    JSON_SCOPE_OBJECT,
    JSON_SCOPE_ARRAY,
} Json_Scope_Type;

typedef enum Json_Error_Code {
    JSON_OK = 0,
    JSON_DOUBLE_KEY,
    JSON_NULL_KEY,
    JSON_KEY_OVERFLOW,
    JSON_INCORRECT_SCOPE,
    JSON_NO_SCOPE,
} Json_Error_Code;

typedef struct Json_Value Json_Value;
typedef struct Json_Pair Json_Pair;

struct Json_Value {
    Json_Value_Type type;
    union {
        char *string;
        double number;
        bool boolean;
        Json_Pair *object; /* array of Json_Pair */
        Json_Value *array; /* array of Json_Value */
    } as;
};

struct Json_Pair {
    char *key;
    Json_Value value;
};

typedef enum Json_Output_Mode {
    JSON_FILE_OUTPUT = 1,
    JSON_BUFFER_OUTPUT
} Json_Output_Mode;

typedef struct Json_Opt {
    const char *indent;
    Json_Output_Mode mode;
    void (*write_to_buffer)(const char*, char*, size_t);
    char *output_buffer;
    size_t output_buffer_size;
    void (*write_to_file)(const char*, FILE*);
    FILE *output_file;
} Json_Opt;

typedef struct Json_Context {
    Json_Scope_Type scope_type; /* current scope type */
    Json_Value *scopes;         /* array of Json_Value (object or array) */
    char *error_buffer;         /* store the latest error string */
    char *current_key;          /* store the current member key */
    Json_Value *root;           /* root object */
    Json_Error_Code code;
    Json_Opt opt;
} Json_Context;

#define json_init(ctx, ...) json_init_opt(ctx, (Json_Opt){__VA_ARGS__})
void json_init_opt(Json_Context *ctx, Json_Opt opt);
void json_fini(Json_Context *ctx);
void json_dump(Json_Context *ctx);
void json_default_write_to_buffer(const char *s, char *buffer, size_t size);
void json_default_write_to_file(const char *s, FILE *file);
void json_print_value(const Json_Value *value);

/* serialization */
bool json_key(Json_Context *ctx, const char *key);
bool json_string(Json_Context *ctx, const char *value);
bool json_number(Json_Context *ctx, double value);
bool json_boolean(Json_Context *ctx, bool value);
bool json_null(Json_Context *ctx);
bool json_object_begin(Json_Context *ctx);
bool json_object_end(Json_Context *ctx);
bool json_array_begin(Json_Context *ctx);
bool json_array_end(Json_Context *ctx);

#ifdef JSON_ENABLE_DESERIALIZATION
/* deserialization */
bool json_parse(Json_Context *ctx, const char *input, size_t size);
#endif /* JSON_ENABLE_DESERIALIZATION */

/* query */
const Json_Value *json_object_get_value(const Json_Value *root, const char *key);
const Json_Pair *json_object_get_pair(const Json_Value *root, size_t idx);
size_t json_object_get_size(const Json_Value *root);
const Json_Value *json_array_get_value(const Json_Value *root, size_t idx);
size_t json_array_get_size(const Json_Value *root);
#define json_context_get_root(context) ((context)->root)
#define json_is_number(value)  ((value)->type == JSON_VALUE_NUMBER)
#define json_is_string(value)  ((value)->type == JSON_VALUE_STRING)
#define json_is_boolean(value) ((value)->type == JSON_VALUE_BOOLEAN)
#define json_is_object(value)  ((value)->type == JSON_VALUE_OBJECT)
#define json_is_array(value)   ((value)->type == JSON_VALUE_ARRAY)
#define json_to_number(value)  ((value)->as.number)
#define json_to_string(value)  ((value)->as.string)
#define json_to_boolean(value) ((value)->as.boolean)

#endif /* JSON_H */

#ifdef JSON_IMPLEMENTATION

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

#ifdef JSON_ENABLE_DESERIALIZATION
#define STB_C_LEXER_IMPLEMENTATION
#include "stb_c_lexer.h"
#endif /* JSON_ENABLE_DESERIALIZATION */

#define JSON__ERROR_BUFFER_SIZE 1024
#define JSON__KEY_MAX_SIZE      256

static void json__write(Json_Context *ctx, const char *s);
static void json__set_error(Json_Context *ctx, const char *key, Json_Error_Code code);
static Json_Value *json__get_current_scope(Json_Context *ctx);
static void json__append_element(Json_Context *ctx, char *key, Json_Value value);
static void json__free_value(Json_Value *value);
static void json__free_pair(Json_Pair *pair);
static void json__push_scope(Json_Context *ctx, Json_Value scope);
static Json_Value json__pop_scope(Json_Context *ctx);
static void json__dump_pair(Json_Context *ctx, size_t level, Json_Pair *pair, bool comma);
static void json__dump_value(Json_Context *ctx, size_t level, Json_Value *value, bool indent);
static void json__dump_indent(Json_Context *ctx, size_t level);
static bool json_scope_begin(Json_Context *ctx, Json_Value scope);
static bool json_scope_end(Json_Context *ctx);
#ifdef JSON_ENABLE_DESERIALIZATION
static long json__peek(stb_lexer *lex);
static long json__advance(stb_lexer *lex);
static bool json__consume(stb_lexer *lex, long expected, const char *msg);
static bool json__parse_value(Json_Context *ctx, stb_lexer *lex);
static bool json__parse_array(Json_Context *ctx, stb_lexer *lex);
static bool json__parse_object(Json_Context *ctx, stb_lexer *lex);
#endif /* JSON_ENABLE_DESERIALIZATION */

void json_init_opt(Json_Context *ctx, Json_Opt opt)
{
    ctx->scopes = NULL;
    ctx->scope_type = JSON_SCOPE_NULL;
    ctx->code = JSON_NO_SCOPE;
    ctx->root = NULL;
    ctx->error_buffer= malloc(JSON__ERROR_BUFFER_SIZE + 1);
    ctx->current_key = malloc(JSON__KEY_MAX_SIZE + 1);
    if (!ctx->error_buffer || !ctx->current_key) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    json__set_error(ctx, NULL, JSON_NO_SCOPE);

    /* use the default configuration if it is not specified */
    if (!opt.indent)          opt.indent = "\t";
    if (!opt.mode)            opt.mode = JSON_FILE_OUTPUT;
    if (!opt.write_to_buffer) opt.write_to_buffer = json_default_write_to_buffer;
    if (!opt.write_to_file)   opt.write_to_file = json_default_write_to_file;
    if (!opt.output_file)     opt.output_file = stdout;
    ctx->opt = opt;
}

void json_fini(Json_Context *ctx)
{
    /* ctx->root has the reference of ctx->scopes[0], so free the
       attached pairs first throuth root. */
    json__free_value(ctx->root);
    ctx->root = NULL;
    aris_vec__free(ctx->scopes);
    ctx->scope_type = JSON_SCOPE_NULL;
    ctx->code = JSON_NO_SCOPE;
    if (ctx->error_buffer) free(ctx->error_buffer);
    if (ctx->current_key) free(ctx->current_key);
    ctx->error_buffer = NULL;
    ctx->current_key = NULL;
}

void json_dump(Json_Context *ctx)
{
    if (ctx->code != JSON_OK) return;
    json__dump_value(ctx, 0, ctx->root, true);
}

void json_print_value(const Json_Value *value)
{
    switch (value->type) {
    case JSON_VALUE_NULL:
        printf("type: null, value: null\n");
        break;
    case JSON_VALUE_OBJECT:
        printf("type: object, value: {...}\n");
        break;
    case JSON_VALUE_ARRAY:
        printf("type: array, value: [...]\n");
        break;
    case JSON_VALUE_STRING:
        printf("type: string, value: '%s'\n", value->as.string);
        break;
    case JSON_VALUE_NUMBER:
        printf("type: number, value: '%.15g'\n", value->as.number);
        break;
    case JSON_VALUE_BOOLEAN:
        printf("type: boolean, value: '%s'\n", value->as.boolean
                                               ? "true" : "false");
        break;
    default:
        break;
    }
}

void json_default_write_to_buffer(const char *s, char *buffer, size_t size)
{
    static size_t pos = 0;
    size_t len = strlen(s);
    if (pos + len < size) {
        strncpy(buffer+pos, s, len);
        pos += len;
        buffer[pos] = '\0';
    }
}

void json_default_write_to_file(const char *s, FILE *file)
{
    fwrite(s, strlen(s), 1, file);
}

bool json_key(Json_Context *ctx, const char *key)
{
    if (ctx->code != JSON_OK) return false;

    if (!key) {
        json__set_error(ctx, NULL, JSON_NULL_KEY);
        return false;
    }
    if (strlen(key) > JSON__KEY_MAX_SIZE) {
        json__set_error(ctx, key, JSON_KEY_OVERFLOW);
        return false;
    }

    if (ctx->scope_type == JSON_SCOPE_NULL) {
        json__set_error(ctx, key, JSON_NO_SCOPE);
        return false;
    } else if (ctx->scope_type == JSON_SCOPE_OBJECT) {
        /* check if the key already exists */
        Json_Value *scope = json__get_current_scope(ctx);
        if (json_object_get_value(scope, key)) {
            json__set_error(ctx, key, JSON_DOUBLE_KEY);
            return false;
        } else {
            snprintf(ctx->current_key, JSON__KEY_MAX_SIZE, "%s", key);
            return true;
        }
    } else {
        json__set_error(ctx, NULL, JSON_INCORRECT_SCOPE);
        return false;
    }
}

bool json_string(Json_Context *ctx, const char *value)
{
    if (ctx->code != JSON_OK) return false;

    Json_Value pair_value = {
        .type = JSON_VALUE_STRING,
        .as.string = value ? strdup(value) : NULL
    };
    char *pair_key = ctx->scope_type == JSON_SCOPE_ARRAY
                     ? NULL : strdup(ctx->current_key);
    json__append_element(ctx, pair_key, pair_value);

    return true;
}

bool json_number(Json_Context *ctx, double value)
{
    if (ctx->code != JSON_OK) return false;

    Json_Value pair_value = {
        .type = JSON_VALUE_NUMBER,
        .as.number = value
    };
    char *pair_key = ctx->scope_type == JSON_SCOPE_ARRAY
                     ? NULL : strdup(ctx->current_key);
    json__append_element(ctx, pair_key, pair_value);

    return true;
}

bool json_boolean(Json_Context *ctx, bool value)
{
    if (ctx->code != JSON_OK) return false;

    Json_Value pair_value = {
        .type = JSON_VALUE_BOOLEAN,
        .as.boolean = value
    };
    char *pair_key = ctx->scope_type == JSON_SCOPE_ARRAY
                     ? NULL : strdup(ctx->current_key);
    json__append_element(ctx, pair_key, pair_value);

    return true;
}

bool json_null(Json_Context *ctx)
{
    if (ctx->code != JSON_OK) return false;

    Json_Value pair_value = {
        .type = JSON_VALUE_NULL
    };
    char *pair_key = ctx->scope_type == JSON_SCOPE_ARRAY
                     ? NULL : strdup(ctx->current_key);
    json__append_element(ctx, pair_key, pair_value);

    return true;
}

bool json_object_begin(Json_Context *ctx)
{
    if (ctx->code != JSON_OK && ctx->code != JSON_NO_SCOPE) {
        return false;
    }
    Json_Value scope = {
        .type = JSON_VALUE_OBJECT,
        .as.object = NULL,
    };
    return json_scope_begin(ctx, scope);
}

bool json_object_end(Json_Context *ctx)
{
    if (ctx->code != JSON_OK) return false;
    return json_scope_end(ctx);
}

bool json_array_begin(Json_Context *ctx)
{
    if (ctx->code != JSON_OK && ctx->code != JSON_NO_SCOPE) {
        return false;
    }
    Json_Value scope = {
        .type = JSON_VALUE_ARRAY,
        .as.array = NULL,
    };
    return json_scope_begin(ctx, scope);
}

bool json_array_end(Json_Context *ctx)
{
    if (ctx->code != JSON_OK) return false;
    return json_scope_end(ctx);
}

#ifdef JSON_ENABLE_DESERIALIZATION
bool json_parse(Json_Context *ctx, const char *input, size_t size)
{
    if (size == 0) return false;

    stb_lexer lex = {0};
    long token;
    static char string_store[4096];

    stb_c_lexer_init(&lex, input, input + size,
                     string_store, sizeof(string_store));

    token = json__peek(&lex);
    if (token == '{') {
        return json__parse_object(ctx, &lex);
    } else if (token == '[') {
        return json__parse_array(ctx, &lex);
    } else {
        return false;
    }
}
#endif /* JSON_ENABLE_DESERIALIZATION */

const Json_Value *json_object_get_value(const Json_Value *root, const char *key)
{
    if (!key || !json_is_object(root)) return NULL;

    for (size_t i = 0; i < aris_vec__size(root->as.object); i++) {
        Json_Pair *pair = &root->as.object[i];
        if (pair->key && strcmp(pair->key, key) == 0) return &pair->value;
    }

    return NULL;
}

const Json_Pair *json_object_get_pair(const Json_Value *root, size_t idx)
{
    if (!json_is_object(root) || idx >= aris_vec__size(root->as.object)) return NULL;
    return &root->as.object[idx];
}

size_t json_object_get_size(const Json_Value *root)
{
    return json_is_object(root) ? aris_vec__size(root->as.object) : 0;
}

const Json_Value *json_array_get_value(const Json_Value *root, size_t idx)
{
    if (!json_is_array(root) || idx >= aris_vec__size(root->as.array)) return NULL;
    return &root->as.array[idx];
}

size_t json_array_get_size(const Json_Value *root)
{
    return json_is_array(root) ? aris_vec__size(root->as.array) : 0;
}

static void json__write(Json_Context *ctx, const char *s)
{
    if (ctx->opt.mode == JSON_BUFFER_OUTPUT) {
        ctx->opt.write_to_buffer(s, ctx->opt.output_buffer,
                                 ctx->opt.output_buffer_size);
    } else {
        ctx->opt.write_to_file(s, ctx->opt.output_file);
    }
}

static void json__set_error(Json_Context *ctx, const char *key, Json_Error_Code code)
{
    ctx->code = code;

    switch (code) {
    case JSON_OK:
        break;

    case JSON_DOUBLE_KEY:
        snprintf(ctx->error_buffer, JSON__ERROR_BUFFER_SIZE+1,
                 "ERROR: double key '%s'!\n", key);
        break;

    case JSON_NULL_KEY:
        snprintf(ctx->error_buffer, JSON__ERROR_BUFFER_SIZE+1,
                 "ERROR: null key!\n");
        break;

    case JSON_KEY_OVERFLOW:
        snprintf(ctx->error_buffer, JSON__ERROR_BUFFER_SIZE+1,
                 "ERROR: key overflow '%s' (maxsize = %d)!\n",
                 key, JSON__KEY_MAX_SIZE);
        break;

    case JSON_NO_SCOPE:
        snprintf(ctx->error_buffer, JSON__ERROR_BUFFER_SIZE+1,
                 "ERROR: what was done without a scope!\n");
        break;

    case JSON_INCORRECT_SCOPE:
        snprintf(ctx->error_buffer, JSON__ERROR_BUFFER_SIZE+1,
                 "ERROR: what was done within incorrect scope!\n");
        break;

    default:
        snprintf(ctx->error_buffer, JSON__ERROR_BUFFER_SIZE+1,
                 "ERROR: unknown code!\n");
        break;
    }
}

static Json_Value *json__get_current_scope(Json_Context *ctx)
{
    if (ctx->code != JSON_OK) return NULL;
    if (aris_vec__size(ctx->scopes) == 0) return NULL;
    return &ctx->scopes[aris_vec__size(ctx->scopes) - 1];
}

static void json__append_element(Json_Context *ctx, char *key, Json_Value value)
{
    Json_Value *scope = json__get_current_scope(ctx);
    if (ctx->scope_type == JSON_SCOPE_OBJECT) {
        Json_Pair pair = {key, value};
        aris_vec__push(scope->as.object, pair);
    } else if (ctx->scope_type == JSON_SCOPE_ARRAY) {
        aris_vec__push(scope->as.array, value);
    }
}

static void json__free_value(Json_Value *value)
{
    switch (value->type) {
    case JSON_VALUE_OBJECT:
        for (size_t i = 0; i < aris_vec__size(value->as.object); i++) {
            json__free_pair(&value->as.object[i]);
        }
        aris_vec__free(value->as.object);
        break;

    case JSON_VALUE_ARRAY:
        for (size_t i = 0; i < aris_vec__size(value->as.array); i++) {
            json__free_value(&value->as.array[i]);
        }
        aris_vec__free(value->as.array);
        break;

    case JSON_VALUE_STRING:
        if (value->as.string) free(value->as.string);
        value->as.string = NULL;
        break;

    case JSON_VALUE_NUMBER:
    case JSON_VALUE_BOOLEAN:
    case JSON_VALUE_NULL:
        break;

    default:
        break;
    }
}

static void json__free_pair(Json_Pair *pair)
{
    if (pair->key) free(pair->key);
    pair->key = NULL;
    json__free_value(&pair->value);
}

static void json__push_scope(Json_Context *ctx, Json_Value scope)
{
    aris_vec__push(ctx->scopes, scope);
    ctx->scope_type = scope.type == JSON_VALUE_ARRAY
                      ? JSON_SCOPE_ARRAY : JSON_SCOPE_OBJECT;
}

static Json_Value json__pop_scope(Json_Context *ctx)
{
    Json_Value res = ctx->scopes[--aris_vec__header(ctx->scopes)->size];

    if (aris_vec__size(ctx->scopes) > 0) {
        Json_Value *scope = json__get_current_scope(ctx);
        ctx->scope_type = scope->type == JSON_VALUE_ARRAY
                          ? JSON_SCOPE_ARRAY : JSON_SCOPE_OBJECT;
    }

    return res;
}

static void json__dump_pair(Json_Context *ctx, size_t level, Json_Pair *pair, bool comma)
{
    json__dump_indent(ctx, level);

    static char key[JSON__KEY_MAX_SIZE + 4] = {0};
    snprintf(key, sizeof(key), "\"%s\": ", pair->key);
    json__write(ctx, key);
    json__dump_value(ctx, level, &pair->value, false);

    if (comma) {
        json__write(ctx, ",\n");
    } else {
        json__write(ctx, "\n");
    }
}

static void json__dump_value(Json_Context *ctx, size_t level, Json_Value *value, bool indent)
{
    static char buffer[50];

    if (indent) json__dump_indent(ctx, level);

    switch (value->type) {
    case JSON_VALUE_OBJECT:
        json__write(ctx, "{\n");
        for (size_t i = 0; i < aris_vec__size(value->as.object); i++) {
            Json_Pair *pair = &value->as.object[i];
            bool comma = true;
            if (i == aris_vec__size(value->as.object) - 1) comma = false;
            json__dump_pair(ctx, level+1, pair, comma);
        }
        json__dump_indent(ctx, level);
        json__write(ctx, "}");
        break;

    case JSON_VALUE_ARRAY:
        json__write(ctx, "[\n");
        for (size_t i = 0; i < aris_vec__size(value->as.array); i++) {
            json__dump_value(ctx, level+1, &value->as.array[i], true);
            if (i == aris_vec__size(value->as.array) - 1) {
                json__write(ctx, "\n");
            } else {
                json__write(ctx, ",\n");
            }
        }
        json__dump_indent(ctx, level);
        json__write(ctx, "]");
        break;

    case JSON_VALUE_STRING:
        json__write(ctx, "\"");
        json__write(ctx, value->as.string);
        json__write(ctx, "\"");
        break;

    case JSON_VALUE_NUMBER:
        snprintf(buffer, sizeof(buffer), "%.15g", value->as.number);
        json__write(ctx, buffer);
        break;

    case JSON_VALUE_BOOLEAN:
        if (value->as.boolean) {
            json__write(ctx, "true");
        } else {
            json__write(ctx, "false");
        }
        break;

    case JSON_VALUE_NULL:
        json__write(ctx, "null");
        break;

    default:
        break;
    }
}

static void json__dump_indent(Json_Context *ctx, size_t level)
{
    for (size_t i = 0; i < level; i++) {
        json__write(ctx, ctx->opt.indent);
    }
}

static bool json_scope_begin(Json_Context *ctx, Json_Value scope)
{
    json__push_scope(ctx, scope);
    if (!ctx->root) {
        ctx->code = JSON_OK;
        ctx->root = &ctx->scopes[0];
    }

    return true;
}

static bool json_scope_end(Json_Context *ctx)
{
    /* ctx->root has the reference of ctx->scopes[0] */
    if (aris_vec__size(ctx->scopes) == 1) return true;

    Json_Value pair_value = json__pop_scope(ctx);
    json__append_element(ctx, ctx->scope_type == JSON_SCOPE_OBJECT
                              ? strdup(ctx->current_key) : NULL, pair_value);

    return true;
}

#ifdef JSON_ENABLE_DESERIALIZATION
static long json__peek(stb_lexer *lex)
{
    char *saved_point;
    long token;

    saved_point = lex->parse_point;
    stb_c_lexer_get_token(lex);
    token = lex->token;
    lex->parse_point = saved_point;

    return token;
}

static long json__advance(stb_lexer *lex)
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

static bool json__consume(stb_lexer *lex, long expected, const char *msg)
{
    long token = json__advance(lex);
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

static bool json__parse_value(Json_Context *ctx, stb_lexer *lex)
{
    long token = json__advance(lex);
    switch (token) {
    case CLEX_dqstring:
        json_string(ctx, lex->string);
        return true;

    case CLEX_intlit:
        json_number(ctx, (float)lex->int_number);
        return true;

    case CLEX_floatlit:
        json_number(ctx, (float)lex->real_number);
        return true;

    case CLEX_id:
        if (strcmp(lex->string, "null") == 0) {
            json_null(ctx);
            return true;
        }
        if (strcmp(lex->string, "true") == 0) {
            json_boolean(ctx, true);
            return true;
        }
        if (strcmp(lex->string, "false") == 0) {
            json_boolean(ctx, false);
            return true;
        }
        return false;

    case '{':
        lex->parse_point--; /* put back the token for parse_object to consume */
        return json__parse_object(ctx, lex);

    case '[':
        lex->parse_point--; /* put back the token for parse_array to consume */
        return json__parse_array(ctx, lex);

    default:
        return false;
    }
}

static bool json__parse_array(Json_Context *ctx, stb_lexer *lex)
{
    if (!json__consume(lex, '[', "array should start with '['")) {
        return false;
    }

    json_array_begin(ctx);

    /* handle empty array */
    if (json__peek(lex) == ']') {
        json__advance(lex);
        json_array_end(ctx);
        return true;
    }

    while (true) {
        if (!json__parse_value(ctx, lex)) return false;

        if (json__peek(lex) != ',') break;

        /* allow trailing comma at the end of array */
        json__advance(lex);
        if (json__peek(lex) == ']') break;
    }
    if (!json__consume(lex, ']', "array should end with ']'")) return false;

    json_array_end(ctx);

    return true;
}

static bool json__parse_object(Json_Context *ctx, stb_lexer *lex)
{
    if (!json__consume(lex, '{', "object should start with '{'")) {
        return false;
    }

    json_object_begin(ctx);

    /* handle empty object */
    if (json__peek(lex) == '}') {
        json__advance(lex);
        json_object_end(ctx);
        return true;
    }

    while (true) {
        /* parse key */
        if (!json__consume(lex, CLEX_dqstring, "key should be a string")) {
            return false;
        }
        json_key(ctx, lex->string);

        /* parse colon separator */
        if (!json__consume(lex, ':', "lack of ':' in a pair")) {
            return false;
        }

        /* parse value */
        if (!json__parse_value(ctx, lex)) return false;

        if (json__peek(lex) != ',') break;

        /* allow trailing comma at the end of object */
        json__advance(lex);
        if (json__peek(lex) == '}') break;
    }
    if (!json__consume(lex, '}', "object should end with '}'")) return false;

    json_object_end(ctx);

    return true;
}
#endif /* JSON_ENABLE_DESERIALIZATION */

#undef aris_vec__header
#undef aris_vec__size
#undef aris_vec__capacity
#undef aris_vec__push
#undef aris_vec__pop
#undef aris_vec__free
#undef aris_vec__reset

#endif /* JSON_IMPLEMENTATION */

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
