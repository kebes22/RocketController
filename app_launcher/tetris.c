#include "tetris.h"
#include "sdk_common.h"

#include "GLCD.h"
#include "gfx/gfx.h"

#include <string.h>
#include <stdbool.h>


//#include <cstdlib>
//#include <cstdint>
//#include <cstdio>
//#include <cstring>
//#include <cassert>
//
//#include <SDL.h>
//#include <SDL_ttf.h>




typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef float f32;
typedef double f64;

//#include "colors.h"

#if 0
#define WIDTH 10
#define HEIGHT 22
#define VISIBLE_HEIGHT 20
#define GRID_SIZE 30
#elif 0
#define WIDTH 10
#define HEIGHT 16
#define VISIBLE_HEIGHT 16
//#define GRID_SIZE 4
#define GRID_WIDTH  5
#define GRID_HEIGHT 4
#define DRAW_GHOST	1
#elif 1
#define WIDTH 10
#define HEIGHT 20
#define VISIBLE_HEIGHT 20
//#define GRID_SIZE 4
#define GRID_WIDTH  4
#define GRID_HEIGHT 3
#define DRAW_GHOST	1
#else
#define WIDTH 10
#define HEIGHT 20
#define VISIBLE_HEIGHT 20
#define GRID_SIZE 3
#define DRAW_GHOST	1
#endif

#define ARRAY_COUNT(x) (sizeof(x) / sizeof((x)[0]))

const u8 FRAMES_PER_DROP[] = {
    48,
    43,
    38,
    33,
    28,
    23,
    18,
    13,
    8,
    6,
    5,
    5,
    5,
    4,
    4,
    4,
    3,
    3,
    3,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    1
};

#define NUM_LEVELS (sizeof(FRAMES_PER_DROP)/sizeof(FRAMES_PER_DROP[0]))

const f32 TARGET_SECONDS_PER_FRAME = 1.f / 60.f;

typedef struct Tetrino
{
    const u8 *data;
    const s32 side;
} Tetrino;


//inline Tetrino
//tetrino(const u8 *data, s32 side)
//{
//    return { data, side };
//}

const u8 TETRINO_1[] = {
    0, 0, 0, 0,
    1, 1, 1, 1,
    0, 0, 0, 0,
    0, 0, 0, 0
};

const u8 TETRINO_2[] = {
    2, 2,
    2, 2
};

const u8 TETRINO_3[] = {
    0, 3, 0,
    3, 3, 3,
    0, 0, 0
};

const u8 TETRINO_4[] = {
    0, 4, 4,
    4, 4, 0,
    0, 0, 0
};

const u8 TETRINO_5[] = {
    5, 5, 0,
    0, 5, 5,
    0, 0, 0
};

const u8 TETRINO_6[] = {
    6, 0, 0,
    6, 6, 6,
    0, 0, 0
};

const u8 TETRINO_7[] = {
    0, 0, 7,
    7, 7, 7,
    0, 0, 0
};


const Tetrino TETRINOS[] = {
    {TETRINO_1, 4},
    {TETRINO_2, 2},
    {TETRINO_3, 3},
    {TETRINO_4, 3},
    {TETRINO_5, 3},
    {TETRINO_6, 3},
    {TETRINO_7, 3},
};

typedef enum Game_Phase
{
    GAME_PHASE_START,
    GAME_PHASE_PLAY,
    GAME_PHASE_LINE,
    GAME_PHASE_GAMEOVER
} Game_Phase;

typedef struct
{
    u8 tetrino_index;
    s32 offset_row;
    s32 offset_col;
    s32 rotation;
} Piece_State;

typedef struct Game_State
{
    u8 board[WIDTH * HEIGHT];
    u8 lines[HEIGHT];
    s32 pending_line_count;

    Piece_State piece;

	uint8_t next_pieces[7];
	uint8_t next_count;
//    Piece_State next_piece;

    Game_Phase phase;

    s32 start_level;
    s32 level;
    s32 line_count;
    s32 points;

    f32 next_drop_time;
    f32 highlight_end_time;
    f32 time;
} Game_State;

typedef struct
{
    u8 left;
    u8 right;
    u8 up;
    u8 down;
    u8 a;
	u8 quit;
} Inputs;

typedef struct
{
	Inputs	on;
	Inputs	onEvent;
	Inputs	offEvent;
	Inputs	repeatEvent;
} Input_State;

//typedef enum Text_Align
//{
//    TEXT_ALIGN_LEFT,
//    TEXT_ALIGN_CENTER,
//    TEXT_ALIGN_RIGHT
//} Text_Align;

static inline u8
matrix_get(const u8 *values, s32 width, s32 row, s32 col)
{
    s32 index = row * width + col;
    return values[index];
}

static inline void
matrix_set(u8 *values, s32 width, s32 row, s32 col, u8 value)
{
    s32 index = row * width + col;
    values[index] = value;
}

static inline u8
tetrino_get(const Tetrino *tetrino, s32 row, s32 col, s32 rotation)
{
    s32 side = tetrino->side;
    switch (rotation)
    {
    case 0:
        return tetrino->data[row * side + col];
    case 1:
        return tetrino->data[(side - col - 1) * side + row];
    case 2:
        return tetrino->data[(side - row - 1) * side + (side - col - 1)];
    case 3:
        return tetrino->data[col * side + (side - row - 1)];
    }
    return 0;
}

static inline u8
//u8
check_row_filled(const u8 *values, s32 width, s32 row)
{
    for (s32 col = 0;
         col < width;
         ++col)
    {
        if (!matrix_get(values, width, row, col))
        {
            return 0;
        }
    }
    return 1;
}

static inline u8
check_row_empty(const u8 *values, s32 width, s32 row)
{
    for (s32 col = 0;
         col < width;
         ++col)
    {
        if (matrix_get(values, width, row, col))
        {
            return 0;
        }
    }
    return 1;
}

static s32
find_lines(const u8 *values, s32 width, s32 height, u8 *lines_out)
{
    s32 count = 0;
    for (s32 row = 0;
         row < height;
         ++row)
    {
        u8 filled = check_row_filled(values, width, row);
        lines_out[row] = filled;
        count += filled;
    }
    return count;
}

static void
clear_lines(u8 *values, s32 width, s32 height, const u8 *lines)
{
    s32 src_row = height - 1;
    for (s32 dst_row = height - 1;
         dst_row >= 0;
         --dst_row)
    {
        while (src_row >= 0 && lines[src_row])
        {
            --src_row;
        }

        if (src_row < 0)
        {
            memset(values + dst_row * width, 0, width);
        }
        else
        {
            if (src_row != dst_row)
            {
                memcpy(values + dst_row * width,
                       values + src_row * width,
                       width);
            }
            --src_row;
        }
    }
}


static bool
check_piece_valid(const Piece_State *piece,
                  const u8 *board, s32 width, s32 height)
{
    const Tetrino *tetrino = TETRINOS + piece->tetrino_index;
    ASSERT(tetrino);

    for (s32 row = 0;
         row < tetrino->side;
         ++row)
    {
        for (s32 col = 0;
             col < tetrino->side;
             ++col)
        {
            u8 value = tetrino_get(tetrino, row, col, piece->rotation);
            if (value > 0)
            {
                s32 board_row = piece->offset_row + row;
                s32 board_col = piece->offset_col + col;
                if (board_row < 0)
                {
                    return false;
                }
                if (board_row >= height)
                {
                    return false;
                }
                if (board_col < 0)
                {
                    return false;
                }
                if (board_col >= width)
                {
                    return false;
                }
                if (matrix_get(board, width, board_row, board_col))
                {
                    return false;
                }
            }
        }
    }
    return true;
}

static void
merge_piece(Game_State *game)
{
    const Tetrino *tetrino = TETRINOS + game->piece.tetrino_index;
    for (s32 row = 0;
         row < tetrino->side;
         ++row)
    {
        for (s32 col = 0;
             col < tetrino->side;
             ++col)
        {
            u8 value = tetrino_get(tetrino, row, col, game->piece.rotation);
            if (value)
            {
                s32 board_row = game->piece.offset_row + row;
                s32 board_col = game->piece.offset_col + col;
                matrix_set(game->board, WIDTH, board_row, board_col, value);
            }
        }
    }
}

static inline s32
random_int(s32 min, s32 max)
{
    s32 range = max - min;
    return min + rand() % range;
}

static inline f32
get_time_to_next_drop(s32 level)
{
    if (level > NUM_LEVELS-1)
    {
        level = NUM_LEVELS-1;
    }
    return FRAMES_PER_DROP[level] * TARGET_SECONDS_PER_FRAME;
}

static void
refill_piece_bag(Game_State *game)
{
	bool used;
    u8 next;
	for ( game->next_count = 0; game->next_count < 7; game->next_count++ ) {
		do {
			next = (u8)random_int(0, ARRAY_COUNT(TETRINOS));
			used = false;
			for ( int i = 0; i < game->next_count; i++ ) {
				if ( game->next_pieces[i] == next ) {
					used = true;
					break;
				}
			}
		} while ( used );
		game->next_pieces[game->next_count] = next;
	}
}

static void
spawn_piece(Game_State *game)
{
	Piece_State piece = { 0 };
	piece.tetrino_index = game->next_pieces[(game->next_count--)-1];
    piece.offset_col = WIDTH / 2 - 1;

    game->piece = piece;
    game->next_drop_time = game->time + get_time_to_next_drop(game->level);

	if ( game->next_count == 0 )
		refill_piece_bag(game);
}


static inline bool
soft_drop(Game_State *game)
{
    ++game->piece.offset_row;
    if (!check_piece_valid(&game->piece, game->board, WIDTH, HEIGHT))
    {
        --game->piece.offset_row;
        merge_piece(game);
        spawn_piece(game);
        return false;
    }

    game->next_drop_time = game->time + get_time_to_next_drop(game->level);
    return true;
}

static inline s32
compute_points(s32 level, s32 line_count)
{
    switch (line_count)
    {
    case 1:
        return 40 * (level + 1);
    case 2:
        return 100 * (level + 1);
    case 3:
        return 300 * (level + 1);
    case 4:
        return 1200 * (level + 1);
    }
    return 0;
}

static inline s32
min(s32 x, s32 y)
{
    return x < y ? x : y;
}

static inline s32
max(s32 x, s32 y)
{
    return x > y ? x : y;
}

static inline s32
get_lines_for_next_level(s32 start_level, s32 level)
{
    s32 first_level_up_limit = min(
        (start_level * 10 + 10),
        max(100, (start_level * 10 - 50)));
    if (level == start_level)
    {
        return first_level_up_limit;
    }
    s32 diff = level - start_level;
    return first_level_up_limit + diff * 10;
}

static void
update_game_start(Game_State *game, const Menu_Keys_t keys)
{
	rand();
    if (keys.anyOnEvent.up && game->start_level < NUM_LEVELS-1)
    {
        ++game->start_level;
    }

    if (keys.anyOnEvent.down && game->start_level > 0)
    {
        --game->start_level;
    }

    if (keys.onEvent.select)
    {
        memset(game->board, 0, WIDTH * HEIGHT);
        game->level = game->start_level;
        game->line_count = 0;
        game->points = 0;
        spawn_piece(game);
//        spawn_piece(game); // next peice
        game->phase = GAME_PHASE_PLAY;
    }
}

static void
update_game_gameover(Game_State *game, const Menu_Keys_t keys)
{
    if (keys.onEvent.select)
    {
        game->phase = GAME_PHASE_START;
    }
}

static void
update_game_line(Game_State *game)
{
    if (game->time >= game->highlight_end_time)
    {
        clear_lines(game->board, WIDTH, HEIGHT, game->lines);
        game->line_count += game->pending_line_count;
        game->points += compute_points(game->level, game->pending_line_count);

        s32 lines_for_next_level = get_lines_for_next_level(game->start_level,
                                                            game->level);
        if (game->line_count >= lines_for_next_level)
        {
            ++game->level;
        }

        game->phase = GAME_PHASE_PLAY;
    }
}

static void
update_game_play(Game_State *game,
                 const Menu_Keys_t keys)
{
    Piece_State piece = game->piece;
    if (keys.anyOnEvent.left)
    {
        --piece.offset_col;
    }
    if (keys.anyOnEvent.right)
    {
        ++piece.offset_col;
    }
    if (keys.anyOnEvent.up)
    {
        piece.rotation = (piece.rotation + 1) % 4;
		// Try wall kick
		//FIXME doesn't work for one orientation of I piece
		if ( !check_piece_valid(&piece, game->board, WIDTH, HEIGHT) ) {
			++piece.offset_col;	// Check one position to the right
			if ( !check_piece_valid(&piece, game->board, WIDTH, HEIGHT) ) {
				piece.offset_col -= 2;	// One position to left, This will be checked below
			}
		}
    }

    if (check_piece_valid(&piece, game->board, WIDTH, HEIGHT))
    {
        game->piece = piece;
    }

    if (keys.anyOnEvent.down)
    {
        soft_drop(game);
    }

    if (keys.onEvent.select)
    {
        while(soft_drop(game));
    }

    while (game->time >= game->next_drop_time)
    {
        soft_drop(game);
    }

    game->pending_line_count = find_lines(game->board, WIDTH, HEIGHT, game->lines);
    if (game->pending_line_count > 0)
    {
        game->phase = GAME_PHASE_LINE;
        game->highlight_end_time = game->time + 0.5f;
    }

    s32 game_over_row = 0;
    if (!check_row_empty(game->board, WIDTH, game_over_row))
    {
        game->phase = GAME_PHASE_GAMEOVER;
    }
}

static void
update_game(Game_State *game,
            const Menu_Keys_t keys)
{
    switch(game->phase)
    {
    case GAME_PHASE_START:
        update_game_start(game, keys);
        break;
    case GAME_PHASE_PLAY:
        update_game_play(game, keys);
        break;
    case GAME_PHASE_LINE:
        update_game_line(game);
        break;
    case GAME_PHASE_GAMEOVER:
        update_game_gameover(game, keys);
        break;
    }
}


//################################################################################
//	Graphics stuff
//################################################################################


#if 0
void
fill_rect(SDL_Renderer *renderer,
          s32 x, s32 y, s32 width, s32 height, Color color)
{
    SDL_Rect rect = {};
    rect.x = x;
    rect.y = y;
    rect.w = width;
    rect.h = height;
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
}


void
draw_rect(SDL_Renderer *renderer,
          s32 x, s32 y, s32 width, s32 height, Color color)
{
    SDL_Rect rect = {};
    rect.x = x;
    rect.y = y;
    rect.w = width;
    rect.h = height;
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawRect(renderer, &rect);
}

void
draw_string(SDL_Renderer *renderer,
            TTF_Font *font,
            const char *text,
            s32 x, s32 y,
            Text_Align alignment,
            Color color)
{
    SDL_Color sdl_color = SDL_Color { color.r, color.g, color.b, color.a };
    SDL_Surface *surface = TTF_RenderText_Solid(font, text, sdl_color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_Rect rect;
    rect.y = y;
    rect.w = surface->w;
    rect.h = surface->h;
    switch (alignment)
    {
    case TEXT_ALIGN_LEFT:
        rect.x = x;
        break;
    case TEXT_ALIGN_CENTER:
        rect.x = x - surface->w / 2;
        break;
    case TEXT_ALIGN_RIGHT:
        rect.x = x - surface->w;
        break;
    }

    SDL_RenderCopy(renderer, texture, 0, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void
draw_cell(SDL_Renderer *renderer,
          s32 row, s32 col, u8 value,
          s32 offset_x, s32 offset_y,
          bool outline = false)
{
    Color base_color = BASE_COLORS[value];
    Color light_color = LIGHT_COLORS[value];
    Color dark_color = DARK_COLORS[value];


    s32 edge = GRID_SIZE / 8;

    s32 x = col * GRID_SIZE + offset_x;
    s32 y = row * GRID_SIZE + offset_y;

    if (outline)
    {
        draw_rect(renderer, x, y, GRID_SIZE, GRID_SIZE, base_color);
        return;
    }

    fill_rect(renderer, x, y, GRID_SIZE, GRID_SIZE, dark_color);
    fill_rect(renderer, x + edge, y,
              GRID_SIZE - edge, GRID_SIZE - edge, light_color);
    fill_rect(renderer, x + edge, y + edge,
              GRID_SIZE - edge * 2, GRID_SIZE - edge * 2, base_color);
}

#else

typedef struct
{
	void		* empty;
} SDL_Renderer;

typedef struct
{

} TTF_Font;

typedef uint32_t Color;

typedef enum
{
	TEXT_ALIGN_LEFT = ALIGN_LEFT,
	TEXT_ALIGN_CENTER = ALIGN_CENTER,
	TEXT_ALIGN_RIGHT = ALIGN_RIGHT,
} Text_Align;

static void
fill_rect(SDL_Renderer *renderer,
          s32 x, s32 y, s32 width, s32 height, Color color)
{
    GLCD_DrawFill( x, y, GFX_DIM_TO_P2(x,width), GFX_DIM_TO_P2(y,height), color );
}


static void
draw_rect(SDL_Renderer *renderer,
          s32 x, s32 y, s32 width, s32 height, Color color)
{
    GLCD_PlotRect( x, y, GFX_DIM_TO_P2(x,width), GFX_DIM_TO_P2(y,height) );
}

static void
draw_string(SDL_Renderer *renderer,
            TTF_Font *font,
            const char *text,
            s32 x, s32 y,
            Text_Align alignment,
            Color color)
{
	GLCD_SetMode(color);
	GLCD_Goto(x,y);
    GLCD_PutStringAligned( text, (gAlignment_e)alignment );
	GLCD_SetMode(DRAW_ON);
}

static void
draw_cell(SDL_Renderer *renderer,
          s32 row, s32 col, u8 value,
          s32 offset_x, s32 offset_y,
          bool outline)
{
    s32 x = col * GRID_WIDTH + offset_x;
    s32 y = row * GRID_HEIGHT + offset_y;

    if (outline)
    {
        draw_rect(renderer, x, y, GRID_WIDTH, GRID_HEIGHT, DRAW_ON);
        return;
    }

	fill_rect(renderer, x, y, GRID_WIDTH, GRID_HEIGHT, DRAW_ON);
}

#endif

static void
draw_piece(SDL_Renderer *renderer,
           const Piece_State *piece,
           s32 offset_x, s32 offset_y,
           bool outline)
{
    const Tetrino *tetrino = TETRINOS + piece->tetrino_index;
    for (s32 row = 0;
         row < tetrino->side;
         ++row)
    {
        for (s32 col = 0;
             col < tetrino->side;
             ++col)
        {
            u8 value = tetrino_get(tetrino, row, col, piece->rotation);
            if (value)
            {
                draw_cell(renderer,
                          row + piece->offset_row,
                          col + piece->offset_col,
                          value,
                          offset_x, offset_y,
                          outline);
            }
        }
    }
}

static void
draw_board(SDL_Renderer *renderer,
           const u8 *board, s32 width, s32 height,
           s32 offset_x, s32 offset_y)
{
	draw_rect(renderer, offset_x-1, offset_y-1, width * GRID_WIDTH + 2, height * GRID_HEIGHT + 2, DRAW_ON );

    for (s32 row = 0;
         row < height;
         ++row)
    {
        for (s32 col = 0;
             col < width;
             ++col)
        {
            u8 value = matrix_get(board, width, row, col);
            if (value)
            {
                draw_cell(renderer, row, col, value, offset_x, offset_y, false);
            }
        }
    }
}

static void
render_game(const Game_State *game,
            SDL_Renderer *renderer,
            TTF_Font *font)
{

    char buffer[32];

//    Color highlight_color = color(0xFF, 0xFF, 0xFF, 0xFF);
    Color highlight_color = DRAW_XOR;

//    s32 margin_y = 60;
    s32 margin_y = 0;
    s32 margin_x = 64 - (WIDTH * GRID_WIDTH / 2);

    draw_board(renderer, game->board, WIDTH, HEIGHT, margin_x, margin_y);

    if (game->phase == GAME_PHASE_PLAY)
    {
        draw_piece(renderer, &game->piece, margin_x, margin_y, false);

#if DRAW_GHOST
        Piece_State piece = game->piece;
        while (check_piece_valid(&piece, game->board, WIDTH, HEIGHT))
        {
            piece.offset_row++;
        }
        --piece.offset_row;
        draw_piece(renderer, &piece, margin_x, margin_y, true);
#endif
    }

	// draw next piece
	if (game->phase == GAME_PHASE_PLAY ||game->phase == GAME_PHASE_LINE) {
		Piece_State piece = { 0 };
		piece.tetrino_index = game->next_pieces[game->next_count-1];
		s32 x = margin_x + (WIDTH * GRID_WIDTH) + 4;
		s32 y = 0;
		snprintf(buffer, sizeof(buffer), "NEXT");
		draw_string(renderer, font, buffer, x, y, TEXT_ALIGN_LEFT, highlight_color);
		y+=8;
		s32 xdim = GRID_WIDTH * 4 + 4;
		s32 ydim = GRID_HEIGHT * 4 + 4;
		draw_rect( renderer, x, y, xdim, ydim, DRAW_ON );
		const Tetrino *tetrino = TETRINOS + piece.tetrino_index;
		s32 xoffset = (4-tetrino->side) * GRID_WIDTH / 2;
		s32 yoffset = (4-tetrino->side) * GRID_HEIGHT / 2;
		draw_piece(renderer, &piece, x+2+xoffset, y+2+yoffset, false);
	}

    if (game->phase == GAME_PHASE_LINE)
    {
        for (s32 row = 0;
             row < HEIGHT;
             ++row)
        {
            if (game->lines[row])
            {
                s32 x = margin_x;
                s32 y = row * GRID_HEIGHT + margin_y;
                fill_rect(renderer, x, y, WIDTH * GRID_WIDTH, GRID_HEIGHT, highlight_color);
            }
        }
    }
    else if (game->phase == GAME_PHASE_GAMEOVER)
    {
        s32 x = 64;
        s32 y = 48;
        draw_string(renderer, font, "GAME OVER",
                    x, y, TEXT_ALIGN_CENTER, highlight_color);
    }
    else if (game->phase == GAME_PHASE_START)
    {
        s32 x = 64;
        s32 y = 48;
        draw_string(renderer, font, "PRESS START",
                    x, y, TEXT_ALIGN_CENTER, highlight_color);

        snprintf(buffer, sizeof(buffer), "STARTING LEVEL: %d", game->start_level);
        draw_string(renderer, font, buffer,
                    x, y + 8, TEXT_ALIGN_CENTER, highlight_color);
    }

    fill_rect(renderer, margin_x, margin_y,
              WIDTH * GRID_WIDTH, (HEIGHT - VISIBLE_HEIGHT) * GRID_HEIGHT,
              DRAW_OFF);

#if 1
	s32 x = 0;
	s32 y = 0;
    s32 h = 8;
	snprintf(buffer, sizeof(buffer), "LEVEL");
    draw_string(renderer, font, buffer, x, y, TEXT_ALIGN_LEFT, highlight_color);
    y+=h;
	snprintf(buffer, sizeof(buffer), "%d", game->level);
    draw_string(renderer, font, buffer, x, y, TEXT_ALIGN_LEFT, highlight_color);
    y+=h;
    snprintf(buffer, sizeof(buffer), "LINES");
    draw_string(renderer, font, buffer, x, y, TEXT_ALIGN_LEFT, highlight_color);
    y+=h;
    snprintf(buffer, sizeof(buffer), "%d", game->line_count);
    draw_string(renderer, font, buffer, x, y, TEXT_ALIGN_LEFT, highlight_color);
    y+=h;
    snprintf(buffer, sizeof(buffer), "POINTS");
    draw_string(renderer, font, buffer, x, y, TEXT_ALIGN_LEFT, highlight_color);
    y+=h;
    snprintf(buffer, sizeof(buffer), "%d", game->points);
    draw_string(renderer, font, buffer, x, y, TEXT_ALIGN_LEFT, highlight_color);
#endif
}

int
tetris_run( bool init, uint32_t ms, Menu_Keys_t keys )
{
	static Game_State game = { 0 };
	static Input_State input = { 0 };
	static bool quit = false;
	if ( init ) {
		memset( &game, 0, sizeof(game) );
		memset( &input, 0, sizeof(input) );
		refill_piece_bag(&game);
		spawn_piece(&game);
		game.piece.tetrino_index = 2;
		quit = false;
	}

	game.time += (float)ms / 1000.0f;

	if ( keys.repeatEvent.back ) { // back held
		return 0;	// done
	}

	update_game(&game, keys);
    fill_rect(NULL, 0, 0, 128, 64, DRAW_OFF);
	render_game(&game, NULL, NULL);

	return 1;		// busy
}


