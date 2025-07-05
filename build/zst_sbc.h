// BRIEF:
//
// This file implements a simple build system. And it only uses
// c to build c project. So I call it 'self build c' --- 'sbc'.
//
// LICENSE:
//
// See end of file for license information.

#ifndef ZST_SBC_H
#define ZST_SBC_H

#include <assert.h>     // for assert
#include <stdio.h>      // for printf, fprintf
#include <stdlib.h>     // for realloc, free
#include <string.h>     // for strerror
#include <stdbool.h>    // for bool
#include <stdarg.h>     // for va_list, va_start, ...
#include <errno.h>      // for errno

#ifdef _WIN32
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif // platform

typedef struct {
    unsigned count;
    unsigned capacity;
    char **items;
} zst_sbc_cmd_t;

static void zst__sbc_cmd_append(zst_sbc_cmd_t *cmd, ...);
static void zst__sbc_rebuild_self(char *cc, int argc, char **argv, char *source, ...);
static bool zst__sbc_copy_file(char *src, char *dest);
char *zst_sbc_cmd_concat(zst_sbc_cmd_t *cmd);
bool zst_sbc_cmd_run(zst_sbc_cmd_t *cmd);

#ifdef ZST_SBC_IMPLEMENTATION
#define zst_sbc_rebuild_self(cc, argc, argv, ...) \
    zst__sbc_rebuild_self(cc, argc, argv, __FILE__, ##__VA_ARGS__, NULL)
#else
#define zst_sbc_rebuild_self(cc, argc, argv, ...)
#endif

#ifdef ZST_SBC_IMPLEMENTATION
#define zst_sbc_log(fmt, ...) fprintf(stdout, fmt"\n", ##__VA_ARGS__)
#else
#define zst_sbc_log(fmt, ...)
#endif

#ifdef ZST_SBC_IMPLEMENTATION
#define zst_sbc_cmd_append(cmd, ...) zst__sbc_cmd_append((cmd), ##__VA_ARGS__, NULL);
#else
#define zst_sbc_cmd_append(cmd, ...)
#endif

#endif // ZST_SBC_H

#ifdef ZST_SBC_IMPLEMENTATION

#ifndef ZST_ARRAY_RESIZE
#define ZST_ARRAY_RESIZE(da, sz)                                        \
    do {                                                                \
        const unsigned align_size = 8;                                  \
        unsigned size = ((sz)+align_size-1) & ~(align_size-1);          \
        if ((da)->capacity <= size) {                                   \
            (da)->capacity = ((da)->capacity==0) ? 8 : (da)->capacity*2;\
            while ((da)->capacity <= size) (da)->capacity *= 2;         \
            (da)->items = realloc((da)->items,                          \
                    sizeof(*(da)->items)*(da)->capacity);               \
            assert((da)->items != NULL);                                \
        }                                                               \
    } while (0)
#endif

static struct {
    unsigned count;
    unsigned capacity;
    zst_sbc_cmd_t *items;
} __cmdlist;

static unsigned __get_file_timestamp(const char *filename)
{
    assert(filename != NULL);
    unsigned timestamp = 0;
#ifdef _WIN32
    HANDLE hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ,
                              NULL, OPEN_EXISTING, 0, NULL);
    assert(hFile != INVALID_HANDLE_VALUE);
    FILETIME ftCreate, ftAccess, ftWrite;
    assert(GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite));
    SYSTEMTIME stUTC, stLocal;
    FileTimeToSystemTime(&ftWrite, &stUTC);
    SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);
    ULONGLONG ft = (((ULONGLONG)ftWrite.dwHighDateTime)<<32)
                    + ftWrite.dwLowDateTime;
    timestamp = (unsigned)((ft-116444736000000000LL)/10000000);
    CloseHandle(hFile);
    return timestamp;
#else
    struct stat sb;
    assert(stat(filename, &sb) != -1);
    timestamp = (unsigned) sb.st_mtime;
#endif // platform
    return timestamp;
}

static void zst__sbc_rebuild_self(char *cc, int argc, char **argv, char *source, ...)
{
    assert(cc != NULL && argv != NULL && source != NULL);

    unsigned long long t1 = __get_file_timestamp(argv[0]);
    unsigned long long t2 = __get_file_timestamp(source);
    if (t1 >= t2) return; // Don't need to update

    char backup_name[strlen(source)+10];
    snprintf(backup_name, sizeof(backup_name), "%s.old", source);

    if(!zst__sbc_copy_file(source, backup_name)) exit(1);

    zst_sbc_cmd_t cmd = {0};

    // Compile
    zst_sbc_cmd_append(&cmd, cc);
    zst_sbc_cmd_append(&cmd, "-o", argv[0], source);

    // Append extra args
    va_list args;
    va_start(args, source);
    char *arg;
    while ((arg = va_arg(args, char*)) != NULL) {
        zst_sbc_cmd_append(&cmd, arg);
    }
    va_end(args);

    // Execute
    zst_sbc_cmd_append(&cmd, "&&");
    for (int i = 0; i < argc; i++) {
        zst_sbc_cmd_append(&cmd, argv[i]);
    }

    zst_sbc_log("[INFO] << Rebuild Self >>");

    // Restore and report error if has
    if (!zst_sbc_cmd_run(&cmd)) {
        zst_sbc_log("[ERROR] failed to rebuild self");
        if (zst__sbc_copy_file(backup_name, source)) {
            remove(backup_name);
        }
        exit(1);
    }

    remove(backup_name);
    exit(0);
}

static bool zst__sbc_copy_file(char *src, char *dest)
{
    assert(src != NULL && dest != NULL);

    FILE *fp = fopen(src, "r");
    if (!fp) {
        zst_sbc_log("[ERROR] failed to open file '%s' to read: %s",
                    src, strerror(errno));
        return false;
    }

    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    rewind(fp);

    char buf[size];
    fread(buf, 1, size, fp);
    fclose(fp);

    fp = fopen(dest, "w");
    if (!fp) {
        zst_sbc_log("[ERROR] failed to open file '%s' to write: %s",
                    dest, strerror(errno));
        return false;
    }

    fwrite(buf, 1, size, fp);
    fclose(fp);

    return true;
}

static void zst__sbc_cmd_append(zst_sbc_cmd_t *cmd, ...)
{    
    assert(cmd != NULL);

    va_list args;
    va_start(args, cmd);

    char *arg;
    while ((arg = va_arg(args, char*)) != NULL) {
        if (cmd->capacity <= cmd->count) {
            ZST_ARRAY_RESIZE(cmd, cmd->count);
        }
        cmd->items[cmd->count++] = arg;
    }

    va_end(args);
}

char *zst_sbc_cmd_concat(zst_sbc_cmd_t *cmd)
{
    assert(cmd != NULL);
    if (cmd->count == 0) return NULL;

    size_t buf_size = 0;
    for (unsigned i = 0; i < cmd->count; i++) {
        buf_size += strlen(cmd->items[i]) + 1;
    }

    char *buf = malloc(buf_size);
    assert(buf != NULL);
    size_t pos = 0;
    for (unsigned i = 0; i < cmd->count; i++) {
        size_t len = strlen(cmd->items[i]);
        strncpy(buf+pos, cmd->items[i], len);
        pos += len;
        buf[pos++] = ' ';
    }
    buf[buf_size-1] = '\0';

    return buf;
}

bool zst_sbc_cmd_run(zst_sbc_cmd_t *cmd)
{
    assert(cmd != NULL);

    if (__cmdlist.capacity <= __cmdlist.count) {
        ZST_ARRAY_RESIZE(&__cmdlist, __cmdlist.count);
        __cmdlist.items[__cmdlist.count++] = *cmd;
    }

    char *cmdstr = zst_sbc_cmd_concat(cmd);
    if (!cmdstr) return false;

    zst_sbc_log("[CMD] %s", cmdstr);

    int status = system(cmdstr);
    if (status != 0) {
        zst_sbc_log("[ERROR] failed to execute command: %s", strerror(errno));
        goto fail;
    }

    free(cmdstr);
ok:
    return true;
fail:
    return false;
}

#undef ZST_ARRAY_RESIZE

#endif // ZST_SBC_IMPLEMENTATION

#ifdef ZST_SBC_NO_PREFIX

#define sbc_log          zst_sbc_log
#define sbc_cmd_concat   zst_sbc_cmd_concat
#define sbc_cmd_run      zst_sbc_cmd_run
#define sbc_cmd_append   zst_sbc_cmd_append
#define sbc_cmdlist_free zst_sbc_cmdlist_free
#define sbc_rebuild_self zst_sbc_rebuild_self

typedef zst_sbc_cmd_t sbc_cmd_t;

#endif // ZST_SBC_NO_PREFIX

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
