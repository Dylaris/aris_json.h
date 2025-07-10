// Merging JSON involves treating the content of each JSON file as a key, 
// adding it under a new key, and finally combining all the key-value pairs 
// into one unified JSON.

#define CJSON_IMPLEMENTATION
#include "cjson.h"

static char *read_file(const char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "failed to open file: '%s'\n", filename);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);

    if (size <= 0) {
        fprintf(stderr, "file '%s' is empty\n", filename);
        fclose(fp);
        return NULL;
    }

    char *buf = malloc(size + 1);
    assert(buf != NULL);

    size_t read = fread(buf, 1, size, fp);
    fclose(fp);

    if (read != (size_t)size) {
        fprintf(stderr, "Read error: expected %ld bytes, got %zu\n", size, read);
        free(buf);
        return NULL;
    }

    buf[size] = '\0';
    return buf;
}

const char *jsons[] = {
    "test1.json",
    "test2.json"
};

int main(void)
{
    CJson_Context cj = {0};
    cjson_init(&cj, stdout, "\t");

    cjson_object_begin(&cj);
        for (unsigned i = 0; i < sizeof(jsons)/sizeof(jsons[0]); i++) {
            cjson_key(&cj, jsons[i]);
            char *input = read_file(jsons[i]);
            if (!cjson_parse(&cj, input, strlen(input))) return 1;
            free(input);
        }
    cjson_object_end(&cj);

    cjson_dump(&cj);

    cjson_fini(&cj);
    return 0;
}
