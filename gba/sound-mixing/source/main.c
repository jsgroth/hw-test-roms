#include <stdlib.h>

#include <tonc.h>

// Pulse 1
volatile u16* SOUND1CNT_L = (volatile u16*)0x4000060;
volatile u16* SOUND1CNT_H = (volatile u16*)0x4000062;
volatile u16* SOUND1CNT_X = (volatile u16*)0x4000064;

// Pulse 2
volatile u16* SOUND2CNT_L = (volatile u16*)0x4000068;
volatile u16* SOUND2CNT_H = (volatile u16*)0x400006C;

// Wavetable
volatile u16* SOUND3CNT_L = (volatile u16*)0x4000070;
volatile u16* SOUND3CNT_H = (volatile u16*)0x4000072;
volatile u16* SOUND3CNT_X = (volatile u16*)0x4000074;
volatile u16* WAVE_RAM    = (volatile u16*)0x4000090;

// Noise
volatile u16* SOUND4CNT_L = (volatile u16*)0x4000078;
volatile u16* SOUND4CNT_H = (volatile u16*)0x400007C;

// Direct Sound
volatile u32* FIFO_A      = (volatile u32*)0x40000A0;
volatile u32* FIFO_B      = (volatile u32*)0x40000A4;

// APU control
volatile u16* SOUNDCNT_L  = (volatile u16*)0x4000080;
volatile u16* SOUNDCNT_H  = (volatile u16*)0x4000082;
volatile u16* SOUNDCNT_X  = (volatile u16*)0x4000084;
volatile u16* SOUNDBIAS   = (volatile u16*)0x4000088;

// Timer 0
volatile u16* TM0CNT_L    = (volatile u16*)0x4000100;
volatile u16* TM0CNT_H    = (volatile u16*)0x4000102;

// DMA
volatile u32* DMA1SAD     = (volatile u32*)0x40000BC;
volatile u32* DMA1DAD     = (volatile u32*)0x40000C0;
volatile u16* DMA1CNT_L   = (volatile u16*)0x40000C4;
volatile u16* DMA1CNT_H   = (volatile u16*)0x40000C6;
volatile u32* DMA2SAD     = (volatile u32*)0x40000C8;
volatile u32* DMA2DAD     = (volatile u32*)0x40000CC;
volatile u16* DMA2CNT_L   = (volatile u16*)0x40000D0;
volatile u16* DMA2CNT_H   = (volatile u16*)0x40000D2;

// Keypad
volatile u16* KEYINPUT    = (volatile u16*)0x4000130;

volatile u16* PALETTE_RAM = (volatile u16*)0x5000000;

int main(void) {
	irq_init(NULL);
	irq_enable(II_VBLANK);

    // Turn on APU
    *SOUNDCNT_X = 1 << 7;
    *SOUNDCNT_L = 0;
    *SOUNDBIAS = 0x200;

    u16 pulse1_freq = 1720;
    u16 pulse2_freq = 1600;
    u16 wavetable_freq = 0;
    u16 noise_freq = 4 | (7 << 4);

    // Initialize channel 1 (pulse w/ sweep)
    // Sweep disabled, 50% duty, envelope disabled
    *SOUND1CNT_L = 1 << 3;
    *SOUND1CNT_H = (2 << 6);

    // Initialize channel 2 (pulse w/o sweep)
    // 50% duty, envelope disabled
    *SOUND2CNT_L = (2 << 6);

    // Initialize channel 3 (wavetable)
    // Alternating 0 and 15 samples
    *SOUND3CNT_L = 1 << 6;
    for (int i = 0; i < 8; i++) WAVE_RAM[i] = 0x00FF;
    *SOUND3CNT_L = 0;
    *SOUND3CNT_H = 1 << 13;

    // Initialize channel 4 (noise)
    // Envelope disabled
    *SOUND4CNT_L = 0;

    bool pulse1_on = false;
    bool pulse2_on = false;
    bool wavetable_on = false;
    bool noise_on = false;
    bool pcm_a_on = false;
    bool pcm_b_on = false;
    bool force_nr51 = false;
    u16 psg_volume = 2;

    s8 pcm_a_samples[4] = {};
    s16 pcm_a_sample = 0;

    s8 pcm_b_samples[4] = {};
    s16 pcm_b_sample = 0;

    u16 bias = 0x200;

    // Initialize channels A and B (Direct Sound)
    // Repeatedly play the same sample value
    *SOUNDCNT_H = (1 << 11) | (1 << 15);

    *DMA1SAD = (u32)pcm_a_samples;
    *DMA1DAD = (u32)FIFO_A;
    *DMA1CNT_L = 4;
    *DMA1CNT_H = (2 << 5) | (2 << 7) | (1 << 9) | (3 << 12) | (1 << 15);

    *DMA2SAD = (u32)pcm_b_samples;
    *DMA2DAD = (u32)FIFO_B;
    *DMA2CNT_L = 4;
    *DMA2CNT_H = (2 << 5) | (2 << 7) | (1 << 9) | (3 << 12) | (1 << 15);

    *TM0CNT_L = 0xFA00;
    *TM0CNT_H = 1 << 7;

    u16 inputs = *KEYINPUT;

    u16 cursor = 0;

    bool odd_frame = false;

	while (1) {
		VBlankIntrWait();

	    REG_DISPCNT = DCNT_MODE0 | (odd_frame ? DCNT_BG1 : DCNT_BG0);
        odd_frame = !odd_frame;

        tte_init_chr4c_default(odd_frame ? 1 : 0, BG_CBB(odd_frame ? 2 : 0) | BG_SBB(odd_frame ? 31 : 30));

        for (int i = 0; i < 15; i++) {
            PALETTE_RAM[15 * 16 + i + 1] = cursor == i ? 0xFFFF : 0x001F;
        }

        tte_set_pos(8, 8);
        tte_erase_line();
        tte_write("#{ci:1}Pulse 1: ");
        tte_write(pulse1_on ? "On" : "Off");

        tte_set_pos(8, 20);
        tte_erase_line();
        tte_write("#{ci:2}Pulse 2: ");
        tte_write(pulse2_on ? "On" : "Off");

        tte_set_pos(8, 32);
        tte_erase_line();
        tte_write("#{ci:3}Wavetable: ");
        tte_write(wavetable_on ? "On" : "Off");

        tte_set_pos(8, 44);
        tte_erase_line();
        tte_write("#{ci:4}Noise: ");
        tte_write(noise_on ? "On" : "Off");

        tte_set_pos(8, 56);
        tte_erase_line();
        tte_write("#{ci:5}Direct Sound A: ");
        tte_write(pcm_a_on ? "On" : "Off");

        tte_set_pos(8, 68);
        tte_erase_line();
        tte_write("#{ci:6}Direct Sound A sample: ");

        char s[10];
        itoa(pcm_a_sample, s, 10);
        tte_write(s);

        tte_set_pos(8, 80);
        tte_erase_line();
        tte_write("#{ci:7}Direct Sound B: ");
        tte_write(pcm_b_on ? "On" : "Off");

        tte_set_pos(8, 92);
        tte_erase_line();
        tte_write("#{ci:8}Direct Sound B sample: ");

        itoa(pcm_b_sample, s, 10);
        tte_write(s);

        tte_set_pos(8, 104);
        tte_erase_line();
        tte_write("#{ci:9}Sound bias: 0x");
        itoa(bias, s, 16);
        tte_write(s);

        tte_set_pos(8, 116);
        tte_erase_line();
        tte_write("#{ci:10}PSG volume: ");
        itoa(psg_volume, s, 10);
        tte_write(s);

        tte_set_pos(8, 128);
        tte_erase_line();
        tte_write("#{ci:11}Unmute disabled channels in NR51: ");
        tte_write(force_nr51 ? "On" : "Off");

        u16 new_inputs = *KEYINPUT;
        u16 pressed = ~new_inputs & inputs;
        inputs = new_inputs;

        bool a_held = (new_inputs & 1) == 0;
        s16 pcm_step = a_held ? 16 : 1;

        bool right = (pressed & (1 << 4)) != 0;
        bool left = (pressed & (1 << 5)) != 0;
        bool up = (pressed & (1 << 6)) != 0;
        bool down = (pressed & (1 << 7)) != 0;

        if (up ^ down) {
            if (up) {
                cursor = cursor == 0 ? 10 : cursor - 1;
            } else if (down) {
                cursor = (cursor == 10 ? 0 : cursor + 1);
            }
        }

        if (left ^ right) {
            switch (cursor) {
                case 0: {
                    pulse1_on = !pulse1_on;
                    if (pulse1_on) {
                        *SOUND1CNT_H |= (15 << 12);
                        *SOUND1CNT_X = pulse1_freq | (1 << 15);
                    } else {
                        *SOUND1CNT_H &= !(15 << 12);
                    }
                    break;
                }
                case 1: {
                    pulse2_on = !pulse2_on;
                    if (pulse2_on) {
                        *SOUND2CNT_L |= (15 << 12);
                        *SOUND2CNT_H = pulse2_freq | (1 << 15);
                    } else {
                        *SOUND2CNT_L &= !(15 << 12);
                    }
                    break;
                }
                case 2: {
                    wavetable_on = !wavetable_on;
                    if (wavetable_on) {
                        *SOUND3CNT_L |= 1 << 7;
                        *SOUND3CNT_X = wavetable_freq | (1 << 15);
                    } else {
                        *SOUND3CNT_L &= !(1 << 7);
                    }
                    break;
                }
                case 3: {
                    noise_on = !noise_on;
                    if (noise_on) {
                        *SOUND4CNT_L |= 15 << 12;
                        *SOUND4CNT_H = noise_freq | (1 << 15);
                    } else {
                        *SOUND4CNT_L &= !(15 << 12);
                    }
                    break;
                }
                case 4: {
                    pcm_a_on = !pcm_a_on;
                    break;
                }
                case 5: {
                    if (left) {
                        pcm_a_sample -= pcm_step;
                        if (pcm_a_sample < -128) pcm_a_sample = -128;
                    } else {
                        pcm_a_sample += pcm_step;
                        if (pcm_a_sample > 127) pcm_a_sample = 127;
                    }
                    break;
                }
                case 6: {
                    pcm_b_on = !pcm_b_on;
                    break;
                }
                case 7: {
                    if (left) {
                        pcm_b_sample -= pcm_step;
                        if (pcm_b_sample < -128) pcm_b_sample = -128;
                    } else {
                        pcm_b_sample += pcm_step;
                        if (pcm_b_sample > 127) pcm_b_sample = 127;
                    }
                    break;
                }
                case 8: {
                    if (left && bias != 0) {
                        if (bias == 0x3FF) bias = 0x400;
                        bias -= 16;
                    } else if (right && bias != 0x3FF) {
                        bias += 16;
                        if (bias > 0x3FF) bias = 0x3FF;
                    }
                    break;
                }
                case 9: {
                    if (left && psg_volume != 0) {
                        psg_volume -= 1;
                    } else if (right && psg_volume != 3) {
                        psg_volume += 1;
                    }
                    break;
                }
                case 10: {
                    force_nr51 = !force_nr51;
                    break;
                }
                default: break;
            }
        }

        // Update PSG stereo control
        if (force_nr51) {
            *SOUNDCNT_L = 7 | (7 << 4) | (15 << 8) | (15 << 12);
        } else {
            *SOUNDCNT_L = 7 | (7 << 4)
                | (pulse1_on << 8) | (pulse1_on << 12)
                | (pulse2_on << 9) | (pulse2_on << 13)
                | (wavetable_on << 10) | (wavetable_on << 14)
                | (noise_on << 11) | (noise_on << 15);
        }

        // Update GBA sound control
        *SOUNDCNT_H = psg_volume | (1 << 2) | (1 << 3)
            | (pcm_a_on << 8) | (pcm_a_on << 9)
            | (pcm_b_on << 12) | (pcm_b_on << 13);

        // Update PCM samples
        for (int i = 0; i < 4; i++) pcm_a_samples[i] = pcm_a_sample;
        for (int i = 0; i < 4; i++) pcm_b_samples[i] = pcm_b_sample;

        // Update sound bias (32768 Hz / 9-bit PWM sampling)
        *SOUNDBIAS = bias;
	}
}
