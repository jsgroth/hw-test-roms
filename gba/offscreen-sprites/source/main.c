#include <tonc.h>

vu16* BG_PALETTES  = (vu16*)0x05000000;
vu16* OBJ_PALETTES = (vu16*)0x05000200;
vu16* OBJ_VRAM     = (vu16*)0x06010000;
vu16* OAM          = (vu16*)0x07000000;

vu16* DISPCNT      = (vu16*)0x04000000;

int main(void) {
    irq_init(NULL);

    // Enable forced blanking
    *DISPCNT = 1 << 7;

    // Black backdrop
    BG_PALETTES[0] = 0;

    // Sprite colors
    OBJ_PALETTES[1] = 0x7FFF;
    OBJ_PALETTES[2] = 0x1F;
    OBJ_PALETTES[3] = 0x1F << 5;
    OBJ_PALETTES[4] = 0x1F << 10;
    OBJ_PALETTES[5] = 0x10 | (0x10 << 10);
    OBJ_PALETTES[6] = 0x1F | (0x1F << 5);
    OBJ_PALETTES[7] = (0x1F << 5) | (0x1F << 10);
    OBJ_PALETTES[8] = 0x1A | (0x1 << 5) | (0xE << 10);
    OBJ_PALETTES[9] = 0x3 | (0xF << 5) | (0x1F << 10);

    // Fill in sprite tile data
    // Tiles 0-63: all 1
    // Tiles 64-127: all 2
    // Tiles 128-191: all 3
    // Tiles 192-255: all 4
    // Tiles 256-319: alternating colors 2-9
    for (int i = 0; i < 64*64/4; i++) {
        OBJ_VRAM[i] = 0x1111;
        OBJ_VRAM[64*64/4 + i] = 0x2222;
        OBJ_VRAM[64*64/2 + i] = 0x3333;
        OBJ_VRAM[3*64*64/4 + i] = 0x4444;
        OBJ_VRAM[64*64 + i] = ((i % 2) != 0) ? 0x2345 : 0x6789;
    }

    // Set affine parameter group 0 to the identity matrix
    OAM[3] = 1 << 8;
    OAM[7] = 0;
    OAM[11] = 0;
    OAM[15] = 1 << 8;

    // 50 fully offscreen 128x128 sprites, at right edge of frame
    // Should take 100 cycles
    for (int i = 0; i < 50; i++) {
        vu16* oam_entry = &OAM[4*i];

        // 128x128 affine sprite at X=240, Y=20, tile 0, palette 0, parameter group 0
        oam_entry[0] = 20 | (1 << 8) | (1 << 9);
        oam_entry[1] = 240 | (3 << 14);
        oam_entry[2] = 0;
    }

    // 50 fully offscreen 128x128 sprites, at left edge of frame
    // Should take 100 cycles (200 total)
    for (int i = 50; i < 100; i++) {
        vu16* oam_entry = &OAM[4*i];

        // 128x128 affine sprite at X=384, Y=20, tile 0, palette 0, parameter group 0
        oam_entry[0] = 20 | (1 << 8) | (1 << 9);
        oam_entry[1] = 384 | (3 << 14);
        oam_entry[2] = 0;
    }

    // 2 partially onscreen 128x128 sprites, offscreen to the right
    // Should take 4+532 cycles (no cycles saved for offscreen pixels)
    // Now at 736 cycles total
    for (int i = 100; i < 102; i++) {
        vu16* oam_entry = &OAM[4*i];

        // 128x128 affine sprite at X=175, Y=20, tile 0, palette 0, parameter group 0, priority 3
        oam_entry[0] = 20 | (1 << 8) | (1 << 9);
        oam_entry[1] = 175 | (3 << 14);
        oam_entry[2] = 3 << 10;
    }

    // 2 partially onscreen 128x128 sprites, offscreen to the left
    // Should take 280 cycles (saved cycles for offscreen pixels)
    // Now at 1016 cycles total
    for (int i = 102; i < 104; i++) {
        vu16* oam_entry = &OAM[4*i];

        // 128x128 affine sprite at X=449, Y=20, tile 0, palette 0, parameter group 0, priority 3
        oam_entry[0] = 20 | (1 << 8) | (1 << 9);
        oam_entry[1] = 449 | (3 << 14);
        oam_entry[2] = 3 << 10;
    }

    // 4 non-affine 64x64 sprites, first partially offscreen to the left (3 pixels)
    // First tile should take 62 cycles (1078 total)
    // Second and third tiles should each take 64 cycles (1206 total)
    // Fourth tile would take 64 cycles, but only 26 cycles remain (1232 per line) so only 24 pixels should render
    // (The last VRAM fetch is seemingly thrown away before rendering it)
    for (int i = 104; i < 108; i++) {
        vu16* oam_entry = &OAM[4*i];

        // 64x64 non-affine sprite at Y=0, palette 0, priority 0
        // X coordinates are 509 (-3), 61, 125, 191
        // Tile numbers are 64, 128, 192, 256
        oam_entry[0] = 0;
        u16 x = (64 * (i - 104) - 3) & 0x1FF;
        oam_entry[1] = x | (3 << 14);
        oam_entry[2] = 64 + 64 * (i - 104);
    }

    // Disable remaining sprites
    for (int i = 108; i < 128; i++) {
        vu16* oam_entry = &OAM[4*i];
        oam_entry[0] = 1 << 9;
    }

    // Disable forced blanking, disable all BG layers, enable OBJs, not free HBlank, 1D OBJ VRAM mapping, BG mode 0
    *DISPCNT = (1 << 6) | (1 << 12);

    // Loop forever
    while (1) {
        VBlankIntrWait();
    }
}
