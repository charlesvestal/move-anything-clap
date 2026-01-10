/*
 * Test CLAP parameter enumeration
 */
#include <assert.h>
#include <stdio.h>
#include "dsp/clap_host.h"

int main(void) {
    printf("Testing CLAP parameter enumeration...\n");

    /* First, we need a plugin with parameters. Create a test fixture. */
    clap_instance_t inst = {0};
    int rc = clap_load_plugin("tests/fixtures/clap/test_param.clap", 0, &inst);

    printf("Load returned: %d\n", rc);
    assert(rc == 0);

    /* Check parameter count */
    int count = clap_param_count(&inst);
    printf("Parameter count: %d\n", count);
    assert(count > 0);

    /* Get parameter info */
    char name[64];
    double min, max, def;
    rc = clap_param_info(&inst, 0, name, sizeof(name), &min, &max, &def);
    printf("Param 0: %s (min=%f, max=%f, def=%f)\n", name, min, max, def);
    assert(rc == 0);

    clap_unload_plugin(&inst);

    printf("All tests passed!\n");
    return 0;
}
