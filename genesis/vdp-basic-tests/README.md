# vdp-basic-tests

Some tests for various VDP functionality that games are known to depend on. These are meant to be basic tests, not extremely comprehensive or strict.

I wrote this mainly just for some practice coding for the Genesis but maybe it will be useful to others.

Most of these tests exercise behaviors that require visual inspection to verify, although a few of them simply perform some VRAM writes and then display PASS or FAIL based on reading the contents of VRAM afterwards. All tests pass on actual hardware.

As a prerequisite, most/all of these tests require emulating the following VDP functionality:
* Basic control and data port functionality
* All three graphics layers (Plane A, Plane B, Sprites); window is only needed for the window tests
* All three DMA modes (memory to VRAM/CRAM/VSRAM, VRAM fill, VRAM copy); timing does not need to be accurate
* Status register's VBlank and DMA active flags (the latter is used to wait for fill/copy DMAs to finish)
* HV counter

## HINT Basics

Tests that HINT triggers on the correct lines when the HINT interval is updated mid-frame, and that the counter decrements 225 times per frame (decrements before the first line and after the last line).

Many games depend on correct HINT emulation.

## VDP Interrupt Enable Delay

Tests enabling HINT then VINT with two consecutive MOVE.W instructions while both interrupts are pending. Expects the 68000 to handle HINT twice and to not handle VINT.

Fatal Rewind / The Killing Game Show depends on this or it will fail to boot. Sesame Street: Counting Cafe depends on a subset of this behavior (VINT enable delay) or it will also fail to boot.

## Sprite Table Cache

Tests that sprite scan and sprite attribute fetch read the first two sprite attribute words (Y coordinate + sprite size + link data) from a write-through cache rather than reading directly from VRAM, and that changing the sprite attribute table address does not immediately modify the cache.

Castlevania: Bloodlines depends on this for the reflection effect in Stage 2.

## Sprite Table Address Mask

Tests that the sprite attribute table address is masked correctly in both H40 and H32 modes.

I'm not actually sure if any official releases depend on this, but Overdrive 1 has an effect that depends on correctly masking the SAT address in H40 mode.

## Sprite Y Mask

Tests that sprite Y coordinates are masked correctly in non-interlaced mode and in double-screen interlaced mode.

Battletoads has various graphical glitches if sprite Y coordinates are not masked to 9-bit in non-interlaced mode.

## Window Nametable Address Mask

Tests that the window nametable address is masked correctly in both H40 and H32 modes.

Cheese Cat-Astrophe Starring Speedy Gonzales has graphical glitches on some screens if the address is not masked correctly in H40 mode.

## VRAM Fill w/ Data Port Increment 2

Tests that VRAM fill works as expected when the data port increment is a value other than 1.

Contra: Hard Corps has heavily corrupted graphics if this is not emulated correctly.

## Register Writes Change Code Bits

Tests that VDP register writes change the lowest code/target bits.

Monster World IV has heavily corrupted title screen graphics if this is not emulated.

## MOVEM.L Pre-Decrement Into VDP Ports

Tests writing into the VDP ports using a MOVEM.L instruction with indirect pre-decrement addressing. Expects words to be written in reverse order.

Monster World IV depends on this for the intro text scroll to work correctly.

## HBlank Nametable Address Updates

Tests that writes to plane A/B nametable address registers during HBlank take effect on the very next line, not the line after.

The Adventures of Batman & Robin depends on this for some graphical effects.

## HBlank Full Screen VSRAM Updates

Tests that when the VDP is using full screen V scroll, writes to the first two words in VSRAM during HBlank do not take effect until after the next line.

A number of games depend on this for correct graphics.

## Data Writes Reset Command Latch

Tests that writing to the data port resets the control port first/second command latch.

A number of games depend on this, but in particular Dynamite Headdy can randomly have corrupted graphics if this is not emulated correctly.
