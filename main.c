#include <ffi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

struct parser_state {
    char *target;
    size_t index;
    void *result;
    char *error_msg;
    _Bool is_error;
};

typedef struct parser_state *(*parser_fn)(struct parser_state *);

struct parser_state *parser_state_dup(struct parser_state *parser_state) {
    struct parser_state *clone = malloc(sizeof(*parser_state));
    memcpy(clone, parser_state, sizeof(*parser_state));

    return clone;
}

void parser_state_set_error(struct parser_state *parser_state,
                            char *error_msg) {
    parser_state->is_error  = 1;
    parser_state->error_msg = error_msg;
}

void parser_state_destroy(struct parser_state *parser_state) {
    free(parser_state);
}

void str_binding(ffi_cif *cif, void *ret, void *args[], void *_str) {
    char *str               = _str;
    struct parser_state *in = *(struct parser_state **)args[0];

    size_t nstr    = strlen(str);
    size_t ntarget = strlen(in->target + in->index);

    struct parser_state *out = parser_state_dup(in);
    if (memcmp(str, in->target + in->index, MIN(nstr, ntarget)) == 0) {
        out->result = str;
        out->index += nstr;
    }

    parser_state_destroy(in);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wint-conversion"
    *(ffi_arg *)ret = out;
#pragma clang diagnostic pop
}

void sequence_of_binding(ffi_cif *cif, void *ret, void *args[],
                         void *_parsers) {
    parser_fn *parsers      = _parsers;
    struct parser_state *in = *(struct parser_state **)args[0];
    size_t nparsers         = 0;

    for (parser_fn *curr = parsers; *curr != NULL; curr++, nparsers++) {
        printf("%p, %zu\n", curr, nparsers);
    };

    fprintf(stderr, "#sequence_of: %zu\n", nparsers);
    for (size_t i = 0; i < nparsers; i++) {
    }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wint-conversion"
    *(ffi_arg *)ret = in;
#pragma clang diagnostic pop
}

#define MAKE_BINDING(binding_name, binding, input_type, ffi_return_type, ...)  \
    [[nodiscard]] parser_fn binding_name(input_type a) {                       \
        ffi_cif cif;                                                           \
        ffi_type **args;                                                       \
        ffi_closure *closure;                                                  \
        ffi_type *args_static[] = {__VA_ARGS__};                               \
        size_t nargs            = sizeof(args_static) / sizeof(ffi_type *);    \
        args                    = malloc(sizeof(ffi_type *) * nargs);          \
        void *bound;                                                           \
        int rc;                                                                \
        closure = ffi_closure_alloc(sizeof(ffi_closure), &bound);              \
        if (closure) {                                                         \
            memcpy(args, args_static, nargs * sizeof(ffi_type *));             \
            if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, nargs, ffi_return_type,    \
                             args) == FFI_OK) {                                \
                if (ffi_prep_closure_loc(closure, &cif, binding, a, bound) ==  \
                    FFI_OK) {                                                  \
                    return ((parser_fn)bound);                                 \
                }                                                              \
            }                                                                  \
        }                                                                      \
        return NULL;                                                           \
    }

MAKE_BINDING(str, str_binding, char *, &ffi_type_pointer, &ffi_type_pointer)
MAKE_BINDING(sequence_of, sequence_of_binding, parser_fn *, &ffi_type_pointer,
             &ffi_type_pointer)

int main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) {
    parser_fn parsers[] = {str("Hello"), str(" "), str("World"), NULL};

    parser_fn hello_world_matcher = sequence_of(parsers);

    struct parser_state *parser_state = calloc(1, sizeof(*parser_state));
    parser_state->target              = "Hello World";

    hello_world_matcher(parser_state);
    fprintf(stderr, "%s %zu\n", parser_state->target, parser_state->index);

    return EXIT_SUCCESS;
}