/*
 * Test specific plugin indices
 */

let phase = 0;
let tick_count = 0;
let loaded = false;

// Test these indices one at a time
const TEST_INDEX = 4; // DC Offset

function display(y, text) {
    print(2, y, text, 1);
}

function init() {
    console.log("=== Testing Plugin " + TEST_INDEX + " ===");
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
        const name = host_module_get_param("plugin_name_" + TEST_INDEX);
        console.log("Loading plugin " + TEST_INDEX + ": " + name);
        host_module_set_param("selected_plugin", String(TEST_INDEX));
        phase = 2;
        tick_count = 0;
    }

    if (phase === 2 && tick_count === 60) {
        const paramCount = parseInt(host_module_get_param("param_count"));
        console.log("=== LOADED OK - " + paramCount + " params ===");
        clear_screen();
        display(2, "Plugin " + TEST_INDEX);
        display(20, "Params: " + paramCount);
        display(40, "SUCCESS!");
        phase = 99;
    }
}

function onMidiMessageInternal(msg) {}
function onMidiMessageExternal(msg) {}

globalThis.init = init;
globalThis.tick = tick;
globalThis.onMidiMessageInternal = onMidiMessageInternal;
globalThis.onMidiMessageExternal = onMidiMessageExternal;
