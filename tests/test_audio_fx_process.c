/*
 * Test CLAP audio FX processing (for Signal Chain integration)
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "dsp/clap_host.h"

int main(void) {
    printf("Testing CLAP audio FX processing...\n");

    /* Load the test FX plugin (has audio input) */
    clap_instance_t inst = {0};
    int rc = clap_load_plugin("tests/fixtures/clap/test_fx.clap", 0, &inst);

    printf("Load returned: %d\n", rc);
    assert(rc == 0);

    /* Process audio with input */
    float in[128 * 2];
    float out[128 * 2];

    /* Fill input with a test signal */
    for (int i = 0; i < 128 * 2; i++) {
        in[i] = 0.5f;  /* DC signal */
    }
    memset(out, 0, sizeof(out));

    rc = clap_process_block(&inst, in, out, 128);
    printf("Process returned: %d\n", rc);
    assert(rc == 0);

    /* Verify output (test_fx is pass-through) */
    printf("First input: %f, first output: %f\n", in[0], out[0]);
    assert(out[0] == in[0]);  /* Pass-through should match */

    clap_unload_plugin(&inst);

    printf("All tests passed!\n");
    return 0;
}
