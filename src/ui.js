// CLAP Host UI for Move Anything
//
// Provides plugin browser and parameter control interface.

import { CC, BUTTON } from "/shared/constants.mjs";

// State
let plugins = [];
let selectedIndex = 0;
let paramBank = 0;
let paramCount = 0;
let octaveTranspose = 0;

// Constants
const PARAMS_PER_BANK = 8;
const SCREEN_WIDTH = 128;
const SCREEN_HEIGHT = 64;

export function init() {
    refresh();
    render();
}

function refresh() {
    // Get plugin count
    const countStr = host_module_get_param("plugin_count");
    const count = parseInt(countStr) || 0;

    plugins = [];
    for (let i = 0; i < count; i++) {
        const name = host_module_get_param(`plugin_name_${i}`);
        plugins.push(name || `Plugin ${i}`);
    }

    // Get current selection
    const selStr = host_module_get_param("selected_plugin");
    selectedIndex = parseInt(selStr) || 0;

    // Get parameter count
    const paramStr = host_module_get_param("param_count");
    paramCount = parseInt(paramStr) || 0;

    // Get octave transpose
    const octStr = host_module_get_param("octave_transpose");
    octaveTranspose = parseInt(octStr) || 0;
}

function render() {
    clear_screen();

    // Title bar
    print("CLAP Host");

    if (plugins.length === 0) {
        print("");
        print("No plugins found");
        print("Add .clap files to:");
        print("  plugins/");
        return;
    }

    // Current plugin
    const name = plugins[selectedIndex] || "None";
    const shortName = name.length > 18 ? name.substring(0, 15) + "..." : name;
    print(`> ${shortName}`);

    // Plugin selector hint
    print(`[${selectedIndex + 1}/${plugins.length}] Oct:${octaveTranspose >= 0 ? '+' : ''}${octaveTranspose}`);

    // Parameters (show current bank)
    if (paramCount > 0) {
        const bankStart = paramBank * PARAMS_PER_BANK;
        const bankEnd = Math.min(bankStart + PARAMS_PER_BANK, paramCount);
        const bankNum = Math.floor(paramBank) + 1;
        const totalBanks = Math.ceil(paramCount / PARAMS_PER_BANK);

        print(`Params ${bankNum}/${totalBanks}:`);

        // Show up to 4 params with names
        for (let i = bankStart; i < Math.min(bankStart + 4, bankEnd); i++) {
            const pname = host_module_get_param(`param_name_${i}`) || `P${i}`;
            const pval = host_module_get_param(`param_value_${i}`) || "0";
            const shortPname = pname.length > 8 ? pname.substring(0, 7) : pname;
            print(`${shortPname}: ${pval}`);
        }
    } else {
        print("");
        print("No parameters");
    }
}

export function tick() {
    // Could refresh periodically if needed
}

export function onMidiMessage(msg, source) {
    const status = msg[0] & 0xF0;
    const channel = msg[0] & 0x0F;
    const cc = msg[1];
    const val = msg[2];

    // Handle control changes
    if (status === 0xB0) {
        // Jog wheel (CC 14) - plugin selection
        if (cc === CC.JOG) {
            if (val === 1) {
                // Right - next plugin
                if (selectedIndex < plugins.length - 1) {
                    selectedIndex++;
                    host_module_set_param("selected_plugin", String(selectedIndex));
                    refresh();
                    render();
                }
            } else if (val === 127) {
                // Left - previous plugin
                if (selectedIndex > 0) {
                    selectedIndex--;
                    host_module_set_param("selected_plugin", String(selectedIndex));
                    refresh();
                    render();
                }
            }
        }
        // Left button (CC 44) - previous param bank
        else if (cc === 44 && val > 0) {
            if (paramBank > 0) {
                paramBank--;
                host_module_set_param("param_bank", String(paramBank));
                render();
            }
        }
        // Right button (CC 45) - next param bank
        else if (cc === 45 && val > 0) {
            const totalBanks = Math.ceil(paramCount / PARAMS_PER_BANK);
            if (paramBank < totalBanks - 1) {
                paramBank++;
                host_module_set_param("param_bank", String(paramBank));
                render();
            }
        }
        // Up button (CC 46) - octave up
        else if (cc === 46 && val > 0) {
            if (octaveTranspose < 2) {
                octaveTranspose++;
                host_module_set_param("octave_transpose", String(octaveTranspose));
                render();
            }
        }
        // Down button (CC 47) - octave down
        else if (cc === 47 && val > 0) {
            if (octaveTranspose > -2) {
                octaveTranspose--;
                host_module_set_param("octave_transpose", String(octaveTranspose));
                render();
            }
        }
        // Encoders (CC 71-78) - parameter control
        else if (cc >= 71 && cc <= 78) {
            const encoderIdx = cc - 71;
            const paramIdx = paramBank * PARAMS_PER_BANK + encoderIdx;

            if (paramIdx < paramCount) {
                // Get current value and adjust
                const currentStr = host_module_get_param(`param_value_${paramIdx}`) || "0";
                let current = parseFloat(currentStr);

                // Relative change based on encoder direction
                const delta = val < 64 ? val * 0.01 : (val - 128) * 0.01;
                current += delta;

                // Clamp (basic, plugin may have different ranges)
                if (current < 0) current = 0;
                if (current > 1) current = 1;

                host_module_set_param(`param_${paramIdx}`, String(current));
                render();
            }
        }
    }

    // Pass through to DSP for note/other handling
    host_module_send_midi(msg, source);
}
