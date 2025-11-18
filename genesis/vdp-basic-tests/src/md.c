#include "md.h"

vu16* const VDP_DATA = (vu16*)0xC00000;
vu16* const VDP_CTRL = (vu16*)0xC00004;
vu32* const VDP_CTRL32 = (vu32*)VDP_CTRL;
vu16* const VDP_HV_COUNTER = (vu16*)0xC00008;
vu8* const VDP_V_COUNTER = (vu8*)0xC00008;
vu8* const VDP_H_COUNTER = (vu8*)0xC00009;

static vu8* const P1_DATA = (vu8*)0xA10003;
static vu8* const P1_CTRL = (vu8*)0xA10009;

const u8 VDP_REG_MODE1 = 0;
const u8 VDP_REG_MODE2 = 1;
const u8 VDP_REG_PLANE_A_NT = 2;
const u8 VDP_REG_WINDOW_NT = 3;
const u8 VDP_REG_PLANE_B_NT = 4;
const u8 VDP_REG_SAT_ADDR = 5;
const u8 VDP_REG_BACKDROP_COLOR = 7;
const u8 VDP_REG_HINT_INTERVAL = 10;
const u8 VDP_REG_MODE3 = 11;
const u8 VDP_REG_MODE4 = 12;
const u8 VDP_REG_H_SCROLL_ADDR = 13;
const u8 VDP_REG_PORT_INCREMENT = 15;
const u8 VDP_REG_PLANE_SIZE = 16;
const u8 VDP_REG_WINDOW_H = 17;
const u8 VDP_REG_WINDOW_V = 18;
const u8 VDP_REG_DMA_LEN_L = 19;
const u8 VDP_REG_DMA_LEN_H = 20;
const u8 VDP_REG_DMA_SRC_L = 21;
const u8 VDP_REG_DMA_SRC_M = 22;
const u8 VDP_REG_DMA_SRC_H = 23;

const u16 PLANE_A_ADDR = 0xA000;
const u16 PLANE_B_ADDR = 0xC000;
const u16 H_SCROLL_ADDR = 0xF800;
const u16 SAT_ADDR = 0xFC00;
const u16 FONT_TILE_ADDR = 0x9000;

const u8 VDP_TARGET_VRAM_WRITE = 1;
const u8 VDP_TARGET_CRAM_WRITE = 3;
const u8 VDP_TARGET_VSRAM_WRITE = 5;
const u8 VDP_TARGET_VRAM_READ = 0;
const u8 VDP_TARGET_CRAM_READ = 8;
const u8 VDP_TARGET_VSRAM_READ = 4;

const u8 JOYPAD_UP = 1 << 0;
const u8 JOYPAD_DOWN = 1 << 1;
const u8 JOYPAD_LEFT = 1 << 2;
const u8 JOYPAD_RIGHT = 1 << 3;
const u8 JOYPAD_B = 1 << 4;
const u8 JOYPAD_C = 1 << 5;
const u8 JOYPAD_A = 1 << 6;
const u8 JOYPAD_START = 1 << 7;

void vdp_init() {
    vdp_write_register(VDP_REG_MODE1, 0x04); // HINT disabled
    vdp_write_register(VDP_REG_MODE2, 0x14); // Display and VINT disabled, DMA enabled
    vdp_write_register(VDP_REG_PLANE_A_NT, (PLANE_A_ADDR >> 13) << 3);
    vdp_write_register(VDP_REG_PLANE_B_NT, PLANE_B_ADDR >> 13);
    vdp_write_register(VDP_REG_SAT_ADDR, SAT_ADDR >> 9);
    vdp_write_register(VDP_REG_BACKDROP_COLOR, 0);
    vdp_write_register(VDP_REG_HINT_INTERVAL, 255);
    vdp_write_register(VDP_REG_MODE3, 0); // Full screen H and V scroll
    vdp_write_register(VDP_REG_MODE4, 0x81); // H40, non-interlaced, shadow/highlight disabled
    vdp_write_register(VDP_REG_H_SCROLL_ADDR, H_SCROLL_ADDR >> 10);
    vdp_write_register(VDP_REG_PLANE_SIZE, 0x01); // 64x32 tiles
    vdp_write_register(VDP_REG_WINDOW_H, 0); // Window H fully offscreen
    vdp_write_register(VDP_REG_WINDOW_V, 0); // Window V fully offscreen

    // Zerofill VRAM using a max len fill DMA
    vdp_write_register(VDP_REG_PORT_INCREMENT, 1);
    vdp_write_register(VDP_REG_DMA_LEN_L, 0);
    vdp_write_register(VDP_REG_DMA_LEN_H, 0);
    vdp_write_register(VDP_REG_DMA_SRC_H, 0x80);

    *VDP_CTRL32 = 0x40000080;
    *VDP_DATA = 0;
    while (*VDP_CTRL & 2);

    // Zerofill CRAM
    vdp_write_register(VDP_REG_PORT_INCREMENT, 2);
    *VDP_CTRL32 = 0xC0000000;
    for (u16 i = 0; i < 64; i++) *VDP_DATA = 0;

    // Zerofill VSRAM
    *VDP_CTRL32 = 0x40000010;
    for (u16 i = 0; i < 40; i++) *VDP_DATA = 0;
}

void vdp_write_register(u8 reg, u8 value) {
    *VDP_CTRL = 0x8000 | (reg << 8) | value;
}

void vdp_write_vram(u16 addr, u16 word) {
    *VDP_CTRL = 0x4000 | (addr & 0x3FFF);
    *VDP_CTRL = addr >> 14;
    *VDP_DATA = word;
}

void vdp_write_cram(u16 addr, u16 word) {
    *VDP_CTRL = 0xC000 | (addr & 0x3FFF);
    *VDP_CTRL = addr >> 14;
    *VDP_DATA = word;
}

void vdp_write_vsram(u16 addr, u16 word) {
    *VDP_CTRL = 0x4000 | (addr & 0x3FFF);
    *VDP_CTRL = 0x0010 | (addr >> 14);
    *VDP_DATA = word;
}

void vdp_write_sprite(u16 sat_addr, u8 idx, Sprite* sprite) {
    vdp_write_register(VDP_REG_PORT_INCREMENT, 2);

    u16 addr = sat_addr | (idx << 3);
    *VDP_CTRL = 0x4000 | (addr & 0x3FFF);
    *VDP_CTRL = addr >> 14;

    *VDP_DATA = sprite->y;
    *VDP_DATA = sprite->link | (sprite->v_cells << 8) | (sprite->h_cells << 10);
    *VDP_DATA = sprite->tile
        | (sprite->h_flip << 11)
        | (sprite->v_flip << 12)
        | (sprite->palette << 13)
        | (sprite->priority << 15);
    *VDP_DATA = sprite->x;
}

void vdp_dma(u32 source, u16 dest, u16 len, u8 target) {
    vdp_write_register(VDP_REG_PORT_INCREMENT, 2);

    vdp_write_register(VDP_REG_DMA_LEN_L, len);
    vdp_write_register(VDP_REG_DMA_LEN_H, len >> 8);

    source &= 0xFFFFFF;
    vdp_write_register(VDP_REG_DMA_SRC_L, source >> 1);
    vdp_write_register(VDP_REG_DMA_SRC_M, source >> 9);
    vdp_write_register(VDP_REG_DMA_SRC_H, source >> 17);

    *VDP_CTRL = ((target & 3) << 14) | (dest & 0x3FFF);
    *VDP_CTRL = 0x0080 | ((target >> 2) << 4) | (dest >> 14);
}

void vdp_dma_vram(u32 source, u16 dest, u16 len) {
    vdp_dma(source, dest, len, VDP_TARGET_VRAM_WRITE);
}

void vdp_dma_cram(u32 source, u16 dest, u16 len) {
    vdp_dma(source, dest, len, VDP_TARGET_CRAM_WRITE);
}

void vdp_vram_fill(u16 addr, u8 data, u16 len) {
    len -= 1; // Initial data port write does not count towards DMA length

    vdp_write_register(VDP_REG_PORT_INCREMENT, 1);
    vdp_write_register(VDP_REG_DMA_LEN_L, len);
    vdp_write_register(VDP_REG_DMA_LEN_H, len >> 8);
    vdp_write_register(VDP_REG_DMA_SRC_H, 0x80);

    *VDP_CTRL = 0x4000 | (addr & 0x3FFF);
    *VDP_CTRL = 0x0080 | (addr >> 14);
    *VDP_DATA = data | (data << 8);

    while (*VDP_CTRL & 2);
}

void vdp_vram_copy(u16 from, u16 to, u16 len) {
    vdp_write_register(VDP_REG_PORT_INCREMENT, 1);
    vdp_write_register(VDP_REG_DMA_LEN_L, len);
    vdp_write_register(VDP_REG_DMA_LEN_H, len >> 8); 
    vdp_write_register(VDP_REG_DMA_SRC_L, from);
    vdp_write_register(VDP_REG_DMA_SRC_M, from >> 8);
    vdp_write_register(VDP_REG_DMA_SRC_H, 0xC0);

    *VDP_CTRL = 0x4000 | (to & 0x3FFF);
    *VDP_CTRL = 0x00C0 | (to >> 14);

    while (*VDP_CTRL & 2);
}

static vbool handled_vint = false;

void vsync_vint_handler() {
    handled_vint = true; 
}

void vdp_enable_display_vint(void* vint_handler) {
    v_vint_vector = (u32)vsync_vint_handler;

    vdp_write_register(VDP_REG_MODE2, 0x24);
    ENABLE_INTS;

    // Any already-pending VINT has been handled by this point; now properly VSync
    handled_vint = false;
    while(!handled_vint);

    v_vint_vector = (u32)vint_handler;
    vdp_write_register(VDP_REG_MODE2, 0x74);
}

void vdp_enable_hint(void* hint_handler) {
    v_hint_vector = (u32)&_noop_int_handler;

    vdp_write_register(VDP_REG_MODE1, 0x14);
    ENABLE_INTS;

    // Any already-pending HINT has been handled by this point
    v_hint_vector = (u32)hint_handler;
}

void vdp_disable_hint() {
    vdp_write_register(VDP_REG_MODE1, 0x04);
    vdp_write_register(VDP_REG_HINT_INTERVAL, 255);
    v_hint_vector = (u32)&_noop_int_handler;
}

void vdp_wait_for_vblank() {
    while (!(*VDP_CTRL & 8));
}

u8 read_p1_inputs() {
    *P1_CTRL = 0x40;

    *P1_DATA = 0x40;
    NOP; NOP;
    u8 inputs = ~(*P1_DATA) & 0x3F;

    *P1_DATA = 0x00;
    NOP; NOP;
    inputs |= (~(*P1_DATA) << 2) & 0xC0;

    return inputs;
}

void load_font_tiles() {
    vdp_dma_vram((u32)FONT_TILES, FONT_TILE_ADDR, (32 * 96) >> 1);
}

u16 font_tile_number(unsigned char c) {
    if (c < 32 || c >= 128) return FONT_TILE_ADDR >> 5;
    return (FONT_TILE_ADDR >> 5) | (c - 32);
}

void puts(char* s, u16 plane_nt_addr, u8 x, u8 y, u8 palette) {
    u16 row_offset = y << 7;
    u16 col_offset = x << 1;
    u16 addr = plane_nt_addr | row_offset | col_offset;

    vdp_write_register(VDP_REG_PORT_INCREMENT, 2);
    *VDP_CTRL = 0x4000 | (addr & 0x3FFF);
    *VDP_CTRL = addr >> 14;

    unsigned char c;
    while ((c = *s++)) {
        u16 tile = font_tile_number(c);
        *VDP_DATA = tile | (palette << 13);
    }
}
