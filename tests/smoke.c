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
    char *version = leme_core_singbox_version();
    char *go_version = leme_core_go_version();

    printf("ABI: %d\n", leme_core_abi_version());
    printf("sing-box: %s\n", version);
    printf("Go: %s\n", go_version);

    ok &= require_ok(leme_core_abi_version() == 1, "unexpected ABI version");
    ok &= require_ok(leme_core_check_config("{\"log\":{\"disabled\":true},\"outbounds\":[{\"type\":\"direct\",\"tag\":\"direct\"}]}") == 0, "valid configuration rejected");
    ok &= require_ok(leme_core_check_config("{") != 0, "invalid configuration accepted");

    if (!ok) {
        char *error = leme_core_last_error();
        fprintf(stderr, "Last error: %s\n", error);
        leme_core_free_string(error);
    }

    leme_core_free_string(version);
    leme_core_free_string(go_version);
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
