# duo
Dual osc module for detune sound!  
For Detune, please reset `parameter 2` to 25.

# Parameters
- shape (NTS-1: A, other: Shape)  
Detune of SubOsc. (Center is neutral.)
- shiftshape (NTS-1: B, other: Shift+Shape)  
OSC2 mixing ratio.
- parameter 1  
Wave Type (1 .. 3)
  1. Tri
  1. Pulse
  1. Saw
- parameter 2  
Note offset of OSC2 (1 .. 25 .. 49) as (-24 .. 0 .. +24)
- parameter 3  
Portament (1 .. 101)

# How to build
1. Clone (or download) and setup [logue-sdk](https://github.com/korginc/logue-sdk).
1. Clone (or download) this project.
1. Change `PLATFORMDIR` of Makefile according to your environment.

# LICENSE
Copyright 2019 Tomoaki Itoh
This software is released under the MIT License, see LICENSE.txt.

# AUTHOR
Tomoaki Itoh(neko) techno.cat.miau@gmail.com
