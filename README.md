# CHOP-8 

CHOP-8 is a CHIP-8 emulator written in C\+\+. It was written as an 
introductory exercise in emulator programming, and has focus placed on 
compatibility with many common CHIP-8 programs. The emulator core is an 
interpreter, as I have yet to become adept with dynamic recompilation (dynarec) 
techniques.  
The core's only dependency is the C\+\+ Standard Library, and is independent of 
IO operations. Some code for basic cross-platform IO operations (excluding
audio, to be implemented later), based on the C\+\+ Standard Library and the 
[Simple DirectMedia Layer](https://wiki.libsdl.org/FrontPage) library (version
2.0, i.e. SDL2), is provided alongside a simple driver program as a primitive 
front-end to allow easy use/debugging of the emulator core.

## Building

For the emulator core alone, the only prerequisite is a hosted compiler 
implementation of C\+\+14 supporting uint8\_t, uint16\_t, and uint32\_t
(e.g. gcc with libstdc++ on x86\_64).  
To build the front-end in addition to this, SDL2 is also required 
\([install instructions here](https://wiki.libsdl.org/Installation)\).  
**UNDER CONSTRUCTION**

## References

* [Mastering CHIP-8, by Matthew Mikolay](http://mattmik.com/files/chip8/mastering/chip8.html)  
* [Cowgod's Chip-8 Technical Reference v1.0](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM)  

Given that there is no formal standard for CHIP-8 behavior (only the original 
implementation from the RCA COSMAC VIP by Joseph Weisbecker, as documented 
[here](http://laurencescotford.co.uk/?p=242)), CHIP-8 implementations/programs 
do not all conform to the same specification. Most conform either to the 
specification detailed in the first of the above links, reflecting the original 
CHIP-8 implementation, or the second, reflecting a 'newer' specification
differing in a few opcodes. CHOP-8 allows for specifying between the two when
running a program.

## Future Additions

* Add audio to front-end
* Better build support/documentation, e.g. CMake support 
* Thread safety (if not already satisfied)
* (Dis)assembler  
* Support for the Libretro API to use RetroArch as a front-end  
* Dynamic recompilation  

## Built With

* [SDL2](https://wiki.libsdl.org/FrontPage) - Basis of debug IO library


## Author

**Oliver Vecchini** - [olivecc](https://github.com/olivecc)  
License information can be found [here](LICENSE.txt).
