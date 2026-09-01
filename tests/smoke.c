#include <stdio.h>
#include <stdlib.h>
#include "leme-singbox.h"

static int require_ok(int condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "FAILED: %s\n", message);
        return 0;
    }
    return 1;
}

int main(void) {
    int ok = 1;
    char config[] = "{\"log\":{\"disabled\":true},\"outbounds\":[{\"type\":\"direct\",\"tag\":\"direct\"}]}";
    char *version = leme_core_singbox_version();
    char *go_version = leme_core_go_version();

    printf("ABI: %d\n", leme_core_abi_version());
    printf("sing-box: %s\n", version);
    printf("Go: %s\n", go_version);

    ok &= require_ok(leme_core_abi_version() == 2, "unexpected ABI version");
    ok &= require_ok(leme_core_status() == 0, "core should initially be stopped");
    ok &= require_ok(leme_core_check_config(config) == 0, "valid configuration rejected");
    ok &= require_ok(leme_core_check_config("{") != 0, "invalid configuration accepted");

    ok &= require_ok(leme_core_start(config) == 0, "core failed to start");
    ok &= require_ok(leme_core_status() == 1, "core should be running");
    ok &= require_ok(leme_core_start(config) != 0, "duplicate start should fail");
    ok &= require_ok(leme_core_reload("{") != 0, "invalid reload should fail");
    ok &= require_ok(leme_core_status() == 1, "invalid reload should preserve running core");
    ok &= require_ok(leme_core_reload(config) == 0, "valid reload failed");
    ok &= require_ok(leme_core_status() == 1, "core should run after reload");
    ok &= require_ok(leme_core_stop() == 0, "core failed to stop");
    ok &= require_ok(leme_core_status() == 0, "core should be stopped");
    ok &= require_ok(leme_core_stop() == 0, "stop should be idempotent");

    for (int i = 0; i < 100 && ok; i++) {
        ok &= require_ok(leme_core_start(config) == 0, "repeated start failed");
        ok &= require_ok(leme_core_status() == 1, "core did not enter running state");
        ok &= require_ok(leme_core_stop() == 0, "repeated stop failed");
        ok &= require_ok(leme_core_status() == 0, "core did not return to stopped state");
    }

    if (!ok) {
        char *error = leme_core_last_error();
        fprintf(stderr, "Last error: %s\n", error);
        leme_core_free_string(error);
    }

    leme_core_free_string(version);
    leme_core_free_string(go_version);
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
