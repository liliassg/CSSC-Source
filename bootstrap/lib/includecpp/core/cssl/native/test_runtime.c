#include "cssc_runtime.h"
#include <stdio.h>

int main(void) {
    cssc_runtime_init();

    CsscVal i = cssc_int(42);
    CsscVal f = cssc_float(3.14);
    CsscVal s = cssc_string("Hello CSSC!");
    CsscVal b = cssc_bool(true);
    CsscVal n = cssc_null();

    cssc_builtin_outln(s);

    CsscVal sum = cssc_add(cssc_int(100), cssc_int(200));
    cssc_builtin_out(cssc_string("100 + 200 = "));
    cssc_builtin_outln(sum);

    CsscVal hello = cssc_string("Hello");
    CsscVal world = cssc_string(" World");
    CsscVal concat = cssc_string_concat(hello, world);
    cssc_builtin_outln(concat);

    CsscVal vec = cssc_vector(8);
    cssc_vector_push(vec, cssc_int(10));
    cssc_vector_push(vec, cssc_int(20));
    cssc_vector_push(vec, cssc_int(30));
    cssc_builtin_out(cssc_string("vec size = "));
    cssc_builtin_outln(cssc_int(cssc_vector_size(vec)));
    cssc_builtin_out(cssc_string("vec[1] = "));
    cssc_builtin_outln(cssc_vector_get(vec, 1));

    CsscScopeStack* scope = cssc_global_scope();
    cssc_scope_set(scope, "myVar", cssc_int(999));
    CsscVal looked_up = cssc_scope_get(scope, "myVar");
    cssc_builtin_out(cssc_string("scope myVar = "));
    cssc_builtin_outln(looked_up);

    cssc_builtin_out(cssc_string("typeof(42) = "));
    cssc_builtin_outln(cssc_builtin_typeof(i));
    cssc_builtin_out(cssc_string("typeof(3.14) = "));
    cssc_builtin_outln(cssc_builtin_typeof(f));
    cssc_builtin_out(cssc_string("typeof('hello') = "));
    cssc_builtin_outln(cssc_builtin_typeof(s));

    cssc_builtin_out(cssc_string("10 < 20 = "));
    cssc_builtin_outln(cssc_bool(cssc_lt(cssc_int(10), cssc_int(20))));

    CsscVal map = cssc_map(16);
    cssc_map_set(map, "name", cssc_string("CSSC"));
    cssc_map_set(map, "version", cssc_string("2.0"));
    cssc_builtin_out(cssc_string("map[name] = "));
    cssc_builtin_outln(cssc_map_get(map, "name"));

    cssc_builtin_out(cssc_string("formatted vec = "));
    cssc_builtin_outln(cssc_format_value(vec));

    cssc_release(vec);
    cssc_release(map);
    cssc_release(concat);
    cssc_release(s);
    cssc_release(sum);

    cssc_builtin_outln(cssc_string("=== ALL TESTS PASSED ==="));

    cssc_runtime_shutdown();
    return 0;
}
