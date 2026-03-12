/*
 * Test module grouping - groups plugins by .clap file (bank)
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dsp/clap_host.h"

/* Test with mock data (no actual .clap files needed) */
static void test_mock_grouping(void) {
    printf("Testing module grouping with mock data...\n");

    /* Simulate a scan result: 3 plugins from airwindows, 1 from chowtape */
    clap_host_list_t list = {0};
    list.items = (clap_plugin_info_t *)calloc(4, sizeof(clap_plugin_info_t));
    list.capacity = 4;
    list.count = 4;

    /* Airwindows plugins */
    strcpy(list.items[0].name, "Density");
    strcpy(list.items[0].path, "/plugins/Airwindows.clap");
    strcpy(list.items[0].category, "Dynamics");
    list.items[0].plugin_index = 0;

    strcpy(list.items[1].name, "PurestDrive");
    strcpy(list.items[1].path, "/plugins/Airwindows.clap");
    strcpy(list.items[1].category, "Saturation");
    list.items[1].plugin_index = 1;

    strcpy(list.items[2].name, "Verbity");
    strcpy(list.items[2].path, "/plugins/Airwindows.clap");
    strcpy(list.items[2].category, "Reverb");
    list.items[2].plugin_index = 2;

    /* CHOWTape plugin */
    strcpy(list.items[3].name, "CHOWTapeModel");
    strcpy(list.items[3].path, "/plugins/CHOWTapeModel.clap");
    strcpy(list.items[3].category, "Unsorted");
    list.items[3].plugin_index = 0;

    /* Build module list */
    clap_module_list_t modules;
    int rc = clap_build_module_list(&list, &modules);
    assert(rc == 0);

    printf("  Found %d modules\n", modules.count);
    assert(modules.count == 2);

    /* Module 0: Airwindows */
    printf("  Module 0: %s (plugins=%d, first=%d, airwindows=%d)\n",
           modules.items[0].name, modules.items[0].plugin_count,
           modules.items[0].first_plugin, modules.items[0].is_airwindows);
    assert(strcmp(modules.items[0].name, "Airwindows") == 0);
    assert(modules.items[0].plugin_count == 3);
    assert(modules.items[0].first_plugin == 0);
    assert(modules.items[0].is_airwindows == true);

    /* Module 1: CHOWTapeModel */
    printf("  Module 1: %s (plugins=%d, first=%d, airwindows=%d)\n",
           modules.items[1].name, modules.items[1].plugin_count,
           modules.items[1].first_plugin, modules.items[1].is_airwindows);
    assert(strcmp(modules.items[1].name, "CHOWTapeModel") == 0);
    assert(modules.items[1].plugin_count == 1);
    assert(modules.items[1].first_plugin == 3);
    assert(modules.items[1].is_airwindows == false);

    clap_free_plugin_list(&list);
    printf("  PASSED\n");
}

/* Test edge cases */
static void test_empty_list(void) {
    printf("Testing empty plugin list...\n");

    clap_host_list_t list = {0};
    clap_module_list_t modules;
    int rc = clap_build_module_list(&list, &modules);
    assert(rc == 0);
    assert(modules.count == 0);

    printf("  PASSED\n");
}

static void test_single_plugin(void) {
    printf("Testing single plugin...\n");

    clap_host_list_t list = {0};
    list.items = (clap_plugin_info_t *)calloc(1, sizeof(clap_plugin_info_t));
    list.capacity = 1;
    list.count = 1;

    strcpy(list.items[0].name, "talchorus");
    strcpy(list.items[0].path, "/plugins/talchorus.clap");
    list.items[0].plugin_index = 0;

    clap_module_list_t modules;
    int rc = clap_build_module_list(&list, &modules);
    assert(rc == 0);
    assert(modules.count == 1);
    assert(strcmp(modules.items[0].name, "talchorus") == 0);
    assert(modules.items[0].plugin_count == 1);
    assert(modules.items[0].is_airwindows == false);

    clap_free_plugin_list(&list);
    printf("  PASSED\n");
}

int main(void) {
    printf("=== Module Grouping Tests ===\n");

    test_mock_grouping();
    test_empty_list();
    test_single_plugin();

    printf("\nAll tests passed!\n");
    return 0;
}
