#ifndef TETRIS_H__
#define TETRIS_H__

#include <stdint.h>
#include "MenuSystem.h"

int
tetris_run( bool init, uint32_t ms, Menu_Keys_t keys );

#endif //TETRIS_H__