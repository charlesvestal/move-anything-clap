// CLAP Host UI for Move Anything
//
// Provides plugin browser and parameter control interface.

import {
    MoveMainKnob,
    MoveLeft, MoveRight, MoveUp, MoveDown
} from "../../shared/constants.mjs";

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
const LINE_HEIGHT = 10;

// CC numbers
const CC_JOG = MoveMainKnob;  // 14
const CC_LEFT = MoveLeft;     // 44
const CC_RIGHT = MoveRight;   // 45
const CC_UP = MoveUp;         // 46
const CC_DOWN = MoveDown;     // 47

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
    let y = 2;

    // Title bar
    print(2, y, "CLAP Host", 1);
    y += LINE_HEIGHT;

    if (plugins.length === 0) {
        y += LINE_HEIGHT;
        print(2, y, "No plugins found", 1);
        y += LINE_HEIGHT;
        print(2, y, "Add .clap files to:", 1);
        y += LINE_HEIGHT;
        print(2, y, "  plugins/", 1);
        return;
    }

    // Current plugin
    const name = plugins[selectedIndex] || "None";
    const shortName = name.length > 18 ? name.substring(0, 15) + "..." : name;
    print(2, y, "> " + shortName, 1);
    y += LINE_HEIGHT;

    // Plugin selector hint
    const octStr = octaveTranspose >= 0 ? "+" + octaveTranspose : String(octaveTranspose);
    print(2, y, "[" + (selectedIndex + 1) + "/" + plugins.length + "] Oct:" + octStr, 1);
    y += LINE_HEIGHT;

    // Parameters (show current bank)
    if (paramCount > 0) {
        const bankStart = paramBank * PARAMS_PER_BANK;
        const bankEnd = Math.min(bankStart + PARAMS_PER_BANK, paramCount);
        const bankNum = Math.floor(paramBank) + 1;
        const totalBanks = Math.ceil(paramCount / PARAMS_PER_BANK);

        print(2, y, "Params " + bankNum + "/" + totalBanks + ":", 1);
        y += LINE_HEIGHT;

        // Show up to 3 params with names (limited screen space)
        for (let i = bankStart; i < Math.min(bankStart + 3, bankEnd); i++) {
            const pname = host_module_get_param("param_name_" + i) || "P" + i;
            const pval = host_module_get_param("param_value_" + i) || "0";
            const shortPname = pname.length > 8 ? pname.substring(0, 7) : pname;
            print(2, y, shortPname + ": " + pval, 1);
            y += LINE_HEIGHT;
        }
    } else {
        y += LINE_HEIGHT;
        print(2, y, "No parameters", 1);
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
        if (cc === CC_JOG) {
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
        // Left button - previous param bank
        else if (cc === CC_LEFT && val > 0) {
            if (paramBank > 0) {
                paramBank--;
                host_module_set_param("param_bank", String(paramBank));
                render();
            }
        }
        // Right button - next param bank
        else if (cc === CC_RIGHT && val > 0) {
            const totalBanks = Math.ceil(paramCount / PARAMS_PER_BANK);
            if (paramBank < totalBanks - 1) {
                paramBank++;
                host_module_set_param("param_bank", String(paramBank));
                render();
            }
        }
        // Up button - octave up
        else if (cc === CC_UP && val > 0) {
            if (octaveTranspose < 2) {
                octaveTranspose++;
                host_module_set_param("octave_transpose", String(octaveTranspose));
                render();
            }
        }
        // Down button - octave down
        else if (cc === CC_DOWN && val > 0) {
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
