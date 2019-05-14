#!/bin/sh

arecord -f S16_LE -r 44100 -c 2 -D hw:1,0 - | aplay -Dhw:0,0 -
