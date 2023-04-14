#include "basesystem.h"
#include "uart.h"
#include "audio.h"

int main()
{
    UARTWrite("Hello, world!\n");

    *IO_AUDIOTX = 0x1E00;
    *IO_AUDIOTX = 0x1E00;
    *IO_AUDIOTX = 0x1E00;
    *IO_AUDIOTX = 0x1E00;

    E32Sleep(1);

    *IO_AUDIOTX = 0x0C0A; //power down, enabled: device power, clock, oscillator, outputs (is this needed?), ADC, LINE.
    *IO_AUDIOTX = 0x0017; //left line input, disable simultaneous volume/mute, normal, 101111 db
    *IO_AUDIOTX = 0x0280; //right line input, disable simultaneous volume/mute, muted
    *IO_AUDIOTX = 0x0400; //left channel headphone, simultaneous update disabled, zero cross detect off, 0 db
    *IO_AUDIOTX = 0x0600; //right channel headphone, simultaneous update disabled, zero cross detect off, 0 db
    *IO_AUDIOTX = 0x0E42; //digital audio interface, master mode, LRSWAP disabled, right channel on, LRCIN high, 16 bit, I2S
    *IO_AUDIOTX = 0x0A0A; //digital audio path, DAC soft mute enabled (I don't want DAC), de-emphasis control 32 kHz, ADC high pass filter enabled
    *IO_AUDIOTX = 0x1018; //sample rate, clock input divider MCLK, clock output divider MCLK, 32 kHz with 12,288 MHz, normal mode
    *IO_AUDIOTX = 0x0802; //analog audio path, sidetone disabled, DAC off, bypass disabled, line select LINE, microphone muted
    *IO_AUDIOTX = 0x1201; //digital interface activation.

    // now we're in I2S mode, tell data out port to start reading and outputting bytes
    //*IO_AUDIOI2S = SETDATAPOINTER;
    //*IO_AUDIOI2S = audiobufferaddress;

    return 0;
}
