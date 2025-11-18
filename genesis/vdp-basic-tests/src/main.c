#include "md.h"

static const u8 MENU_HINT_BASICS = 1;
static const u8 MENU_VDP_INT_DELAY = MENU_HINT_BASICS + 1;
static const u8 MENU_SAT_CACHE = MENU_VDP_INT_DELAY + 1;
static const u8 MENU_SAT_MASK_H40 = MENU_SAT_CACHE + 1;
static const u8 MENU_SAT_MASK_H32 = MENU_SAT_MASK_H40 + 1;
static const u8 MENU_SPRITE_Y9_PROG = MENU_SAT_MASK_H32 + 1;
static const u8 MENU_SPRITE_Y9_INTERLACED = MENU_SPRITE_Y9_PROG + 1;
static const u8 MENU_WINDOW_MASK_H40 = MENU_SPRITE_Y9_INTERLACED + 1;
static const u8 MENU_WINDOW_MASK_H32 = MENU_WINDOW_MASK_H40 + 1;
static const u8 MENU_FILL_INCREMENT2 = MENU_WINDOW_MASK_H32 + 1;
static const u8 MENU_REGISTER_CODE_BITS = MENU_FILL_INCREMENT2 + 1;
static const u8 MENU_MOVEM_L_PREDEC = MENU_REGISTER_CODE_BITS + 1;
static const u8 MENU_HBLANK_NT_UPDATE = MENU_MOVEM_L_PREDEC + 1;
static const u8 MENU_HBLANK_FULL_VS_UPDATE = MENU_HBLANK_NT_UPDATE + 1;
// No test on updating per-column VSRAM during HBlank because it's very timing-sensitive
static const u8 MENU_DATA_PORT_LATCH_RESET = MENU_HBLANK_FULL_VS_UPDATE + 1;

static const u8 MENU_OPTIONS = MENU_DATA_PORT_LATCH_RESET;

static vu8 cursor_pos = 1;
static vu8 selected_option = 0;
static vbool exit_test = false;

static u8 last_inputs = 0;

u8 update_inputs() {
    u8 inputs = read_p1_inputs();
    u8 pressed = inputs & ~last_inputs;
    last_inputs = inputs;
    return pressed;
}

void menu_vint_handler() {
    u8 pressed = update_inputs();

    if (pressed & JOYPAD_DOWN) {
        cursor_pos = cursor_pos < MENU_OPTIONS ? cursor_pos + 1 : 1;
        vdp_write_vram(SAT_ADDR, 0x0088 + (cursor_pos << 3));
    }

    if (pressed & JOYPAD_UP) {
        cursor_pos = cursor_pos > 1 ? cursor_pos - 1 : MENU_OPTIONS;
        vdp_write_vram(SAT_ADDR, 0x0088 + (cursor_pos << 3));
    }

    if (pressed & (JOYPAD_A | JOYPAD_C | JOYPAD_START)) {
        selected_option = cursor_pos;
    }
}

void render_menu() {
    static const u16 palettes[] = {
        0x000,0xC22,0xEEE,0xEEE,0xEEE,0xEEE,0xEEE,0xEEE,
        0xEEE,0xEEE,0xEEE,0xEEE,0xEEE,0xEEE,0xEEE,0xEEE
    };

    vdp_dma_cram((u32)palettes, 0, 16);

    u8 x = 3;
    u8 y = 2;
    puts("HINT basics", PLANE_A_ADDR, x, y++, 0);
    puts("VDP interrupt enable delay", PLANE_A_ADDR, x, y++, 0);
    puts("Sprite table cache", PLANE_A_ADDR, x, y++, 0);
    puts("Sprite table address mask H40", PLANE_A_ADDR, x, y++, 0);
    puts("Sprite table address mask H32", PLANE_A_ADDR, x, y++, 0);
    puts("Sprite Y mask non-interlaced", PLANE_A_ADDR, x, y++, 0);
    puts("Sprite Y mask 2-screen interlaced", PLANE_A_ADDR, x, y++, 0);
    puts("Window nametable address mask H40", PLANE_A_ADDR, x, y++, 0);
    puts("Window nametable address mask H32", PLANE_A_ADDR, x, y++, 0);
    puts("VRAM fill w/ data port increment 2", PLANE_A_ADDR, x, y++, 0);
    puts("Register writes change code bits", PLANE_A_ADDR, x, y++, 0);
    puts("MOVEM.L pre-dec into VDP ports", PLANE_A_ADDR, x, y++, 0);
    puts("HBlank nametable address updates", PLANE_A_ADDR, x, y++, 0);
    puts("HBlank full screen VSRAM updates", PLANE_A_ADDR, x, y++, 0);
    puts("Data writes reset command latch", PLANE_A_ADDR, x, y++, 0);

    selected_option = 0;

    Sprite cursor_sprite = {
        .tile = font_tile_number('>'),
        .x = 0x0088,
        .y = 0x0088 + (cursor_pos << 3)
    };
    vdp_write_sprite(SAT_ADDR, 0, &cursor_sprite);

    vdp_enable_display_vint(menu_vint_handler);

    while(!selected_option);
}

void write_test_palettes() {
    // Palette 0 green, palette 1 red
    static const u16 palettes[] = {
        0x000,0x060,0x0E0,0x0E0,0x0E0,0x0E0,0x0E0,0x0E0,
        0x0E0,0x0E0,0x0E0,0x0E0,0x0E0,0x0E0,0x0E0,0x0E0,
        0x000,0x006,0x00E,0x00E,0x00E,0x00E,0x00E,0x00E,
        0x00E,0x00E,0x00E,0x00E,0x00E,0x00E,0x00E,0x00E
    };

    vdp_dma_cram((u32)palettes, 0, 32);
}

void test_vint_handler() {
    u8 pressed = update_inputs();
    exit_test |= pressed & JOYPAD_B;
}

static const u8 HINT_INTERVAL_VALUES[] = {5, 10, 37, 0, 0, 161, 0, 0};
static const u8 HINT_EXPECTED_LINES[] = {0, 5, 11, 22, 60, 61, 62, 224};
static const u8 HINT_ARR_LEN = 8;
static vu16 hint_trigger_count = 0;
static vu8 hint_trigger_lines[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

void test_hint_basics_handler() {
    hint_trigger_count++;
    if (hint_trigger_count > HINT_ARR_LEN) return;
    hint_trigger_lines[hint_trigger_count] = *VDP_V_COUNTER;
    if (hint_trigger_count >= HINT_ARR_LEN) return;
    vdp_write_register(VDP_REG_HINT_INTERVAL, HINT_INTERVAL_VALUES[hint_trigger_count]);
}

u8 itoa(u8 value, char* c) {
    u8 i = 0;
    if (value / 100) {
        c[i++] = value / 100 + '0';
        value %= 100;
    }

    if ((value / 10) || i != 0) {
        c[i++] = value / 10 + '0';
        value %= 10;
    }

    c[i++] = value + '0';
    c[i++] = 0;

    return i;
}

void test_hint_basics() {
    char s[320];

    write_test_palettes();
    vdp_write_cram((32 + 1) << 1, 0xA6A);
    vdp_write_cram((32 + 15) << 1, 0xEEE);

    vdp_write_register(VDP_REG_MODE2, 0x54); // Enable display but not VINT

    vdp_write_register(VDP_REG_HINT_INTERVAL, HINT_INTERVAL_VALUES[0]);
    hint_trigger_count = 0;
    for (u16 i = 0; i < HINT_ARR_LEN + 1; i++) hint_trigger_lines[i] = 255;

    while (*VDP_CTRL & 8); // Wait for VBlank to end
    vdp_wait_for_vblank();

    // Enable HINTs for a single frame
    vdp_enable_hint(test_hint_basics_handler);

    while (*VDP_CTRL & 8); // Wait for VBlank to end
    vdp_wait_for_vblank();

    vdp_disable_hint();

    bool count_match = hint_trigger_count == HINT_ARR_LEN - 1;
    bool lines_match[HINT_ARR_LEN];
    for (u16 i = 1; i < HINT_ARR_LEN; i++) lines_match[i] = hint_trigger_lines[i] == HINT_EXPECTED_LINES[i];

    bool all_lines_match = true;
    for (u16 i = 1; i < HINT_ARR_LEN; i++) all_lines_match &= lines_match[i];

    puts("HINT trigger count: ", PLANE_A_ADDR, 2, 2, 2);
    puts(count_match ? "PASS" : "FAIL", PLANE_A_ADDR, 22, 2, count_match ? 0 : 1);
    
    puts("Expected:", PLANE_A_ADDR, 4, 3, 2);
    itoa(HINT_ARR_LEN - 1, s);
    puts(s, PLANE_A_ADDR, 14, 3, 2);

    puts("Actual:", PLANE_A_ADDR, 4, 4, 2);
    itoa(hint_trigger_count, s);
    puts(s, PLANE_A_ADDR, 12, 4, count_match ? 0 : 1);

    puts("HINT trigger lines: ", PLANE_A_ADDR, 2, 5, 2);
    puts(all_lines_match ? "PASS" : "FAIL", PLANE_A_ADDR, 22, 5, all_lines_match ? 0 : 1);

    u8 si = itoa(HINT_EXPECTED_LINES[1], s);
    for (u16 i = 2; i < HINT_ARR_LEN; i++) {
        s[si - 1] = ' ';
        si += itoa(HINT_EXPECTED_LINES[i], &s[si]);
    }
    puts("Expected:", PLANE_A_ADDR, 4, 6, 2);
    puts(s, PLANE_A_ADDR, 6, 7, 2);

    si = itoa(hint_trigger_lines[1], s);
    for (u16 i = 2; i < HINT_ARR_LEN + 1; i++) {
        if (hint_trigger_lines[i] != 255) {
            s[si - 1] = ' ';
            si += itoa(hint_trigger_lines[i], &s[si]);
        }
    }
    puts("Actual:", PLANE_A_ADDR, 4, 8, 2);
    puts(s, PLANE_A_ADDR, 6, 9, all_lines_match ? 0 : 1);

    vdp_enable_display_vint(test_vint_handler);

    while (!exit_test);
}

static vu8 int_delay_hint_count = 0;
static vbool int_delay_vint = false;

void int_delay_hint_handler() {
    int_delay_hint_count++;
}

void int_delay_vint_handler() {
    int_delay_vint = true;
}

void test_vdp_interrupt_delay() {
    write_test_palettes();
    vdp_write_cram(66, 0xA4A);
    vdp_write_cram(94, 0xEEE);

    // Disable VINT and HINT, enable display, set HINT interval to 0 to guarantee that it's pending
    vdp_write_register(VDP_REG_MODE1, 0x04);
    vdp_write_register(VDP_REG_MODE2, 0x54);
    vdp_write_register(VDP_REG_HINT_INTERVAL, 0);

    // Wait until the next VBlank period
    while (*VDP_CTRL & 8);
    vdp_wait_for_vblank();

    // Beginning of VBlank; VINT and HINT should both be pending at this point
    vdp_write_register(VDP_REG_HINT_INTERVAL, 255);

    v_vint_vector = (u32)int_delay_vint_handler;
    v_hint_vector = (u32)int_delay_hint_handler;

    int_delay_hint_count = 0;
    int_delay_vint = false;

    ENABLE_INTS;

    // Enable HINTs then VINTs; should handle HINT twice
    __asm__(
        "move.w #0x8014, (0xC00004)\n"
        "move.w #0x8174, (0xC00004)\n"
    );

    bool hint_count_pass = int_delay_hint_count == 2;
    puts("HINT handled twice:", PLANE_A_ADDR, 3, 3, 2);
    puts(hint_count_pass ? "PASS" : "FAIL", PLANE_A_ADDR, 23, 3, hint_count_pass ? 0 : 1);

    bool vint_pass = !int_delay_vint;
    puts("VINT not handled:", PLANE_A_ADDR, 3, 4, 2);
    puts(vint_pass ? "PASS" : "FAIL", PLANE_A_ADDR, 21, 4, vint_pass ? 0 : 1);

    vdp_disable_hint();
    vdp_enable_display_vint(test_vint_handler);

    while (!exit_test);
}

void test_sprite_table_cache() {
    write_test_palettes();

    vdp_vram_copy(FONT_TILE_ADDR | (('P' - 32) << 5), 0x1000, 32);
    vdp_vram_copy(FONT_TILE_ADDR | (('A' - 32) << 5), 0x1020, 32);
    vdp_vram_copy(FONT_TILE_ADDR | (('S' - 32) << 5), 0x1040, 32);
    vdp_vram_copy(FONT_TILE_ADDR | (('S' - 32) << 5), 0x1060, 32);

    vdp_vram_copy(FONT_TILE_ADDR | (('F' - 32) << 5), 0x1080, 32);
    vdp_vram_copy(FONT_TILE_ADDR | (('A' - 32) << 5), 0x10A0, 32);
    vdp_vram_copy(FONT_TILE_ADDR | (('I' - 32) << 5), 0x10C0, 32);
    vdp_vram_copy(FONT_TILE_ADDR | (('L' - 32) << 5), 0x10E0, 32);

    // FAIL sprite; should use offscreen Y value and link 1 from SAT cache
    Sprite sprite = {};
    sprite.tile = 0x1080 >> 5;
    sprite.x = 0x0080 + (18 << 3);
    sprite.y = 0x0080 + 256;
    sprite.h_cells = 3;
    sprite.palette = 1;
    sprite.link = 1;
    vdp_write_sprite(0xFC00, 0, &sprite);

    sprite.y = 0x0080 + (13 << 3);
    sprite.link = 0;
    vdp_write_sprite(0xF000, 0, &sprite);

    // PASS sprite; should use onscreen Y value and 4-cell size from SAT cache 
    sprite.tile = 0;
    sprite.x = 0x0080 + (18 << 3);
    sprite.y = 0x0080 + (14 << 3);
    sprite.h_cells = 3;
    sprite.palette = 0;
    sprite.link = 0;
    vdp_write_sprite(0xFC00, 1, &sprite);

    sprite.tile = 0x1000 >> 5;
    sprite.y = 0x0080 + 256;
    sprite.h_cells = 0;
    vdp_write_sprite(0xF000, 1, &sprite);

    vdp_write_register(VDP_REG_SAT_ADDR, 0xF000 >> 9);

    vdp_enable_display_vint(test_vint_handler);

    while(!exit_test);
}

void test_sprite_table_mask(bool h40) {
    write_test_palettes();
    vdp_write_cram(4, 0);

    vdp_vram_fill(32, 0x22, 4 * 32);
    vdp_vram_fill(SAT_ADDR, 0, 0x400);

    puts("FAIL", PLANE_A_ADDR, 18, 13, 1);

    vdp_write_register(VDP_REG_MODE4, h40 | (h40 << 7));

    vdp_write_register(VDP_REG_SAT_ADDR, 0x7F);

    u16 sat_addr = h40 ? 0xFC00 : 0xFE00;

    Sprite sprite = {
        .tile = font_tile_number('P'),
        .x = 0x0080 + (18 << 3),
        .y = 0x0080 + (13 << 3),
        .link = 1
    };
    vdp_write_sprite(sat_addr, 0, &sprite);

    sprite.tile = font_tile_number('A');
    sprite.x += 8;
    sprite.link += 1;
    vdp_write_sprite(sat_addr, 1, &sprite);

    sprite.tile = font_tile_number('S');
    sprite.x += 8;
    sprite.link += 1;
    vdp_write_sprite(sat_addr, 2, &sprite);

    sprite.x += 8;
    sprite.link += 1;
    vdp_write_sprite(sat_addr, 3, &sprite);

    sprite.tile = 1;
    sprite.x = 0x0080 + (18 << 3);
    sprite.h_cells = 3;
    sprite.link = 0;
    vdp_write_sprite(sat_addr, 4, &sprite);

    vdp_enable_display_vint(test_vint_handler);

    while(!exit_test);
}

void test_sprite_y9(bool progressive) {
    write_test_palettes();
    vdp_write_cram(4, 0);

    vdp_vram_copy(FONT_TILE_ADDR | (('P' - 32) << 5), 0x1000, 32);
    vdp_vram_copy(FONT_TILE_ADDR | (('A' - 32) << 5), 0x1000 | (0x20 << !progressive), 32);
    vdp_vram_copy(FONT_TILE_ADDR | (('S' - 32) << 5), 0x1000 | (0x40 << !progressive), 32);
    vdp_vram_copy(FONT_TILE_ADDR | (('S' - 32) << 5), 0x1000 | (0x60 << !progressive), 32);
    
    vdp_vram_copy(FONT_TILE_ADDR | (('F' - 32) << 5), 0x1000 | (0x80 << !progressive), 32);
    vdp_vram_copy(FONT_TILE_ADDR | (('A' - 32) << 5), 0x1000 | (0xA0 << !progressive), 32);
    vdp_vram_copy(FONT_TILE_ADDR | (('I' - 32) << 5), 0x1000 | (0xC0 << !progressive), 32);
    vdp_vram_copy(FONT_TILE_ADDR | (('L' - 32) << 5), 0x1000 | (0xE0 << !progressive), 32);

    vdp_vram_fill(0x1200, 0x22, (4 * 32) << !progressive);

    Sprite sprite = {};

    // PASS sprite
    sprite.tile = (0x1000 >> 5) >> !progressive;
    sprite.x = 0x0080 + (18 << 3);
    sprite.y = progressive ? 0xFE00 | (0x0080 + (13 << 3)) : 0xFC00 | (0x0100 + (13 << 4));
    sprite.h_cells = 3;
    sprite.palette = 0;
    sprite.link = 1;
    vdp_write_sprite(SAT_ADDR, 0, &sprite);

    // Solid black sprite
    sprite.tile = (0x1200 >> 5) >> !progressive;
    sprite.link += 1;
    vdp_write_sprite(SAT_ADDR, 1, &sprite);

    // FAIL sprite; should be hidden behind other sprites
    sprite.tile = ((0x1000 >> 5) >> !progressive) | (0x80 >> 5);
    sprite.y &= progressive ? 0x01FF : 0x03FF;
    sprite.palette = 1;
    sprite.link = 0;
    vdp_write_sprite(SAT_ADDR, 2, &sprite);

    if (!progressive) vdp_write_register(VDP_REG_MODE4, 0x87);

    vdp_enable_display_vint(test_vint_handler);

    while(!exit_test);
}

void test_window_nt_mask(bool h40) {
    write_test_palettes();
    vdp_write_cram(4, 0);

    vdp_vram_fill(0x2000, 0x22, 32);

    vdp_write_register(VDP_REG_MODE4, h40 | (h40 << 7));

    vdp_write_register(VDP_REG_H_SCROLL_ADDR, 0);
    vdp_write_register(VDP_REG_SAT_ADDR, 0);
    vdp_write_register(VDP_REG_WINDOW_NT, 0x3F);
    vdp_write_register(VDP_REG_WINDOW_H, 0x80);
    vdp_write_register(VDP_REG_WINDOW_V, 0x80);

    puts("FAIL", PLANE_B_ADDR, 18, 13, 1);

    u16 window_nt_addr = h40 ? 0xF000 : 0xF800;
    u16 window_fail_addr = window_nt_addr | (13 << (h40 ? 7 : 6)) | (18 << 1);
    for (u16 i = 0; i < 4; i++) {
        vdp_write_vram(window_fail_addr + (i << 1), 0x2000 >> 5);
    }

    u16 window_pass_addr = window_nt_addr | (14 << (h40 ? 7 : 6)) | (18 << 1);
    puts("PASS", window_pass_addr, 0, 0, 0);

    vdp_enable_display_vint(test_vint_handler);

    while(!exit_test);
}

void test_vram_fill_increment2() {
    write_test_palettes();

    vdp_write_register(VDP_REG_PORT_INCREMENT, 2);
    vdp_write_register(VDP_REG_DMA_LEN_L, 8);
    vdp_write_register(VDP_REG_DMA_LEN_H, 0);
    vdp_write_register(VDP_REG_DMA_SRC_H, 0x80);

    *VDP_CTRL32 = 0x60000080;
    *VDP_DATA = 0x5678;
    while (*VDP_CTRL & 2);

    static const u16 expected[10] = {
        0x5678,0x0056,0x0056,0x0056,0x0056,0x0056,0x0056,0x0056,0x0056,0x0000
    };
    *VDP_CTRL32 = 0x20000000;
    bool passed = true;
    for (u16 i = 0; i < 10; i++) passed &= *VDP_DATA == expected[i];

    puts(passed ? "PASS" : "FAIL", PLANE_A_ADDR, 18, 13, passed ? 0 : 1);

    vdp_enable_display_vint(test_vint_handler);

    while(!exit_test);
}

void prepare_vram_write(u16 address) {
    *VDP_CTRL = 0x4000 | (address & 0x3FFF);
    *VDP_CTRL = address >> 14;
}

void test_register_code_bits() {
    write_test_palettes();

    puts("PASS", PLANE_A_ADDR, 18, 13, 0);

    u16 pass_addr = PLANE_A_ADDR | (13 << 7) | (18 << 1);
    prepare_vram_write(pass_addr);

    // Register write should change target to invalid
    vdp_write_register(VDP_REG_WINDOW_NT, 0xFF);

    *VDP_DATA = font_tile_number('F') | (1 << 13);
    *VDP_DATA = font_tile_number('A') | (1 << 13);
    *VDP_DATA = font_tile_number('I') | (1 << 13);
    *VDP_DATA = font_tile_number('L') | (1 << 13);

    vdp_enable_display_vint(test_vint_handler);

    while(!exit_test);
}

void test_movem_predec() {
    write_test_palettes();

    puts("FAIL", PLANE_A_ADDR, 18, 13, 1);

    // Overwrite "FAIL" with "PASS" using MOVEM.L instructions with indirect pre-decrement addressing
    // VRAM address $A6A4 (X=18 Y=13 in nametable $A000)
    // Tile numbers 0x4B0 (P), 0x4A1 (A), 0x4B3 (S), all with palette 0
    __asm__(
        "movem.l %d0-%d1/%a0, -(%sp)\n"

        "move.l #0x000266A4, %d1\n"
        "move.l #0x04A104B0, %d0\n"
        "move.l #0xC00008, %a0\n"
        "movem.l %d0-%d1, -(%a0)\n"

        "addq.l #4, %d1\n"
        "move.l #0x04B304B3, %d0\n"
        "addq.l #8, %a0\n"
        "movem.l %d0-%d1, -(%a0)\n"

        "movem.l (%sp)+, %d0-%d1/%a0\n"
    );

    vdp_enable_display_vint(test_vint_handler);

    while(!exit_test);
}

void test_hblank_nt_update() {
    vdp_write_cram(0, 0);
    vdp_write_cram(2, 0xA4A);
    vdp_write_cram(30, 0xEEE);

    vdp_vram_fill(32, 0x11, 4);
    vdp_vram_fill(64, 0xFF, 4);

    vdp_write_register(VDP_REG_PORT_INCREMENT, 2);

    u16 plane_a_addr = 0x8000 | (6 << 7);
    prepare_vram_write(plane_a_addr);
    for (u16 i = 0; i < 20; i++) *VDP_DATA = 0x0001;

    u16 plane_b_addr = 0xC000 | (6 << 7) | (20 << 1);
    prepare_vram_write(plane_b_addr);
    for (u16 i = 0; i < 20; i++) *VDP_DATA = 0x0001;

    plane_a_addr = 0x8000 | (12 << 7);
    prepare_vram_write(plane_a_addr);
    for (u16 i = 0; i < 20; i++) *VDP_DATA = 0x0002;

    plane_b_addr = 0x6000 | (12 << 7) | (20 << 1);
    prepare_vram_write(plane_b_addr);
    for (u16 i = 0; i < 20; i++) *VDP_DATA = 0x0002;

    puts("Horizontal lines should span entire", 0xA000, 2, 2, 0);
    puts("screen", 0xA000, 2, 3, 0);

    vdp_enable_display_vint(test_vint_handler);

    while (!exit_test) {
        vdp_write_register(VDP_REG_PLANE_A_NT, (0xA000 >> 13) << 3);
        vdp_write_register(VDP_REG_PLANE_B_NT, 0xC000 >> 13);

        // Change Plane A nametable address during HBlank between lines 47 and 48
        while (*VDP_V_COUNTER != 48);
        while (*VDP_H_COUNTER < 0xC0);
        vdp_write_register(VDP_REG_PLANE_A_NT, (0x8000 >> 13) << 3);

        // Change Plane B nametable address during HBlank between lines 95 and 96
        while (*VDP_V_COUNTER != 96);
        while (*VDP_H_COUNTER < 0xC0);
        vdp_write_register(VDP_REG_PLANE_B_NT, 0x6000 >> 13);

        vdp_wait_for_vblank();
    }
}

void prepare_vsram_write(u16 addr) {
    *VDP_CTRL = 0x4000 | (addr & 0x3FFF);
    *VDP_CTRL = 0x0010 | (addr >> 14);
}

void test_hblank_vsram_update() {
    vdp_write_cram(0, 0);
    vdp_write_cram(2, 0xA4A);
    vdp_write_cram(30, 0xEEE);

    vdp_vram_fill(32, 0x11, 4);
    vdp_vram_fill(64, 0xFF, 4);

    vdp_write_register(VDP_REG_PORT_INCREMENT, 2);
    
    u16 plane_a_addr = PLANE_A_ADDR | (6 << 7);
    prepare_vram_write(plane_a_addr); 
    for (u16 i = 0; i < 20; i++) *VDP_DATA = 0x0001;
    
    u16 plane_b_addr = PLANE_B_ADDR | (6 << 7) | (20 << 1);
    prepare_vram_write(plane_b_addr);
    for (u16 i = 0; i < 20; i++) *VDP_DATA = 0x0001;
            
    plane_a_addr = PLANE_A_ADDR | (12 << 7);
    prepare_vram_write(plane_a_addr);
    for (u16 i = 0; i < 20; i++) *VDP_DATA = 0x0002;
    
    plane_b_addr = PLANE_B_ADDR | (12 << 7) | (20 << 1);
    prepare_vram_write(plane_b_addr);
    for (u16 i = 0; i < 20; i++) *VDP_DATA = 0x0002;

    puts("Horizontal lines should span entire", PLANE_A_ADDR, 2, 2, 0);
    puts("screen", PLANE_A_ADDR, 2, 3, 0);

    vdp_enable_display_vint(test_vint_handler);

    while (!exit_test) {
        // Set both V scroll values to 0
        prepare_vsram_write(0);
        *VDP_DATA = 0;
        *VDP_DATA = 0;

        // Change Plane A's V scroll to 1 during HBlank between lines 47 and 48
        // Should not be used until line 49
        //
        // hblank_vsram_update routine defined in boot.s:
        //   hblank_vsram_update(u8 line, u16 data_word)
        prepare_vsram_write(0);
        __asm__(
            "movem.l %d0-%d1, -(%sp)\n"

            "move.b #48, %d0\n"
            "move.w #1, %d1\n"
            "jsr hblank_vsram_update\n"

            "movem.l (%sp)+, %d0-%d1\n"
        );

        // Change Plane A's V scroll back to 0
        while (*VDP_V_COUNTER != 50);
        vdp_write_vsram(0, 0);

        // Change Plane B's V scroll to 1 during HBlank between lines 95 and 96
        // Should not be used until line 97
        prepare_vsram_write(2);
        __asm__(
            "movem.l %d0-%d1, -(%sp)\n"

            "move.b #96, %d0\n"
            "move.w #1, %d1\n"
            "jsr hblank_vsram_update\n"

            "movem.l (%sp)+, %d0-%d1\n"
        );

        vdp_wait_for_vblank();
    }
}

void test_data_port_latch_reset() {
    write_test_palettes();

    // Write 0x5678 to $1000 and 0xABCD to $2000
    *VDP_CTRL32 = 0x00000000;
    *VDP_CTRL = 0x5000;
    *VDP_DATA = 0x5678; // Should reset control port latch
    *VDP_CTRL32 = 0x60000000;
    *VDP_DATA = 0xABCD;

    // Validate that both writes went through
    bool passed = true;
    *VDP_CTRL32 = 0x10000000;
    passed &= *VDP_DATA == 0x5678;
    *VDP_CTRL32 = 0x20000000;
    passed &= *VDP_DATA == 0xABCD;

    puts(passed ? "PASS" : "FAIL", PLANE_A_ADDR, 18, 13, passed ? 0 : 1);

    vdp_enable_display_vint(test_vint_handler);

    while(!exit_test);
}

void main() {
    while (true) {
        DISABLE_INTS;
        vdp_init();
        load_font_tiles();

        render_menu();

        DISABLE_INTS;
        vdp_write_register(VDP_REG_MODE2, 0x14);

        vdp_vram_fill(PLANE_A_ADDR, 0, 0x1000);
        vdp_vram_fill(SAT_ADDR, 0, 8);

        exit_test = false;
        switch (selected_option) {
            case MENU_HINT_BASICS: {
                test_hint_basics();
                break;
            }
            case MENU_VDP_INT_DELAY: {
                test_vdp_interrupt_delay();
                break;
            }
            case MENU_SAT_CACHE: {
                test_sprite_table_cache();
                break;
            }
            case MENU_SAT_MASK_H40:
            case MENU_SAT_MASK_H32: {
                test_sprite_table_mask(selected_option == MENU_SAT_MASK_H40);
                break;
            }
            case MENU_SPRITE_Y9_PROG:
            case MENU_SPRITE_Y9_INTERLACED: {
                test_sprite_y9(selected_option == MENU_SPRITE_Y9_PROG);
                break;
            }
            case MENU_WINDOW_MASK_H40:
            case MENU_WINDOW_MASK_H32: {
                test_window_nt_mask(selected_option == MENU_WINDOW_MASK_H40);
                break;
            }
            case MENU_FILL_INCREMENT2: {
                test_vram_fill_increment2();
                break;
            }
            case MENU_REGISTER_CODE_BITS: {
                test_register_code_bits();
                break;
            }
            case MENU_MOVEM_L_PREDEC: {
                test_movem_predec();
                break;
            }
            case MENU_HBLANK_NT_UPDATE: {
                test_hblank_nt_update();
                break;
            }
            case MENU_HBLANK_FULL_VS_UPDATE: {
                test_hblank_vsram_update();
                break;
            }
            case MENU_DATA_PORT_LATCH_RESET: {
                test_data_port_latch_reset();
                break;
            }
            default: break;
        }
    }
}

void _error() {
    while(true);
}
