// CLAP Host UI - placeholder
// Will be implemented in Task 4

import { CC } from "/shared/constants.mjs";

let plugins = [];
let selectedIndex = 0;

export function init() {
    print("CLAP Host Module");
    print("Loading plugins...");
    refresh();
}

function refresh() {
    // Get plugin count from DSP
    const countStr = host_module_get_param("plugin_count");
    const count = parseInt(countStr) || 0;

    plugins = [];
    for (let i = 0; i < count; i++) {
        const name = host_module_get_param(`plugin_name_${i}`);
        plugins.push(name || `Plugin ${i}`);
    }

    render();
}

function render() {
    clear_screen();
    print("CLAP Host");
    print("");

    if (plugins.length === 0) {
        print("No plugins found");
        print("Add .clap files to plugins/");
    } else {
        const name = plugins[selectedIndex] || "None";
        print(`Plugin: ${name}`);
        print(`[${selectedIndex + 1}/${plugins.length}]`);
    }
}

export function tick() {
    // Periodic update if needed
}

export function onMidiMessage(msg, source) {
    const status = msg[0] & 0xF0;
    const cc = msg[1];
    const val = msg[2];

    // CC 14 = jog wheel for plugin selection
    if (status === 0xB0 && cc === CC.JOG) {
        if (val === 1) {
            // Right
            selectedIndex = Math.min(selectedIndex + 1, plugins.length - 1);
        } else if (val === 127) {
            // Left
            selectedIndex = Math.max(selectedIndex - 1, 0);
        }

        if (plugins.length > 0) {
            host_module_set_param("selected_plugin", String(selectedIndex));
        }
        render();
    }
}
