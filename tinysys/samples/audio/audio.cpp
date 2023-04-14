#include "basesystem.h"
#include "uart.h"
#include "audio.h"

int main()
{
    UARTWrite("Hello, world!\n");

    *IO_AUDIOTX = 0x1E00; // reset a few times
    *IO_AUDIOTX = 0x1E00;
    *IO_AUDIOTX = 0x1E00;
    *IO_AUDIOTX = 0x1E00;

    E32Sleep(1);

    *IO_AUDIOTX = 0x0C0A; // power down, enabled: device power, clock, oscillator, outputs (is this needed?), ADC, LINE.
    *IO_AUDIOTX = 0x0017; // left line input, disable simultaneous volume/mute, normal, 101111 db
    *IO_AUDIOTX = 0x0280; // right line input, disable simultaneous volume/mute, muted
    *IO_AUDIOTX = 0x0400; // left channel headphone, simultaneous update disabled, zero cross detect off, 0 db
    *IO_AUDIOTX = 0x0600; // right channel headphone, simultaneous update disabled, zero cross detect off, 0 db
    *IO_AUDIOTX = 0x0E42; // digital audio interface, master mode, LRSWAP disabled, right channel on, LRCIN high, 16 bit, I2S
    *IO_AUDIOTX = 0x0A0A; // digital audio path, DAC soft mute enabled (I don't want DAC), de-emphasis control 32 kHz, ADC high pass filter enabled
    *IO_AUDIOTX = 0x1018; // sample rate, clock input divider MCLK, clock output divider MCLK, 32 kHz with 12,288 MHz, normal mode
    *IO_AUDIOTX = 0x0802; // analog audio path, sidetone disabled, DAC off, bypass disabled, line select LINE, microphone muted
    *IO_AUDIOTX = 0x1201; // digital interface activation; active

    // NOTES: Since we set up the device device to master mode above, we'll get BCLK, LRCOUT and LRCIN from the chip

    // The device is I2S mode
    // We can now tell the 32kHZ stream out unit to start reading from memory at given address and output the sample data

    // Set up audio pointer and start audio output
    // This will start doing burst reads from memory and streaming audio data out
    //*IO_AUDIOFIFO = AUDIOSTREAMSTART;
    //*IO_AUDIOFIFO = audiobufferaddress;

    // Write this value to the fifo to stop audio playback
    // This will stop burst reads and also set sample output to silence
    //*IO_AUDIOFIFO = AUDIOSTREAMSTOP;

    // TODO: Add commands to control volume etc
    // *IO_AUDIOFIFO

    return 0;
}
