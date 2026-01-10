/*
 * Test CLAP plugin discovery and port classification
 */
#include <assert.h>
#include <stdio.h>
#include "dsp/clap_host.h"

int main(void) {
    printf("Testing CLAP plugin discovery...\n");

    clap_host_list_t list = {0};
    int rc = clap_scan_plugins("tests/fixtures/clap", &list);

    printf("Scan returned: %d, count: %d\n", rc, list.count);

    assert(rc == 0);
    assert(list.count == 2);

    // Print discovered plugins
    for (int i = 0; i < list.count; i++) {
        printf("Plugin %d: %s (audio_in=%d, audio_out=%d, midi_in=%d, midi_out=%d)\n",
               i, list.items[i].name,
               list.items[i].has_audio_in,
               list.items[i].has_audio_out,
               list.items[i].has_midi_in,
               list.items[i].has_midi_out);
    }

    // test_synth.clap should have audio out but no audio in (synth)
    assert(list.items[0].has_audio_out == 1);

    // test_fx.clap should have audio in (effect)
    assert(list.items[1].has_audio_in == 1);

    clap_free_plugin_list(&list);

    printf("All tests passed!\n");
    return 0;
}
