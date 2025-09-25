# Pimoroni Cosmic Unicorn Games and Demos
The file to upload to your pico on the cosmic unicorn is build/cosmic_launcher.uf2.

Hold the button on the pico while you turn it on to put it in the correct bootsel mode. Then upload the uf2 to the drive that appears. 
the pico will reboot and begin displaying the launcher menu. 

Hold 'D' button during any animation/game to exit back to the launcher. 

Or for development work, use pictool intead: 

```shell
sudo /usr/local/bin/picotool load './build/cosmic_launcher.uf2' -f
```


The Games and Demos are:

- SPOOK - A halloween themed animation. 
- P-TYPE - An R-type inspired side scrolling shooter.
- RACE - An outrun inspired race game.
- FROG - A game of Cosmic Frogger.
- QIX - A version of Qix for the Cosmic Unicorn.
- BLOCKS - A simple playable version of tetris.
- PRETTY - Some colourful visual effects.


Most of these will play as an animation until you start interacting with the buttons. Hold D to return to Menu.


## Installation: 
Download the uf2 file from build here: https://github.com/mrkwllmsn/cosmic_laucher/blob/main/build/cosmic_launcher.uf2

Upload it to your pico. 


## Building 
You need the pico-sdk and associated pimoroni setup for building c. If you can build their c++ examples, you should be good to go to build this. 

