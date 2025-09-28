# json.h

## Brief

`json.h` is a stb-style C library for serializing and deserializing JSON.

## Dependency

- `stb_c_lexer_h`: Used to parse the tokens in the JSON file.

- `nob.h`: To build examples.

## Usage

```shell
$ cc -o nob nob.c
$ ./nob
```

- serialization

```c
json_object_begin(&ctx);
    json_key(&ctx, "string");
    json_string(&ctx, "hello");

    json_key(&ctx, "number");
    json_number(&ctx, 1.2);

    json_key(&ctx, "boolean");
    json_boolean(&ctx, false);

    json_key(&ctx, "null");
    json_null(&ctx);
json_object_end(&ctx);
```

```shell
output:

{
    "string": "hello",
    "number": 1.2,
    "boolean": false,
    "null": null
}
```

- deserialization

```c
const char *object = "{\"name\": \"Jack\", \"age\": 20, \"student\": false, }";

if (!json_parse(&ctx, object, strlen(object))) return 1;

const Json_Value *root = json_context_get_root(&ctx);
const Json_Value *name = json_object_get_value(root, "name");
if (json_is_string(name)) printf("name: %s\n", json_to_string(name));

const Json_Value *age = json_object_get_value(root, "age");
if (json_is_number(age)) printf("age: %d\n", (int)json_to_number(age));
```

```shell
output:

name: Jack
age: 20
```

## Reference

- [tsoding/jim](https://github.com/tsoding/jim)
