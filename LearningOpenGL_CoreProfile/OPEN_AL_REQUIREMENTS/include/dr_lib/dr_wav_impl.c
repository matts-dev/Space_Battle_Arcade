
//instructions for dr_lib's dr_wav require that you create a .c file and define a preprocessor macro before including the .h.
//this seems to generate function bodies for the linker to link against.

#define DR_WAV_IMPLEMENTATION
#include "dr_lib/dr_wav.h"

