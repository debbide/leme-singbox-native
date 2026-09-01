#include <node_api.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

typedef int (*core_int_fn)(void);
typedef int (*core_config_fn)(const char *);
typedef char *(*core_string_fn)(void);
typedef void (*core_free_fn)(char *);

typedef struct {
#ifdef _WIN32
    HMODULE library;
#else
    void *library;
#endif
    core_int_fn abi_version;
    core_string_fn singbox_version;
    core_string_fn go_version;
    core_config_fn check_config;
    core_config_fn start;
    core_config_fn reload;
    core_int_fn stop;
    core_int_fn status;
    core_string_fn last_error;
    core_free_fn free_string;
} leme_core_api;

static leme_core_api api;

static napi_value throw_error(napi_env env, const char *message) {
    napi_throw_error(env, NULL, message);
    return NULL;
}

static void *resolve_symbol(const char *name) {
#ifdef _WIN32
    return (void *)GetProcAddress(api.library, name);
#else
    return dlsym(api.library, name);
#endif
}

static int resolve_api(void) {
    api.abi_version = (core_int_fn)resolve_symbol("leme_core_abi_version");
    api.singbox_version = (core_string_fn)resolve_symbol("leme_core_singbox_version");
    api.go_version = (core_string_fn)resolve_symbol("leme_core_go_version");
    api.check_config = (core_config_fn)resolve_symbol("leme_core_check_config");
    api.start = (core_config_fn)resolve_symbol("leme_core_start");
    api.reload = (core_config_fn)resolve_symbol("leme_core_reload");
    api.stop = (core_int_fn)resolve_symbol("leme_core_stop");
    api.status = (core_int_fn)resolve_symbol("leme_core_status");
    api.last_error = (core_string_fn)resolve_symbol("leme_core_last_error");
    api.free_string = (core_free_fn)resolve_symbol("leme_core_free_string");

    return api.abi_version && api.singbox_version && api.go_version
        && api.check_config && api.start && api.reload && api.stop
        && api.status && api.last_error && api.free_string;
}

static napi_value load_library(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    size_t length = 0;
    char *path = NULL;
    napi_value result;

    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    if (argc != 1) {
        return throw_error(env, "load requires a dynamic library path");
    }

    if (napi_get_value_string_utf8(env, argv[0], NULL, 0, &length) != napi_ok) {
        return throw_error(env, "library path must be a string");
    }

    path = (char *)malloc(length + 1);
    if (!path) {
        return throw_error(env, "failed to allocate library path");
    }
    napi_get_value_string_utf8(env, argv[0], path, length + 1, &length);

#ifdef _WIN32
    api.library = LoadLibraryA(path);
#else
    api.library = dlopen(path, RTLD_NOW | RTLD_LOCAL);
#endif
    free(path);

    if (!api.library) {
        return throw_error(env, "failed to load Leme native core library");
    }
    if (!resolve_api()) {
        return throw_error(env, "native core library is missing required ABI symbols");
    }

    napi_get_boolean(env, 1, &result);
    return result;
}

static napi_value call_int(napi_env env, core_int_fn fn) {
    napi_value result;
    if (!api.library || !fn) {
        return throw_error(env, "native core library is not loaded");
    }
    napi_create_int32(env, fn(), &result);
    return result;
}

static napi_value call_string(napi_env env, core_string_fn fn) {
    napi_value result;
    char *value;
    if (!api.library || !fn || !api.free_string) {
        return throw_error(env, "native core library is not loaded");
    }
    value = fn();
    napi_create_string_utf8(env, value ? value : "", NAPI_AUTO_LENGTH, &result);
    if (value) {
        api.free_string(value);
    }
    return result;
}

static napi_value call_config(napi_env env, napi_callback_info info, core_config_fn fn) {
    size_t argc = 1;
    napi_value argv[1];
    size_t length = 0;
    char *config;
    napi_value result;

    if (!api.library || !fn) {
        return throw_error(env, "native core library is not loaded");
    }
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    if (argc != 1 || napi_get_value_string_utf8(env, argv[0], NULL, 0, &length) != napi_ok) {
        return throw_error(env, "configuration must be a JSON string");
    }
    config = (char *)malloc(length + 1);
    if (!config) {
        return throw_error(env, "failed to allocate configuration buffer");
    }
    napi_get_value_string_utf8(env, argv[0], config, length + 1, &length);
    napi_create_int32(env, fn(config), &result);
    free(config);
    return result;
}

static napi_value abi_version(napi_env env, napi_callback_info info) { (void)info; return call_int(env, api.abi_version); }
static napi_value status(napi_env env, napi_callback_info info) { (void)info; return call_int(env, api.status); }
static napi_value stop(napi_env env, napi_callback_info info) { (void)info; return call_int(env, api.stop); }
static napi_value singbox_version(napi_env env, napi_callback_info info) { (void)info; return call_string(env, api.singbox_version); }
static napi_value go_version(napi_env env, napi_callback_info info) { (void)info; return call_string(env, api.go_version); }
static napi_value last_error(napi_env env, napi_callback_info info) { (void)info; return call_string(env, api.last_error); }
static napi_value check_config(napi_env env, napi_callback_info info) { return call_config(env, info, api.check_config); }
static napi_value start(napi_env env, napi_callback_info info) { return call_config(env, info, api.start); }
static napi_value reload_core(napi_env env, napi_callback_info info) { return call_config(env, info, api.reload); }

static napi_value init(napi_env env, napi_value exports) {
    napi_property_descriptor properties[] = {
        {"load", NULL, load_library, NULL, NULL, NULL, napi_default, NULL},
        {"abiVersion", NULL, abi_version, NULL, NULL, NULL, napi_default, NULL},
        {"singBoxVersion", NULL, singbox_version, NULL, NULL, NULL, napi_default, NULL},
        {"goVersion", NULL, go_version, NULL, NULL, NULL, napi_default, NULL},
        {"checkConfig", NULL, check_config, NULL, NULL, NULL, napi_default, NULL},
        {"start", NULL, start, NULL, NULL, NULL, napi_default, NULL},
        {"reload", NULL, reload_core, NULL, NULL, NULL, napi_default, NULL},
        {"stop", NULL, stop, NULL, NULL, NULL, napi_default, NULL},
        {"status", NULL, status, NULL, NULL, NULL, napi_default, NULL},
        {"lastError", NULL, last_error, NULL, NULL, NULL, napi_default, NULL}
    };
    napi_define_properties(env, exports, sizeof(properties) / sizeof(properties[0]), properties);
    return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, init)