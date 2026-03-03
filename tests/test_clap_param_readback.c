#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "dsp/clap_host.h"

int main(void) {
    printf("Testing CLAP pending param readback...\n");

    clap_instance_t inst = {0};
    int rc = clap_load_plugin("tests/fixtures/clap/test_param.clap", 0, &inst);
    assert(rc == 0);

    double initial = clap_param_get(&inst, 1);
    assert(fabs(initial - 0.0) < 1e-9);

    rc = clap_param_set(&inst, 1, 0.75);
    assert(rc == 0);

    /* Regression: UI readback should reflect just-set values immediately,
       even before the next process block consumes queued param events. */
    double readback = clap_param_get(&inst, 1);
    assert(fabs(readback - 0.75) < 1e-9);

    clap_unload_plugin(&inst);
    printf("OK\n");
    return 0;
}
