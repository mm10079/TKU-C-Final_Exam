#ifndef GAME_H
#define GAME_H

#include <stddef.h>

#ifdef _WIN32
#  define GAMEDLLAPI
#else
#  define GAMEDLLAPI
#endif

typedef struct GameHandle GameHandle;

GAMEDLLAPI GameHandle* game_init(int rows, int cols, int bomb_count);
GAMEDLLAPI int game_left_click(GameHandle* h, int r, int c);
GAMEDLLAPI int game_right_click(GameHandle* h, int r, int c);
GAMEDLLAPI int game_undo(GameHandle* h);
// buffer should be preallocated; returns number of bytes written
GAMEDLLAPI int game_get_state(GameHandle* h, char* buffer, size_t bufsize);
GAMEDLLAPI void game_free(GameHandle* h);

#endif // GAME_H
