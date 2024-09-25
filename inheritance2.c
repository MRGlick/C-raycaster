#include <stdio.h>

#define DEF_STRUCT(name, typename, ...) enum {typename = __COUNTER__}; typedef struct name __VA_ARGS__ name;
#define END_STRUCT(typename) enum {typename##_END = __COUNTER__};
#define typename __COUNTER__ \
#define instanceof(type, parent_type) (type >= parent_type && type <= parent_type##_END)

DEF_STRUCT(Foo, FOO, {
    int x, y, z;
})

    DEF_STRUCT(Bar, BAR, {
        Foo foo;
        double a, b, c;
    })
        DEF_STRUCT(Baz, BAZ, {
            Bar bar;
            float d, e, f;
        })

    END_STRUCT(BAR)

END_STRUCT(FOO);


int main(int argc, char *argv[]) {
    printf("foo: %d, bar: %d, baz: %d, foo_end: %d, bar_end: %d \n", FOO, BAR, BAZ, FOO_END, BAR_END);
}