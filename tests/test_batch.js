/*
 * Batch test plugins - test indices 3, 5, 7, 8, 9, 10
 * Skip 2 (ADSR crashes), 4 (works), 6 (works)
 */

let phase = 0;
let tick_count = 0;
let loaded = false;
let test_indices = [2, 3, 11, 14, 16, 17, 18, 19]; // Test previously crashing plugins
let current_test = 0;

function display(y, text) {
    print(2, y, text, 1);
}

function init() {
    console.log("=== Batch Plugin Test ===");
    phase = 0;
    tick_count = 0;
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
        phase = 2;
        tick_count = 0;
    }

    if (phase === 2 && tick_count === 1) {
        if (current_test >= test_indices.length) {
            console.log("=== All tests complete ===");
            clear_screen();
            display(2, "Done!");
            phase = 99;
            return;
        }

        const idx = test_indices[current_test];
        const name = host_module_get_param("plugin_name_" + idx);
        console.log("Testing " + idx + ": " + name + "...");
        host_module_set_param("selected_plugin", String(idx));
        phase = 3;
        tick_count = 0;
    }

    if (phase === 3 && tick_count === 30) {
        const idx = test_indices[current_test];
        const paramCount = parseInt(host_module_get_param("param_count"));
        console.log("  -> " + idx + " OK (" + paramCount + " params)");
        current_test++;
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
