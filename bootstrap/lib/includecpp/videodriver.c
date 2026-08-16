
#include "cssc_runtime.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cssc_allocwatcher.h"
#include "cssc_gipeo.h"
#include "cssc_tft.h"
#include "cssc_http.h"

static CsscVal _cssc_label_VideoDriver_boot(CsscObject* _obj, CsscVal* _args, uint32_t _nargs);
static CsscVal _cssc_label_VideoDriver_tick(CsscObject* _obj, CsscVal* _args, uint32_t _nargs);
static CsscVal _cssc_top_VideoDriver(CsscObject* _obj, CsscVal* _args, uint32_t _nargs);
static CsscVal _cssc_free_VideoDriver(CsscObject* _obj, CsscVal* _args, uint32_t _nargs);

int main(void) {
    cssc_runtime_init();
    CsscScopeStack* _scope = cssc_global_scope();

    {

        const char* _mod_name = "gipeo";
        char _dll_path[1024];
        CsscVal _mod_val = cssc_null();

        snprintf(_dll_path, sizeof(_dll_path), "%s.dll", _mod_name);
        _mod_val = cssc_load_dll(_scope, _dll_path, "gipeo");
        if (CSSC_IS_NULL(_mod_val)) {

            const char* _appdata = getenv("APPDATA");
            if (_appdata) {
                snprintf(_dll_path, sizeof(_dll_path), "%s\\cssc\\modules\\%s.dll", _appdata, _mod_name);
                _mod_val = cssc_load_dll(_scope, _dll_path, "gipeo");
            }
        }
    }

    {

        const char* _mod_name = "tft";
        char _dll_path[1024];
        CsscVal _mod_val = cssc_null();

        snprintf(_dll_path, sizeof(_dll_path), "%s.dll", _mod_name);
        _mod_val = cssc_load_dll(_scope, _dll_path, "tft");
        if (CSSC_IS_NULL(_mod_val)) {

            const char* _appdata = getenv("APPDATA");
            if (_appdata) {
                snprintf(_dll_path, sizeof(_dll_path), "%s\\cssc\\modules\\%s.dll", _appdata, _mod_name);
                _mod_val = cssc_load_dll(_scope, _dll_path, "tft");
            }
        }
    }

    CsscVal _t3 = cssc_scope_get(_scope, "bus");
    CsscVal _t4[] = {cssc_int(400000LL)};
    CsscVal _t2;
    if (CSSC_TYPE(_t3) == CSSC_TYPE_SECTOR) {
        cssc_sector_push_members(_scope, _t3);
        CsscVal _fn = cssc_sector_get(_t3, "begin");
        _t2 = cssc_function_call(_fn, _t4, 1);
        cssc_scope_pop(_scope);
    } else if (CSSC_TYPE(_t3) == CSSC_TYPE_OBJECT) {
        _t2 = cssc_object_call_label(_t3, "begin", _t4, 1);
    } else {
        _t2 = cssc_null();
    }
    CsscVal _t1 = _t2;
    (void)_t1;
    CsscVal _t5 = cssc_tft_create_pins("ssd1306", (uint16_t)(128), (uint16_t)(64), (int16_t)(12), (int16_t)(14), (uint8_t)(0x3C));
    cssc_scope_set(_scope, "display", _t5);
    cssc_release(_t5);

    static CsscLabel _t6[] = {
        { "boot", (void*)_cssc_label_VideoDriver_boot, 0, 0 },
        { "tick", (void*)_cssc_label_VideoDriver_tick, 0, 0 },
    };
    cssc_scope_set(_scope, "VideoDriver", cssc_object_create("VideoDriver", _t6, 2, (void*)_cssc_top_VideoDriver, (void*)_cssc_free_VideoDriver));
    CsscVal _t39 = cssc_scope_get(_scope, "VideoDriver");
    CsscVal _t38;
    if (CSSC_TYPE(_t39) == CSSC_TYPE_OBJECT) {
        _t38 = cssc_object_execute(_t39, NULL, 0);
    } else {
        _t38 = cssc_function_call(_t39, NULL, 0);
    }
    cssc_scope_set(_scope, "vd", _t38);
    CsscVal _t42 = cssc_scope_get(_scope, "vd");
    CsscVal _t41;
    if (CSSC_TYPE(_t42) == CSSC_TYPE_SECTOR) {
        cssc_sector_push_members(_scope, _t42);
        CsscVal _fn = cssc_sector_get(_t42, "boot");
        _t41 = cssc_function_call(_fn, NULL, 0);
        cssc_scope_pop(_scope);
    } else if (CSSC_TYPE(_t42) == CSSC_TYPE_OBJECT) {
        _t41 = cssc_object_call_label(_t42, "boot", NULL, 0);
    } else {
        _t41 = cssc_null();
    }
    CsscVal _t40 = _t41;
    (void)_t40;
    while (1) {
        if (!cssc_is_truthy(cssc_bool(true))) break;
        CsscVal _t44 = cssc_scope_get(_scope, "vd");
        CsscVal _t43;
        if (CSSC_TYPE(_t44) == CSSC_TYPE_SECTOR) {
            cssc_sector_push_members(_scope, _t44);
            CsscVal _fn = cssc_sector_get(_t44, "tick");
            _t43 = cssc_function_call(_fn, NULL, 0);
            cssc_scope_pop(_scope);
        } else if (CSSC_TYPE(_t44) == CSSC_TYPE_OBJECT) {
            _t43 = cssc_object_call_label(_t44, "tick", NULL, 0);
        } else {
            _t43 = cssc_null();
        }
        cssc_scope_set(_scope, "r", _t43);
        { CsscVal _t45 = cssc_scope_get(_scope, "r");
          if (CSSC_TYPE(_t45) == CSSC_TYPE_CONSOLE) cssc_console_free(_t45);
          cssc_scope_delete(_scope, "r"); }
        cssc_builtin_sleep(cssc_to_float(cssc_int(20LL)));
        CsscVal _t46 = cssc_null();
        (void)_t46;
        CsscVal _t47 = cssc_builtin_uptime();
        _t47.tag |= CSSC_FLAG_STACK | ((uint64_t)64 << 32);
        cssc_scope_set(_scope, "uptime", _t47);
        cssc_release(_t47);
        if (cssc_is_truthy(cssc_bool(cssc_ge(cssc_scope_get(_scope, "uptime"), cssc_sector_get(cssc_scope_get(_scope, "vd"), "shutdownAt"))))) {
            break;
        }
        { CsscVal _t48 = cssc_scope_get(_scope, "uptime");
          if (CSSC_TYPE(_t48) == CSSC_TYPE_CONSOLE) cssc_console_free(_t48);
          cssc_scope_delete(_scope, "uptime"); }
    }
    { CsscVal _t49 = cssc_scope_get(_scope, "vd");
      if (CSSC_TYPE(_t49) == CSSC_TYPE_CONSOLE) cssc_console_free(_t49);
      cssc_scope_delete(_scope, "vd"); }
    { CsscVal _t50 = cssc_scope_get(_scope, "bus");
      if (CSSC_TYPE(_t50) == CSSC_TYPE_CONSOLE) cssc_console_free(_t50);
      cssc_scope_delete(_scope, "bus"); }
    { CsscVal _t51 = cssc_scope_get(_scope, "display");
      if (CSSC_TYPE(_t51) == CSSC_TYPE_CONSOLE) cssc_console_free(_t51);
      cssc_scope_delete(_scope, "display"); }
    { CsscVal _fv = cssc_scope_get(_scope, "gipeo");
      if (CSSC_TYPE(_fv) == CSSC_TYPE_OBJECT) cssc_object_free(_fv);
      else cssc_sector_free(_fv); }
    cssc_scope_delete(_scope, "gipeo");
    { CsscVal _fv = cssc_scope_get(_scope, "tft");
      if (CSSC_TYPE(_fv) == CSSC_TYPE_OBJECT) cssc_object_free(_fv);
      else cssc_sector_free(_fv); }
    cssc_scope_delete(_scope, "tft");
    { CsscVal _t52 = cssc_scope_get(_scope, "START_TIME");
      if (CSSC_TYPE(_t52) == CSSC_TYPE_CONSOLE) cssc_console_free(_t52);
      cssc_scope_delete(_scope, "START_TIME"); }

    cssc_runtime_shutdown();
    return 0;
}

static CsscVal _cssc_label_VideoDriver_boot(CsscObject* _obj, CsscVal* _args, uint32_t _nargs) {
    CsscScopeStack* _scope = cssc_global_scope();
    cssc_scope_push(_scope);
    CsscVal _obj_val; _obj_val.tag = CSSC_TYPE_OBJECT; _obj_val.data.ptr = _obj;
    cssc_scope_set(_scope, "VideoDriver", _obj_val);
    int _mirror_set = 0;
    CsscVal _mirror_value = cssc_null();
    (void)_mirror_set; (void)_mirror_value;
    cssc_tft_begin(cssc_scope_get(_scope, "display"));
    CsscVal _t7 = cssc_null();
    (void)_t7;
    cssc_tft_fill(cssc_scope_get(_scope, "display"), (uint32_t)cssc_to_int(cssc_int(0x0000)));
    CsscVal _t8 = cssc_null();
    (void)_t8;
    cssc_tft_text(cssc_scope_get(_scope, "display"), (int16_t)cssc_to_int(cssc_int(4LL)), (int16_t)cssc_to_int(cssc_int(16LL)), cssc_to_cstr(cssc_sector_get(cssc_scope_get(_scope, "VideoDriver"), "title")), (uint32_t)cssc_to_int(cssc_int(0xFFFF)), (uint8_t)cssc_to_int(cssc_int(1LL)));
    CsscVal _t9 = cssc_null();
    (void)_t9;
    cssc_tft_text(cssc_scope_get(_scope, "display"), (int16_t)cssc_to_int(cssc_int(4LL)), (int16_t)cssc_to_int(cssc_int(32LL)), cssc_to_cstr(cssc_sector_get(cssc_scope_get(_scope, "VideoDriver"), "tag")), (uint32_t)cssc_to_int(cssc_int(0xFFFF)), (uint8_t)cssc_to_int(cssc_int(1LL)));
    CsscVal _t10 = cssc_null();
    (void)_t10;
    CsscVal _t11 = cssc_string("boot ok");
    _t11.tag |= CSSC_FLAG_STACK | ((uint64_t)64 << 32);
    cssc_scope_set(_scope, "msg", _t11);
    cssc_release(_t11);
    cssc_tft_text(cssc_scope_get(_scope, "display"), (int16_t)cssc_to_int(cssc_int(4LL)), (int16_t)cssc_to_int(cssc_int(48LL)), cssc_to_cstr(cssc_scope_get(_scope, "msg")), (uint32_t)cssc_to_int(cssc_int(0xFFFF)), (uint8_t)cssc_to_int(cssc_int(1LL)));
    CsscVal _t12 = cssc_null();
    (void)_t12;
    cssc_tft_show(cssc_scope_get(_scope, "display"));
    CsscVal _t13 = cssc_null();
    (void)_t13;
    { CsscVal _t14 = cssc_scope_get(_scope, "msg");
      if (CSSC_TYPE(_t14) == CSSC_TYPE_CONSOLE) cssc_console_free(_t14);
      cssc_scope_delete(_scope, "msg"); }
    cssc_builtin_sleep(cssc_to_float(cssc_int(2500LL)));
    CsscVal _t15 = cssc_null();
    (void)_t15;
    cssc_scope_pop(_scope);
    return _mirror_value;
}
static CsscVal _cssc_label_VideoDriver_tick(CsscObject* _obj, CsscVal* _args, uint32_t _nargs) {
    CsscScopeStack* _scope = cssc_global_scope();
    cssc_scope_push(_scope);
    CsscVal _obj_val; _obj_val.tag = CSSC_TYPE_OBJECT; _obj_val.data.ptr = _obj;
    cssc_scope_set(_scope, "VideoDriver", _obj_val);
    int _mirror_set = 0;
    CsscVal _mirror_value = cssc_null();
    (void)_mirror_set; (void)_mirror_value;
    cssc_tft_fill(cssc_scope_get(_scope, "display"), (uint32_t)cssc_to_int(cssc_int(0x0000)));
    CsscVal _t16 = cssc_null();
    (void)_t16;
    cssc_tft_text(cssc_scope_get(_scope, "display"), (int16_t)cssc_to_int(cssc_int(0LL)), (int16_t)cssc_to_int(cssc_int(25LL)), cssc_to_cstr(cssc_builtin_uptime()), (uint32_t)cssc_to_int(cssc_int(0xFFFF)), (uint8_t)cssc_to_int(cssc_int(1LL)));
    CsscVal _t17 = cssc_null();
    (void)_t17;
    CsscVal _t18 = cssc_add(cssc_add(cssc_string("CSSC Embedded ("), cssc_sector_get(cssc_scope_get(_scope, "VideoDriver"), "frame")), cssc_string(")"));
    _t18.tag |= CSSC_FLAG_STACK | ((uint64_t)1028 << 32);
    cssc_scope_set(_scope, "flabel", _t18);
    cssc_release(_t18);
    cssc_tft_text(cssc_scope_get(_scope, "display"), (int16_t)cssc_to_int(cssc_int(0LL)), (int16_t)cssc_to_int(cssc_int(0LL)), cssc_to_cstr(cssc_scope_get(_scope, "flabel")), (uint32_t)cssc_to_int(cssc_int(0xFFFF)), (uint8_t)cssc_to_int(cssc_int(1LL)));
    CsscVal _t19 = cssc_null();
    (void)_t19;
    { CsscVal _t20 = cssc_scope_get(_scope, "flabel");
      if (CSSC_TYPE(_t20) == CSSC_TYPE_CONSOLE) cssc_console_free(_t20);
      cssc_scope_delete(_scope, "flabel"); }
    cssc_tft_line(cssc_scope_get(_scope, "display"), (int16_t)cssc_to_int(cssc_int(0LL)), (int16_t)cssc_to_int(cssc_int(39LL)), (int16_t)cssc_to_int(cssc_int(127LL)), (int16_t)cssc_to_int(cssc_int(38LL)), (uint32_t)cssc_to_int(cssc_int(0xFFFF)));
    CsscVal _t21 = cssc_null();
    (void)_t21;
    cssc_tft_fillrect(cssc_scope_get(_scope, "display"), (int16_t)cssc_to_int(cssc_sector_get(cssc_scope_get(_scope, "VideoDriver"), "box_x")), (int16_t)cssc_to_int(cssc_int(46LL)), (int16_t)cssc_to_int(cssc_int(14LL)), (int16_t)cssc_to_int(cssc_int(14LL)), (uint32_t)cssc_to_int(cssc_int(0xFFFF)));
    CsscVal _t22 = cssc_null();
    (void)_t22;
    cssc_sector_set(cssc_scope_get(_scope, "VideoDriver"), "box_x", cssc_add(cssc_sector_get(cssc_scope_get(_scope, "VideoDriver"), "box_x"), cssc_sector_get(cssc_scope_get(_scope, "VideoDriver"), "dir")));
    if (cssc_is_truthy(cssc_bool(cssc_gt(cssc_sector_get(cssc_scope_get(_scope, "VideoDriver"), "box_x"), cssc_int(114LL))))) {
        cssc_sector_set(cssc_scope_get(_scope, "VideoDriver"), "dir", cssc_neg(cssc_int(4LL)));
    }
    if (cssc_is_truthy(cssc_bool(cssc_lt(cssc_sector_get(cssc_scope_get(_scope, "VideoDriver"), "box_x"), cssc_int(0LL))))) {
        cssc_sector_set(cssc_scope_get(_scope, "VideoDriver"), "dir", cssc_int(4LL));
    }
    cssc_tft_show(cssc_scope_get(_scope, "display"));
    CsscVal _t23 = cssc_null();
    (void)_t23;
    cssc_sector_set(cssc_scope_get(_scope, "VideoDriver"), "frame", cssc_add(cssc_sector_get(cssc_scope_get(_scope, "VideoDriver"), "frame"), cssc_int(1LL)));
    if (!_mirror_set) { _mirror_value = cssc_sector_get(cssc_scope_get(_scope, "VideoDriver"), "frame"); _mirror_set = 1; }
    cssc_scope_pop(_scope);
    return _mirror_value;
}
static CsscVal _cssc_top_VideoDriver(CsscObject* _obj, CsscVal* _args, uint32_t _nargs) {
    CsscScopeStack* _scope = cssc_global_scope();
    cssc_scope_push(_scope);
    CsscVal _obj_val; _obj_val.tag = CSSC_TYPE_OBJECT; _obj_val.data.ptr = _obj;
    cssc_scope_set(_scope, "VideoDriver", _obj_val);
    int _mirror_set = 0;
    CsscVal _mirror_value = cssc_null();
    (void)_mirror_set; (void)_mirror_value;
    cssc_scope_set(_scope, "display", cssc_scope_get(_scope, "display"));
    cssc_scope_set(_scope, "tft", cssc_scope_get(_scope, "tft"));
    CsscVal _t24 = cssc_float(2000000.0);
    _t24.tag |= CSSC_FLAG_STACK | ((uint64_t)64 << 32);
    cssc_sector_set(cssc_scope_get(_scope, "VideoDriver"), "shutdownAt", _t24);
    cssc_release(_t24);
    CsscVal _t25 = cssc_string("VideoDriver v1.0.1");
    _t25.tag |= CSSC_FLAG_STACK | ((uint64_t)256 << 32);
    cssc_sector_set(cssc_scope_get(_scope, "VideoDriver"), "title", _t25);
    cssc_release(_t25);
    CsscVal _t26 = cssc_add(cssc_add(cssc_string("VideoDriver "), cssc_copy(cssc_sector_get(cssc_scope_get(_scope, "VideoDriver"), "shutdownAt"))), cssc_string("s"));
    _t26.tag |= CSSC_FLAG_STACK | ((uint64_t)256 << 32);
    cssc_sector_set(cssc_scope_get(_scope, "VideoDriver"), "tag", _t26);
    cssc_release(_t26);
    CsscVal _t27 = cssc_scope_get(_scope, "START_TIME");
    _t27.tag |= CSSC_FLAG_STACK | ((uint64_t)32 << 32);
    cssc_sector_set(cssc_scope_get(_scope, "VideoDriver"), "frame", _t27);
    cssc_release(_t27);
    CsscVal _t28 = cssc_int(0);
    _t28.tag |= CSSC_FLAG_STACK | ((uint64_t)32 << 32);
    cssc_sector_set(cssc_scope_get(_scope, "VideoDriver"), "box_x", _t28);
    cssc_release(_t28);
    CsscVal _t29 = cssc_int(4LL);
    _t29.tag |= CSSC_FLAG_STACK | ((uint64_t)32 << 32);
    cssc_sector_set(cssc_scope_get(_scope, "VideoDriver"), "dir", _t29);
    cssc_release(_t29);
    cssc_scope_pop(_scope);
    return _mirror_value;
}
static CsscVal _cssc_free_VideoDriver(CsscObject* _obj, CsscVal* _args, uint32_t _nargs) {
    (void)_args; (void)_nargs;
    CsscScopeStack* _scope = cssc_global_scope();
    cssc_scope_push(_scope);
    CsscVal _obj_val; _obj_val.tag = CSSC_TYPE_OBJECT; _obj_val.data.ptr = _obj;
    cssc_scope_set(_scope, "VideoDriver", _obj_val);
    int _mirror_set = 0;
    CsscVal _mirror_value = cssc_null();
    (void)_mirror_set; (void)_mirror_value;
    cssc_tft_fill(cssc_scope_get(_scope, "display"), (uint32_t)cssc_to_int(cssc_int(0x0000)));
    CsscVal _t30 = cssc_null();
    (void)_t30;
    CsscVal _t31 = cssc_string("VideoDriver");
    _t31.tag |= CSSC_FLAG_STACK | ((uint64_t)128 << 32);
    cssc_scope_set(_scope, "bye1", _t31);
    cssc_release(_t31);
    CsscVal _t32 = cssc_string("shutdown");
    _t32.tag |= CSSC_FLAG_STACK | ((uint64_t)128 << 32);
    cssc_scope_set(_scope, "bye2", _t32);
    cssc_release(_t32);
    cssc_tft_text(cssc_scope_get(_scope, "display"), (int16_t)cssc_to_int(cssc_int(4LL)), (int16_t)cssc_to_int(cssc_int(24LL)), cssc_to_cstr(cssc_scope_get(_scope, "bye1")), (uint32_t)cssc_to_int(cssc_int(0xFFFF)), (uint8_t)cssc_to_int(cssc_int(1LL)));
    CsscVal _t33 = cssc_null();
    (void)_t33;
    cssc_tft_text(cssc_scope_get(_scope, "display"), (int16_t)cssc_to_int(cssc_int(4LL)), (int16_t)cssc_to_int(cssc_int(40LL)), cssc_to_cstr(cssc_scope_get(_scope, "bye2")), (uint32_t)cssc_to_int(cssc_int(0xFFFF)), (uint8_t)cssc_to_int(cssc_int(1LL)));
    CsscVal _t34 = cssc_null();
    (void)_t34;
    cssc_tft_show(cssc_scope_get(_scope, "display"));
    CsscVal _t35 = cssc_null();
    (void)_t35;
    { CsscVal _t36 = cssc_scope_get(_scope, "bye1");
      if (CSSC_TYPE(_t36) == CSSC_TYPE_CONSOLE) cssc_console_free(_t36);
      cssc_scope_delete(_scope, "bye1"); }
    { CsscVal _t37 = cssc_scope_get(_scope, "bye2");
      if (CSSC_TYPE(_t37) == CSSC_TYPE_CONSOLE) cssc_console_free(_t37);
      cssc_scope_delete(_scope, "bye2"); }
    cssc_sector_set(cssc_scope_get(_scope, "VideoDriver"), "title", cssc_null());
    cssc_sector_set(cssc_scope_get(_scope, "VideoDriver"), "tag", cssc_null());
    cssc_sector_set(cssc_scope_get(_scope, "VideoDriver"), "frame", cssc_null());
    cssc_sector_set(cssc_scope_get(_scope, "VideoDriver"), "box_x", cssc_null());
    cssc_sector_set(cssc_scope_get(_scope, "VideoDriver"), "dir", cssc_null());
    cssc_sector_set(cssc_scope_get(_scope, "VideoDriver"), "shutdownAt", cssc_null());
    cssc_scope_pop(_scope);
    return _mirror_value;
}