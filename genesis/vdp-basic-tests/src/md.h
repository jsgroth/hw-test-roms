#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;

typedef volatile u8 vu8;
typedef volatile u16 vu16;
typedef volatile u32 vu32;
typedef volatile bool vbool;

typedef struct {
    u16 tile;
    u16 x;
    u16 y;
    u8 h_cells;
    u8 v_cells;
    u8 palette;
    bool priority;
    bool h_flip;
    bool v_flip;
    u8 link;
} Sprite;

#define NOP __asm__ __volatile__("nop");

#define ENABLE_INTS __asm__("move #0x2000, %sr");
#define DISABLE_INTS __asm__("move #0x2700, %sr");

// defined in boot.s
extern vu32 v_vint_vector;
extern vu32 v_hint_vector;
extern const u32 _noop_int_handler;

// defined in font.s
// must be an array rather than a pointer or it will contain the wrong value
extern const u32 FONT_TILES[];

// defined in md.c
extern vu16* const VDP_DATA;
extern vu16* const VDP_CTRL;
extern vu32* const VDP_CTRL32;
extern vu16* const VDP_HV_COUNTER;
extern vu8* const VDP_H_COUNTER;
extern vu8* const VDP_V_COUNTER;

extern const u8 VDP_REG_MODE1;
extern const u8 VDP_REG_MODE2;
extern const u8 VDP_REG_PLANE_A_NT;
extern const u8 VDP_REG_WINDOW_NT;
extern const u8 VDP_REG_PLANE_B_NT;
extern const u8 VDP_REG_SAT_ADDR;
extern const u8 VDP_REG_BACKDROP_COLOR;
extern const u8 VDP_REG_HINT_INTERVAL;
extern const u8 VDP_REG_MODE3;
extern const u8 VDP_REG_MODE4;
extern const u8 VDP_REG_H_SCROLL_ADDR;
extern const u8 VDP_REG_PORT_INCREMENT;
extern const u8 VDP_REG_PLANE_SIZE;
extern const u8 VDP_REG_WINDOW_H;
extern const u8 VDP_REG_WINDOW_V;
extern const u8 VDP_REG_DMA_LEN_L;
extern const u8 VDP_REG_DMA_LEN_H;
extern const u8 VDP_REG_DMA_SRC_L;
extern const u8 VDP_REG_DMA_SRC_M;
extern const u8 VDP_REG_DMA_SRC_H;

extern const u16 PLANE_A_ADDR;
extern const u16 PLANE_B_ADDR;
extern const u16 H_SCROLL_ADDR;
extern const u16 SAT_ADDR;
extern const u16 FONT_TILE_ADDR;

extern const u8 JOYPAD_UP;
extern const u8 JOYPAD_DOWN;
extern const u8 JOYPAD_LEFT;
extern const u8 JOYPAD_RIGHT;
extern const u8 JOYPAD_A;
extern const u8 JOYPAD_B;
extern const u8 JOYPAD_C;
extern const u8 JOYPAD_START;

void vdp_init();
void vdp_write_register(u8 reg, u8 value);
void vdp_write_vram(u16 addr, u16 word);
void vdp_write_cram(u16 addr, u16 word);
void vdp_write_vsram(u16 addr, u16 word);
void vdp_write_sprite(u16 sat_addr, u8 idx, Sprite* sprite);
void vdp_dma_vram(u32 source, u16 dest, u16 len);
void vdp_dma_cram(u32 source, u16 dest, u16 len);
void vdp_vram_fill(u16 addr, u8 data, u16 len);
void vdp_vram_copy(u16 from, u16 to, u16 len);
void vdp_enable_display_vint(void* vint_handler);
void vdp_enable_hint(void* hint_handler);
void vdp_disable_hint();
void vdp_wait_for_vblank();

u8 read_p1_inputs();

void load_font_tiles();
u16 font_tile_number(unsigned char c);
void puts(char* s, u16 plane_nt_addr, u8 x, u8 y, u8 palette);
