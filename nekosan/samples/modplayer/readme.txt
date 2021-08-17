To run the mod player sample:

1) Format an SD Card to FAT32
2) Copy the test.mod file to the root directory
3) Plug the microSD PMOD to PORT C of the Arty A7 board
4) Plug the i2s2 stereo audio PMOD to PORT D of the Arty A7 board
5) Plug headphones/speakers to the green output connector of the i2s2 board (WARNING! Watch out for volume!)
6) Reset the device using board reset button (upper left corner)
7) Finally, use the following command from a terminal to send the modplayer.elf to the SoC
   build/release/riscvtool modplayer.elf -sendelf 0x10000 /dev/ttyUSB1
   where ttyUSB1 should match the USB port that your A7 board is attached to
8) You should now see 'Playing' message and some information about the mod file on the terminal
9) After the message, audio playback will start. Gradually increase the volume on your headphones/speakers to hear the music.
