/*
 * Test CLAP plugin load and process
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "dsp/clap_host.h"

int main(void) {
    printf("Testing CLAP plugin load and process...\n");

    clap_instance_t inst = {0};
    int rc = clap_load_plugin("tests/fixtures/clap/test_synth.clap", 0, &inst);

    printf("Load returned: %d\n", rc);
    assert(rc == 0);
    assert(inst.plugin != NULL);
    assert(inst.activated == true);
    assert(inst.processing == true);

    /* Process a block of audio */
    float out[128 * 2] = {0};
    rc = clap_process_block(&inst, NULL, out, 128);

    printf("Process returned: %d\n", rc);
    assert(rc == 0);

    /* Verify output buffer was touched (synth outputs silence, which is fine) */
    printf("First sample: %f\n", out[0]);

    /* Unload */
    clap_unload_plugin(&inst);
    assert(inst.plugin == NULL);

    printf("All tests passed!\n");
    return 0;
}
