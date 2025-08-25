## sound-mixing

Enables testing sound mixing by adjusting SOUNDBIAS and turning channels on and off while playing at max volume.

Holding A makes the Direct Sound sample and sound bias values change in larger increments.

Some interesting test cases:
* When a pulse channel is on plus both Direct Sound channels are on and one of them is playing sample -128, volume should not change when the other channel's sample is any value between 0 and -128
  * Same when one Direct Sound channel is playing 127 and the other is playing any value between 0 and 127
* Toggling "unmute disabled channels" while no channels are on should cause audio pops
* When the wavetable sample pattern is 0000 or FFFF, toggling the wavetable channel on and off should cause audio pops
  * _Unless_ "unmute disabled channels" is on, in which case sample pattern 0000 should not cause audio pops when turning wavetable on/off (FFFF still causes pops)
* When one of the pulse channels is on, volume should be constant when sound bias ranges from 0x080 to 0x380
  * Range changes to 0x040-0x3C0 at PSG volume 1 and 0x020-0x3E0 at PSG volume 0 or 3
* When one of the pulse channels is on, volume should gradually decrease when decreasing sound bias from 0x080 to 0 or increasing from 0x380 to 0x3FF
  * Should still be audible at min/max sound bias
  * Unmuting disabled channels should completely silence audio output at sound bias 0 when only one of the pulse channels is on
  * Unmuting disabled channels should make the audio output _louder_ at sound bias 0x3FF
* When sound bias is 0, turning on one of the pulse channels and one of the Direct Sound channels with sample value -30 should silence audio; -29 should not silence (but should be very quiet)
* When sound bias is 0x3FF, turning on one of the pulse channels and one of the Direct Sound channels with sample value 30 should silence audio; 29 should not silence (but should be very quiet)
* PSG volume 3 (prohibited) should behave the same as PSG volume 0 (25% volume)
