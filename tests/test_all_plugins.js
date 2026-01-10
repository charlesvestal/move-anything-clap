/*
 * Test loading each plugin from clap-plugins.clap
 */

let phase = 0;
let tick_count = 0;
let loaded = false;
let test_index = 0;
let results = [];

function display(y, text) {
    print(2, y, text, 1);
}

function init() {
    console.log("=== Testing All Plugins ===");
    phase = 0;
    tick_count = 0;
    clear_screen();
    display(2, "Testing Plugins...");
}

function tick() {
    tick_count++;

    if (phase === 0 && tick_count === 5 && !loaded) {
        loaded = true;
        const mods = host_list_modules();
        for (let i = 0; i < mods.length; i++) {
            if (mods[i].id === 'clap') {
                host_load_module(i);
                phase = 1;
                tick_count = 0;
                return;
            }
        }
    }

    if (phase === 1 && tick_count === 60) {
        const count = parseInt(host_module_get_param("plugin_count")) || 0;
        console.log("Total plugins: " + count);
        test_index = 2; // Start at 2 (skip simple_synth and test_synth)
        phase = 2;
        tick_count = 0;
    }

    if (phase === 2 && tick_count === 1) {
        const count = parseInt(host_module_get_param("plugin_count")) || 0;

        if (test_index >= count) {
            // Done testing
            console.log("=== Results ===");
            for (let r of results) {
                console.log(r);
            }
            clear_screen();
            display(2, "Test Complete");
            display(20, "See log for results");
            phase = 99;
            return;
        }

        const name = host_module_get_param("plugin_name_" + test_index);
        console.log("Testing " + test_index + ": " + name);
        display(14, "Testing: " + test_index);
        display(26, name ? name.substring(0, 16) : "?");

        // Try to load
        host_module_set_param("selected_plugin", String(test_index));
        phase = 3;
        tick_count = 0;
    }

    if (phase === 3 && tick_count === 60) {
        // Check if it loaded successfully by getting param count
        const paramCount = parseInt(host_module_get_param("param_count"));
        const name = host_module_get_param("plugin_name_" + test_index);

        if (paramCount >= 0) {
            results.push(test_index + ": " + name + " - OK (" + paramCount + " params)");
            console.log("  OK - " + paramCount + " params");
        } else {
            results.push(test_index + ": " + name + " - FAILED");
            console.log("  FAILED");
        }

        test_index++;
        phase = 2;
        tick_count = 0;
    }
}

function onMidiMessageInternal(msg) {}
function onMidiMessageExternal(msg) {}

globalThis.init = init;
globalThis.tick = tick;
globalThis.onMidiMessageInternal = onMidiMessageInternal;
globalThis.onMidiMessageExternal = onMidiMessageExternal;
