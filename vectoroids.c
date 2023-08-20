/*
  vectoroids.c

  An asteroid shooting game with vector graphics.
  Based on "Agendaroids."

  by Bill Kendrick
  bill@newbreedsoftware.com
  http://www.newbreedsoftware.com/vectoroids/

  November 30, 2001 - April 20, 2002

  > -------------------------------------------- <

  SDL2 port by Marc-Alexandre Espiaut, for Logicoq
  malespiaut.dev@posteo.eu
  http://logicoq.free.fr

  August 17, 2023

*/

#define kGameName "Vectoroids"
#define kGameVersion "1.2.0"
#define kGameDate "2023.08.17"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef DATA_PREFIX
#define DATA_PREFIX "data/"
#endif

/* Constraints: */

#define kNumBullets 2

#define kNumAsteroids 20
#define kNumBits 50

#define kAsteroidsSides 6
#define kAsteroidsRadius 10
#define kShipRadius 20

#define kZoomStart 40
#define kOneUpScore 10000
#define kScreenFPS 60

#define kScreenWidth 480
#define kScreenHeight 480

#define LEFT_EDGE 0x0001
#define RIGHT_EDGE 0x0002
#define TOP_EDGE 0x0004
#define BOTTOM_EDGE 0x0008

/* Types: */

typedef struct letter_type
{
  size_t x, y;
  int32_t xm, ym;
} letter_type;

typedef struct bullet_type
{
  int32_t timer;
  int32_t x, y;
  int32_t xm, ym;
} bullet_type;

typedef struct shape_type
{
  int32_t radius;
  int32_t angle;
} shape_type;

typedef struct asteroid_type
{
  int32_t alive, size;
  int32_t x, y;
  int32_t xm, ym;
  int32_t angle, angle_m;
  shape_type shape[kAsteroidsSides];
} asteroid_type;

typedef struct bit_type
{
  int32_t timer;
  int32_t x, y;
  int32_t xm, ym;
} bit_type;

typedef struct color_type
{
  uint8_t r;
  uint8_t g;
  uint8_t b;
} color_type;

/* Data: */

enum
{
  SND_BULLET,
  SND_AST1,
  SND_AST2,
  SND_AST3,
  SND_AST4,
  SND_THRUST,
  SND_EXPLODE,
  SND_GAMEOVER,
  SND_EXTRALIFE,
  NUM_SOUNDS
};

char* sound_names[NUM_SOUNDS] = {
  DATA_PREFIX "sounds/bullet.wav",
  DATA_PREFIX "sounds/ast1.wav",
  DATA_PREFIX "sounds/ast2.wav",
  DATA_PREFIX "sounds/ast3.wav",
  DATA_PREFIX "sounds/ast4.wav",
  DATA_PREFIX "sounds/thrust.wav",
  DATA_PREFIX "sounds/explode.wav",
  DATA_PREFIX "sounds/gameover.wav",
  DATA_PREFIX "sounds/extralife.wav"};

#define CHAN_THRUST 0

char* mus_game_name = DATA_PREFIX "music/decision.s3m";

#ifdef JOY_YES
#define JOY_A 0
#define JOY_B 1
#define JOY_X 0
#define JOY_Y 1
#endif

/* Globals: */

SDL_Window* g_window = 0;
SDL_Renderer* g_renderer = 0;
SDL_Texture* g_texture = 0;
Mix_Chunk* sounds[NUM_SOUNDS] = {0};
Mix_Music* game_music = 0;
#ifdef JOY_YES
SDL_Joystick* js = 0;
#endif
bullet_type bullets[kNumBullets] = {0};
asteroid_type asteroids[kNumAsteroids] = {0};
bit_type bits[kNumBits] = {0};
bool use_sound = true, use_joystick = false, fullscreen = false;
int32_t text_zoom = 0;
char zoom_str[24] = {0};
int32_t player_x = 0, player_y = 0, player_xm = 0, player_ym = 0, player_angle = 0;
int32_t player_alive = 0, player_die_timer = 0;
size_t lives = 0, score = 0, high = 0, level = 0;
bool game_pending = false;

/* Trig junk:  (thanks to Atari BASIC for this) */

int32_t trig[12] = {
  1024,
  1014,
  984,
  935,
  868,
  784,
  685,
  572,
  448,
  316,
  117,
  0};

/* Characters: */

int32_t char_vectors[36][5][4] = {
  {/* 0 */
   {0, 0, 1, 0},
   {1, 0, 1, 2},
   {1, 2, 0, 2},
   {0, 2, 0, 0},
   {-1, -1, -1, -1}},

  {/* 1 */
   {1, 0, 1, 2},
   {-1, -1, -1, -1},
   {-1, -1, -1, -1},
   {-1, -1, -1, -1},
   {-1, -1, -1, -1}},

  {
    /* 2 */
    {1, 0, 0, 0},
    {1, 0, 1, 1},
    {0, 1, 1, 1},
    {0, 1, 0, 2},
    {1, 2, 0, 2},
  },

  {/* 3 */
   {0, 0, 1, 0},
   {1, 0, 1, 2},
   {0, 1, 1, 1},
   {0, 2, 1, 2},
   {-1, -1, -1, -1}},

  {/* 4 */
   {1, 0, 1, 2},
   {0, 0, 0, 1},
   {0, 1, 1, 1},
   {-1, -1, -1, -1},
   {-1, -1, -1, -1}},

  {/* 5 */
   {1, 0, 0, 0},
   {0, 0, 0, 1},
   {0, 1, 1, 1},
   {1, 1, 1, 2},
   {1, 2, 0, 2}},

  {/* 6 */
   {1, 0, 0, 0},
   {0, 0, 0, 2},
   {0, 2, 1, 2},
   {1, 2, 1, 1},
   {1, 1, 0, 1}},

  {/* 7 */
   {0, 0, 1, 0},
   {1, 0, 1, 2},
   {-1, -1, -1, -1},
   {-1, -1, -1, -1},
   {-1, -1, -1, -1}},

  {/* 8 */
   {0, 0, 1, 0},
   {0, 0, 0, 2},
   {1, 0, 1, 2},
   {0, 2, 1, 2},
   {0, 1, 1, 1}},

  {/* 9 */
   {1, 0, 1, 2},
   {0, 0, 1, 0},
   {0, 0, 0, 1},
   {0, 1, 1, 1},
   {-1, -1, -1, -1}},

  {/* A */
   {0, 2, 0, 1},
   {0, 1, 1, 0},
   {1, 0, 1, 2},
   {0, 1, 1, 1},
   {-1, -1, -1, -1}},

  {/* B */
   {0, 2, 0, 0},
   {0, 0, 1, 0},
   {1, 0, 0, 1},
   {0, 1, 1, 2},
   {1, 2, 0, 2}},

  {/* C */
   {1, 0, 0, 0},
   {0, 0, 0, 2},
   {0, 2, 1, 2},
   {-1, -1, -1, -1},
   {-1, -1, -1, -1}},

  {/* D */
   {0, 0, 1, 1},
   {1, 1, 0, 2},
   {0, 2, 0, 0},
   {-1, -1, -1, -1},
   {-1, -1, -1, -1}},

  {/* E */
   {1, 0, 0, 0},
   {0, 0, 0, 2},
   {0, 2, 1, 2},
   {0, 1, 1, 1},
   {-1, -1, -1, -1}},

  {/* F */
   {1, 0, 0, 0},
   {0, 0, 0, 2},
   {0, 1, 1, 1},
   {-1, -1, -1, -1},
   {-1, -1, -1, -1}},

  {/* G */
   {1, 0, 0, 0},
   {0, 0, 0, 2},
   {0, 2, 1, 2},
   {1, 2, 1, 1},
   {-1, -1, -1, -1}},

  {/* H */
   {0, 0, 0, 2},
   {1, 0, 1, 2},
   {0, 1, 1, 1},
   {-1, -1, -1, -1},
   {-1, -1, -1, -1}},

  {/* I */
   {1, 0, 1, 2},
   {-1, -1, -1, -1},
   {-1, -1, -1, -1},
   {-1, -1, -1, -1},
   {-1, -1, -1, -1}},

  {/* J */
   {1, 0, 1, 2},
   {1, 2, 0, 2},
   {0, 2, 0, 1},
   {-1, -1, -1, -1},
   {-1, -1, -1, -1}},

  {/* K */
   {0, 0, 0, 2},
   {1, 0, 0, 1},
   {0, 1, 1, 2},
   {-1, -1, -1, -1},
   {-1, -1, -1, -1}},

  {/* L */
   {0, 0, 0, 2},
   {0, 2, 1, 2},
   {-1, -1, -1, -1},
   {-1, -1, -1, -1},
   {-1, -1, -1, -1}},

  {/* M */
   {0, 0, 0, 2},
   {1, 0, 1, 2},
   {0, 0, 1, 1},
   {0, 1, 1, 0},
   {-1, -1, -1, -1}},

  {/* N */
   {0, 2, 0, 0},
   {0, 0, 1, 2},
   {1, 2, 1, 0},
   {-1, -1, -1, -1},
   {-1, -1, -1, -1}},

  {/* O */
   {0, 0, 1, 0},
   {1, 0, 1, 2},
   {1, 2, 0, 2},
   {0, 2, 0, 0},
   {-1, -1, -1, -1}},

  {/* P */
   {0, 2, 0, 0},
   {0, 0, 1, 0},
   {1, 0, 1, 1},
   {1, 1, 0, 1},
   {-1, -1, -1, -1}},

  {/* Q */
   {0, 0, 1, 0},
   {1, 0, 1, 2},
   {1, 2, 0, 2},
   {0, 2, 0, 0},
   {0, 1, 1, 2}},

  {/* R */
   {0, 2, 0, 0},
   {0, 0, 1, 0},
   {1, 0, 1, 1},
   {1, 1, 0, 1},
   {0, 1, 1, 2}},

  {/* S */
   {1, 0, 0, 0},
   {0, 0, 0, 1},
   {0, 1, 1, 1},
   {1, 1, 1, 2},
   {1, 2, 0, 2}},

  {/* T */
   {0, 0, 1, 0},
   {1, 0, 1, 2},
   {-1, -1, -1, -1},
   {-1, -1, -1, -1},
   {-1, -1, -1, -1}},

  {/* U */
   {0, 0, 0, 2},
   {0, 2, 1, 2},
   {1, 2, 1, 0},
   {-1, -1, -1, -1},
   {-1, -1, -1, -1}},

  {/* V */
   {0, 0, 0, 1},
   {0, 1, 1, 2},
   {1, 2, 1, 0},
   {-1, -1, -1, -1},
   {-1, -1, -1, -1}},

  {/* W */
   {0, 0, 0, 2},
   {1, 0, 1, 2},
   {0, 1, 1, 2},
   {0, 2, 1, 1},
   {-1, -1, -1, -1}},

  {/* X */
   {0, 0, 1, 2},
   {0, 2, 1, 0},
   {-1, -1, -1, -1},
   {-1, -1, -1, -1},
   {-1, -1, -1, -1}},

  {/* Y */
   {0, 0, 1, 1},
   {1, 0, 1, 2},
   {-1, -1, -1, -1},
   {-1, -1, -1, -1},
   {-1, -1, -1, -1}},

  {/* Z */
   {0, 0, 1, 0},
   {1, 0, 0, 2},
   {0, 2, 1, 2},
   {-1, -1, -1, -1},
   {-1, -1, -1, -1}}};

/* Local function prototypes: */

bool title(void);
bool game(void);
void finish(void);
void setup(const int argc, const char* argv[]);
int32_t fast_cos(int32_t v);
int32_t fast_sin(int32_t v);
void draw_line(int32_t x1, int32_t y1, color_type c1, int32_t x2, int32_t y2, color_type c2);
int32_t clip(int32_t* x1, int32_t* y1, int32_t* x2, int32_t* y2);
color_type mkcolor(int32_t r, int32_t g, int32_t b);
void sdl_drawline(int32_t x1, int32_t y1, color_type c1, int32_t x2, int32_t y2, color_type c2);
uint8_t encode(double x, double y);
void drawvertline(int32_t x, int32_t y1, color_type c1, int32_t y2, color_type c2);
void putpixel(int32_t x, int32_t y, color_type color);
void draw_segment(int32_t r1, int32_t a1, color_type c1, int32_t r2, int32_t a2, color_type c2, int32_t cx, int32_t cy, int32_t ang);
void add_bullet(int32_t x, int32_t y, int32_t a, int32_t xm, int32_t ym);
void add_asteroid(int32_t x, int32_t y, int32_t xm, int32_t ym, int32_t size);
void add_bit(int32_t x, int32_t y, int32_t xm, int32_t ym);
void draw_asteroid(int32_t size, int32_t x, int32_t y, int32_t angle, shape_type* shape);
void playsound(int32_t snd);
void hurt_asteroid(int32_t j, int32_t xm, int32_t ym, size_t exp_size);
void add_score(int32_t amount);
void draw_char(char c, int32_t x, int32_t y, int32_t r, color_type cl);
void draw_text(char* str, int32_t x, int32_t y, int32_t s, color_type c);
void draw_thick_line(int32_t x1, int32_t y1, color_type c1, int32_t x2, int32_t y2, color_type c2);
void reset_level(void);
void show_version(void);
void show_usage(FILE* f, const char* prg);
void draw_centered_text(char* str, int32_t y, int32_t s, color_type c);
const char* user_file_path_get(const char* file_name);

/* File manipulation */

const char*
user_file_path_get(const char* file_name)
{
  static char path[1024] = {0};
  const char* user_dir = SDL_GetPrefPath("Logicoq", kGameName);

  SDL_snprintf(path, sizeof(path), "%s%s", user_dir, file_name);
  return path;
}

/* --- MAIN --- */

int
main(const int argc, const char* argv[])
{
  setup(argc, argv);

  /* Load state from disk: */

  const char* statefile = user_file_path_get("vectoroids-state");

  FILE* fi = fopen(statefile, "r");
  if (!fi)
  {
    perror("fopen");
  }
  else
  {
    char buf[256] = {0};

    /* Skip comment line: */

    if (!fgets(buf, sizeof(buf), fi))
    {
      fprintf(stderr, "fgets: Error\n");
      exit(EXIT_FAILURE);
    }

    /* Grab statefile version: */

    if (!fgets(buf, sizeof(buf), fi))
    {
      fprintf(stderr, "fgets: Error\n");
      exit(EXIT_FAILURE);
    }
    buf[strlen(buf) - 1] = '\0';

    if (strncmp(buf, kGameDate, 10))
    {
      fprintf(stderr, "strncmp: %s state file format has been updated.\n"
                      "Old game state is unreadable.  Sorry!\n",
              kGameName);
    }
    else
    {
      game_pending = fgetc(fi);
      lives = fgetc(fi);
      level = fgetc(fi);
      player_alive = fgetc(fi);
      player_die_timer = fgetc(fi);
      fread(&score, sizeof(int), 1, fi);
      fread(&high, sizeof(int), 1, fi);
      fread(&player_x, sizeof(int), 1, fi);
      fread(&player_y, sizeof(int), 1, fi);
      fread(&player_xm, sizeof(int), 1, fi);
      fread(&player_ym, sizeof(int), 1, fi);
      fread(&player_angle, sizeof(int), 1, fi);
      fread(bullets, sizeof(bullet_type), kNumBullets, fi);
      fread(asteroids, sizeof(asteroid_type), kNumAsteroids, fi);
      fread(bits, sizeof(bit_type), kNumBits, fi);
    }

    if (fclose(fi))
    {
      perror("fclose");
      exit(EXIT_FAILURE);
    }
  }

  /* Main app loop! */

  bool done = false;
  while (!done)
  {
    done = title();

    if (!done)
    {
      done = game();
    }
  }

  /* Save state: */

  fi = fopen(statefile, "w");
  if (!fi)
  {
    perror("fopen");
  }
  else
  {
    fprintf(fi, "%s State File\n", kGameName);
    fprintf(fi, "%s\n", kGameDate);

    fputc(game_pending, fi);
    fputc(lives, fi);
    fputc(level, fi);
    fputc(player_alive, fi);
    fputc(player_die_timer, fi);
    fwrite(&score, sizeof(int), 1, fi);
    fwrite(&high, sizeof(int), 1, fi);
    fwrite(&player_x, sizeof(int), 1, fi);
    fwrite(&player_y, sizeof(int), 1, fi);
    fwrite(&player_xm, sizeof(int), 1, fi);
    fwrite(&player_ym, sizeof(int), 1, fi);
    fwrite(&player_angle, sizeof(int), 1, fi);
    fwrite(bullets, sizeof(bullet_type), kNumBullets, fi);
    fwrite(asteroids, sizeof(asteroid_type), kNumAsteroids, fi);
    fwrite(bits, sizeof(bit_type), kNumBits, fi);

    if (fclose(fi))
    {
      perror("fclose");
      exit(EXIT_FAILURE);
    }
  }

  finish();

  return EXIT_SUCCESS;
}

/* Title screen: */

bool
title(void)
{
  bool quit = false;
  const char* titlestr = "VECTOROIDS";

  /* Reset letters: */

  letter_type letters[11] = {0};
  for (size_t i = 0; i < strlen(titlestr); i++)
  {
    letters[i].x = (rand() % kScreenWidth);
    letters[i].y = (rand() % kScreenHeight);
    letters[i].xm = 0;
    letters[i].ym = 0;
  }

  int32_t x = (rand() % kScreenWidth);
  int32_t y = (rand() % kScreenHeight);
  int32_t xm = (rand() % 4) + 2;
  int32_t ym = (rand() % 10) - 5;

  int32_t size = 40;

  bool done = false;
  int32_t angle = 0;
  for (size_t counter = 0; !done; ++counter)
  {
    Uint64 last_time = SDL_GetTicks64();

    /* Rotate rock: */

    angle = ((angle + 2) % 360);

    /* Make rock grow: */

    if (!(counter % 3))
    {
      if (size > 1)
      {
        --size;
      }
    }

    /* Move rock: */

    x += xm;

    if (x >= kScreenWidth)
    {
      x -= kScreenWidth;
    }

    y += ym;

    if (y >= kScreenHeight)
    {
      y -= kScreenHeight;
    }
    else if (y < 0)
    {
      y += kScreenHeight;
    }

    /* Handle events: */

    SDL_Event event = {0};
    while (SDL_PollEvent(&event) > 0)
    {
      if (event.type == SDL_QUIT)
      {
        done = true;
        quit = true;
      }
      else if (event.type == SDL_KEYDOWN)
      {
        switch (event.key.keysym.scancode)
        {
          case SDL_SCANCODE_SPACE:
            done = true;
            break;
          case SDL_SCANCODE_ESCAPE:
            done = true;
            quit = true;
            break;
          default:
            break;
        }
      }
#ifdef JOY_YES
      else if (event.type == SDL_JOYBUTTONDOWN)
      {
        done = true;
      }
#endif
      else if (event.type == SDL_MOUSEBUTTONDOWN)
      {
        if (event.button.x >= (kScreenWidth - 50) / 2 && event.button.x <= (kScreenWidth + 50) / 2 && event.button.y >= 180 && event.button.y <= 195)
        {
          /* Start! */

          game_pending = false;
          done = true;
        }
        else if (event.button.x >= (kScreenWidth - 80) / 2 && event.button.x <= (kScreenWidth + 80) / 2 && event.button.y >= 200 && event.button.y <= 215 && game_pending)
        {
          done = true;
        }
      }
    }

    /* Move title characters: */

    size_t snapped = 0;
    if (snapped < strlen(titlestr))
    {
      for (size_t i = 0; i < strlen(titlestr); ++i)
      {
        letters[i].x = letters[i].x + letters[i].xm;
        letters[i].y = letters[i].y + letters[i].ym;

        /* Home in on final spot! */

        if (letters[i].x > ((kScreenWidth - (strlen(titlestr) * 14)) / 2 + (i * 14)) && letters[i].xm > -4)
        {
          letters[i].xm--;
        }
        else if (letters[i].x < ((kScreenWidth - (strlen(titlestr) * 14)) / 2 + (i * 14)) && letters[i].xm < 4)
        {
          letters[i].xm++;
        }

        if (letters[i].y > 100 && letters[i].ym > -4)
        {
          letters[i].ym--;
        }
        else if (letters[i].y < 100 && letters[i].ym < 4)
        {
          letters[i].ym++;
        }

        /* Snap into place: */

        if (letters[i].x >= ((kScreenWidth - (strlen(titlestr) * 14)) / 2 + (i * 14)) - 8 && letters[i].x <= ((kScreenWidth - (strlen(titlestr) * 14)) / 2 + (i * 14)) + 8 && letters[i].y >= 92 && letters[i].y <= 108 && (letters[i].xm != 0 || letters[i].ym != 0))
        {
          letters[i].x = ((kScreenWidth - (strlen(titlestr) * 14)) / 2 + (i * 14));
          letters[i].xm = 0;

          letters[i].y = 100;
          letters[i].ym = 0;

          ++snapped;
        }
      }
    }

    /* Draw screen: */

    /* (Erase first) */

    if (SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255))
    {
      fprintf(stderr, "SDL_SetRenderDrawControl: %s\n", SDL_GetError());
      exit(EXIT_FAILURE);
    }
    if (SDL_RenderClear(g_renderer))
    {
      fprintf(stderr, "SDL_RenderClear: %s\n", SDL_GetError());
      exit(EXIT_FAILURE);
    }

    /* (Title) */

    if (snapped != strlen(titlestr))
    {
      for (size_t i = 0; i < strlen(titlestr); i++)
      {
        draw_char(titlestr[i], letters[i].x, letters[i].y, 10, mkcolor(255, 255, 255));
      }
    }
    else
    {
      for (size_t i = 0; i < strlen(titlestr); i++)
      {
        int32_t z1 = (i + counter) % 255;
        int32_t z2 = ((i + counter + 128) * 2) % 255;
        int32_t z3 = ((i + counter) * 5) % 255;

        draw_char(titlestr[i], letters[i].x, letters[i].y, 10, mkcolor(z1, z2, z3));
      }
    }

    /* (Credits) */

    if (snapped == strlen(titlestr))
    {
      draw_centered_text("BY BILL KENDRICK", 140, 5, mkcolor(128, 128, 128));
      draw_centered_text("NEW BREED SOFTWARE", 155, 5, mkcolor(96, 96, 96));

      char str[20] = {0};

      sprintf(str, "HIGH %.6ld", high);
      draw_text(str, (kScreenWidth - 110) / 2, 5, 5, mkcolor(128, 255, 255));
      draw_text(str, (kScreenWidth - 110) / 2 + 1, 6, 5, mkcolor(128, 255, 255));

      if (score && (score != high || (counter % 20) < 10))
      {
        if (!game_pending)
        {
          sprintf(str, "LAST %.6ld", score);
        }
        else
        {
          sprintf(str, "SCR  %.6ld", score);
        }
        draw_text(str, (kScreenWidth - 110) / 2, 25, 5, mkcolor(128, 128, 255));
        draw_text(str, (kScreenWidth - 110) / 2 + 1, 26, 5, mkcolor(128, 128, 255));
      }
    }

    draw_text("START", (kScreenWidth - 50) / 2, 180, 5, mkcolor(0, 255, 0));

    if (game_pending)
    {
      draw_text("CONTINUE", (kScreenWidth - 80) / 2, 200, 5, mkcolor(0, 255, 0));
    }

    /* (Giant rock) */

    draw_segment(40 / size, 0, mkcolor(255, 255, 255), 30 / size, 30, mkcolor(255, 255, 255), x, y, angle);
    draw_segment(30 / size, 30, mkcolor(255, 255, 255), 40 / size, 55, mkcolor(255, 255, 255), x, y, angle);
    draw_segment(40 / size, 55, mkcolor(255, 255, 255), 25 / size, 90, mkcolor(255, 255, 255), x, y, angle);
    draw_segment(25 / size, 90, mkcolor(255, 255, 255), 40 / size, 120, mkcolor(255, 255, 255), x, y, angle);
    draw_segment(40 / size, 120, mkcolor(255, 255, 255), 35 / size, 130, mkcolor(255, 255, 255), x, y, angle);
    draw_segment(35 / size, 130, mkcolor(255, 255, 255), 40 / size, 160, mkcolor(255, 255, 255), x, y, angle);
    draw_segment(40 / size, 160, mkcolor(255, 255, 255), 30 / size, 200, mkcolor(255, 255, 255), x, y, angle);
    draw_segment(30 / size, 200, mkcolor(255, 255, 255), 45 / size, 220, mkcolor(255, 255, 255), x, y, angle);
    draw_segment(45 / size, 220, mkcolor(255, 255, 255), 25 / size, 265, mkcolor(255, 255, 255), x, y, angle);
    draw_segment(25 / size, 265, mkcolor(255, 255, 255), 30 / size, 300, mkcolor(255, 255, 255), x, y, angle);
    draw_segment(30 / size, 300, mkcolor(255, 255, 255), 45 / size, 335, mkcolor(255, 255, 255), x, y, angle);
    draw_segment(45 / size, 335, mkcolor(255, 255, 255), 40 / size, 0, mkcolor(255, 255, 255), x, y, angle);

    /* Flush and pause! */

    SDL_RenderPresent(g_renderer);

    Uint64 now_time = SDL_GetTicks64();

    if (now_time < last_time + (1000 / kScreenFPS))
    {
      SDL_Delay(last_time + 1000 / kScreenFPS - now_time);
    }
  }

  return quit;
}

/* --- GAME --- */

bool
game(void)
{
  bool done = false, quit = false;
  int32_t counter = 0;
  int32_t num_asteroids_alive = 0;
  SDL_Event event = {0};
  bool left_pressed = false, right_pressed = false, up_pressed = false, shift_pressed = false;
  char str[10] = {0};
  uint32_t now_time = 0, last_time = 0;

  if (!game_pending)
  {
    lives = 3;
    score = 0;

    player_alive = 1;
    player_die_timer = 0;
    player_angle = 90;
    player_x = (kScreenWidth / 2) << 4;
    player_y = (kScreenHeight / 2) << 4;
    player_xm = 0;
    player_ym = 0;

    level = 1;
    reset_level();
  }

  game_pending = true;

  /* Hide mouse cursor: */

  if (fullscreen)
  {
    SDL_ShowCursor(0);
  }

  /* Play music: */

  if (use_sound)
  {
    if (!Mix_PlayingMusic())
    {
      Mix_PlayMusic(game_music, -1);
    }
  }

  do
  {
    last_time = SDL_GetTicks();
    counter++;

    /* Handle events: */

    while (SDL_PollEvent(&event) > 0)
    {
      if (event.type == SDL_QUIT)
      {
        /* Quit! */

        done = true;
        quit = true;
      }
      else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
      {
        if (event.type == SDL_KEYDOWN)
        {
          switch (event.key.keysym.scancode)
          {
            case SDL_SCANCODE_ESCAPE:
              /* Return to menu! */
              done = true;
              break;

              /* Key press... */
            case SDL_SCANCODE_RIGHT:
              /* Rotate CW */
              left_pressed = false;
              right_pressed = true;
              break;
            case SDL_SCANCODE_LEFT:
              /* Rotate CCW */
              left_pressed = true;
              right_pressed = false;
              break;
            case SDL_SCANCODE_UP:
              /* Thrust! */
              up_pressed = true;
              break;
            case SDL_SCANCODE_SPACE:
              if (player_alive)
              {
                /* Fire a bullet! */
                add_bullet(player_x >> 4, player_y >> 4, player_angle, player_xm, player_ym);
              }
              break;
            case SDL_SCANCODE_LSHIFT:
            case SDL_SCANCODE_RSHIFT:
              /* Respawn now (if applicable) */
              shift_pressed = true;
              break;
            default:
              break;
          }
        }
        else if (event.type == SDL_KEYUP)
        {
          /* Key release... */
          switch (event.key.keysym.scancode)
          {
            case SDL_SCANCODE_RIGHT:
              right_pressed = false;
              break;
            case SDL_SCANCODE_LEFT:
              left_pressed = false;
              break;
            case SDL_SCANCODE_UP:
              up_pressed = false;
              break;
            case SDL_SCANCODE_LSHIFT:
            case SDL_SCANCODE_RSHIFT:
              /* Respawn now (if applicable) */
              shift_pressed = false;
              break;
            default:
              break;
          }
        }
      }
#ifdef JOY_YES
      else if (event.type == SDL_JOYBUTTONDOWN && player_alive)
      {
        if (event.jbutton.button == JOY_B)
        {
          /* Fire a bullet! */

          add_bullet(x >> 4, y >> 4, angle, player_xm, player_ym);
        }
        else if (event.jbutton.button == JOY_A)
        {
          /* Thrust: */

          up_pressed = true;
        }
        else
        {
          shift_pressed = true;
        }
      }
      else if (event.type == SDL_JOYBUTTONUP)
      {
        if (event.jbutton.button == JOY_A)
        {
          /* Stop thrust: */

          up_pressed = false;
        }
        else if (event.jbutton.button != JOY_B)
        {
          shift_pressed = false;
        }
      }
      else if (event.type == SDL_JOYAXISMOTION)
      {
        if (event.jaxis.axis == JOY_X)
        {
          if (event.jaxis.value < -256)
          {
            left_pressed = true;
            right_pressed = false;
          }
          else if (event.jaxis.value > 256)
          {
            left_pressed = false;
            right_pressed = true;
          }
          else
          {
            left_pressed = false;
            right_pressed = false;
          }
        }
      }
#endif
    }

    /* Rotate ship: */

    if (right_pressed)
    {
      player_angle -= 8;
      if (player_angle < 0)
      {
        player_angle += 360;
      }
    }
    else if (left_pressed)
    {
      player_angle += 8;
      if (player_angle >= 360)
      {
        player_angle -= 360;
      }
    }

    /* Thrust ship: */

    if (up_pressed && player_alive)
    {
      /* Move forward: */

      player_xm += (fast_cos(player_angle >> 3) * 3) >> 10;
      player_ym -= (fast_sin(player_angle >> 3) * 3) >> 10;

      /* Start thruster sound: */
      if (use_sound)
      {
        if (!Mix_Playing(CHAN_THRUST))
        {
          Mix_PlayChannel(CHAN_THRUST, sounds[SND_THRUST], -1);
        }
      }
    }
    else
    {
      /* Slow down (unrealistic, but.. feh!) */

      if ((counter % 20) == 0)
      {
        player_xm = (player_xm * 7) / 8;
        player_ym = (player_ym * 7) / 8;
      }

      /* Stop thruster sound: */

      if (use_sound)
      {
        if (Mix_Playing(CHAN_THRUST))
        {
          Mix_HaltChannel(CHAN_THRUST);
        }
      }
    }

    /* Handle player death: */

    if (player_alive == 0)
    {
      player_die_timer--;

      if (player_die_timer <= 0)
      {
        if (lives > 0)
        {
          /* Reset player: */

          player_die_timer = 0;
          player_angle = 90;
          player_x = (kScreenWidth / 2) << 4;
          player_y = (kScreenHeight / 2) << 4;
          player_xm = 0;
          player_ym = 0;

          /* Only bring player back when it's alright to! */

          player_alive = 1;

          if (!shift_pressed)
          {
            for (size_t i = 0; i < kNumAsteroids && player_alive; i++)
            {
              if (asteroids[i].alive)
              {
                if (asteroids[i].x >= (player_x >> 4) - (kScreenWidth / 5) && asteroids[i].x <= (player_x >> 4) + (kScreenWidth / 5) && asteroids[i].y >= (player_y >> 4) - (kScreenHeight / 5) && asteroids[i].y <= (player_y >> 4) + (kScreenHeight / 5))
                {
                  /* If any asteroid is too close for comfort,
                     don't bring ship back yet! */

                  player_alive = 0;
                }
              }
            }
          }
        }
        else
        {
          done = true;
          game_pending = false;
        }
      }
    }

    /* Erase screen: */

    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
    SDL_RenderClear(g_renderer);
    SDL_RenderCopy(g_renderer, g_texture, NULL, NULL);

    /* Move ship: */

    player_x += player_xm;
    player_y += player_ym;

    /* Wrap ship around edges of screen: */

    if (player_x >= (kScreenWidth << 4))
    {
      player_x -= (kScreenWidth << 4);
    }
    else if (player_x < 0)
    {
      player_x += (kScreenWidth << 4);
    }

    if (player_y >= (kScreenHeight << 4))
    {
      player_y -= (kScreenHeight << 4);
    }
    else if (player_y < 0)
    {
      player_y += (kScreenHeight << 4);
    }

    /* Move bullets: */

    for (size_t i = 0; i < kNumBullets; i++)
    {
      if (bullets[i].timer >= 0)
      {
        /* Bullet wears out: */

        bullets[i].timer--;

        /* Move bullet: */

        bullets[i].x = bullets[i].x + bullets[i].xm;
        bullets[i].y = bullets[i].y + bullets[i].ym;

        /* Wrap bullet around edges of screen: */

        if (bullets[i].x >= kScreenWidth)
        {
          bullets[i].x = bullets[i].x - kScreenWidth;
        }
        else if (bullets[i].x < 0)
        {
          bullets[i].x = bullets[i].x + kScreenWidth;
        }

        if (bullets[i].y >= kScreenHeight)
        {
          bullets[i].y = bullets[i].y - kScreenHeight;
        }
        else if (bullets[i].y < 0)
        {
          bullets[i].y = bullets[i].y + kScreenHeight;
        }

        /* Check for collision with any asteroids! */

        for (size_t j = 0; j < kNumAsteroids; j++)
        {
          if (bullets[i].timer > 0 && asteroids[j].alive)
          {
            if ((bullets[i].x + 5 >= asteroids[j].x - asteroids[j].size * kAsteroidsRadius) && (bullets[i].x - 5 <= asteroids[j].x + asteroids[j].size * kAsteroidsRadius) && (bullets[i].y + 5 >= asteroids[j].y - asteroids[j].size * kAsteroidsRadius) && (bullets[i].y - 5 <= asteroids[j].y + asteroids[j].size * kAsteroidsRadius))
            {
              /* Remove bullet! */

              bullets[i].timer = 0;

              hurt_asteroid(j, bullets[i].xm, bullets[i].ym, asteroids[j].size * 3);
            }
          }
        }
      }
    }

    /* Move asteroids: */

    num_asteroids_alive = 0;

    for (size_t i = 0; i < kNumAsteroids; i++)
    {
      if (asteroids[i].alive)
      {
        num_asteroids_alive++;

        /* Move asteroid: */

        if ((counter % 4) == 0)
        {
          asteroids[i].x = asteroids[i].x + asteroids[i].xm;
          asteroids[i].y = asteroids[i].y + asteroids[i].ym;
        }

        /* Wrap asteroid around edges of screen: */

        if (asteroids[i].x >= kScreenWidth)
        {
          asteroids[i].x = asteroids[i].x - kScreenWidth;
        }
        else if (asteroids[i].x < 0)
        {
          asteroids[i].x = asteroids[i].x + kScreenWidth;
        }

        if (asteroids[i].y >= kScreenHeight)
        {
          asteroids[i].y = asteroids[i].y - kScreenHeight;
        }
        else if (asteroids[i].y < 0)
        {
          asteroids[i].y = asteroids[i].y + kScreenHeight;
        }

        /* Rotate asteroid: */

        asteroids[i].angle = (asteroids[i].angle + asteroids[i].angle_m);

        /* Wrap rotation angle... */

        if (asteroids[i].angle < 0)
        {
          asteroids[i].angle = asteroids[i].angle + 360;
        }
        else if (asteroids[i].angle >= 360)
        {
          asteroids[i].angle = asteroids[i].angle - 360;
        }

        /* See if we collided with the player: */

        if (asteroids[i].x >= (player_x >> 4) - kShipRadius && asteroids[i].x <= (player_x >> 4) + kShipRadius && asteroids[i].y >= (player_y >> 4) - kShipRadius && asteroids[i].y <= (player_y >> 4) + kShipRadius && player_alive)
        {
          hurt_asteroid(i, player_xm >> 4, player_ym >> 4, kNumBits);

          player_alive = 0;
          player_die_timer = 30;

          playsound(SND_EXPLODE);

          /* Stop thruster sound: */

          if (use_sound)
          {
            if (Mix_Playing(CHAN_THRUST))
            {
              Mix_HaltChannel(CHAN_THRUST);
            }
          }

          lives--;

          if (lives == 0)
          {
            if (use_sound)
            {
              playsound(SND_GAMEOVER);
              playsound(SND_GAMEOVER);
              playsound(SND_GAMEOVER);
              /* Mix_PlayChannel(CHAN_THRUST,
                 sounds[SND_GAMEOVER], 0); */
            }
            player_die_timer = 100;
          }
        }
      }
    }

    /* Move bits: */

    for (size_t i = 0; i < kNumBits; i++)
    {
      if (bits[i].timer > 0)
      {
        /* Countdown bit's lifespan: */

        bits[i].timer--;

        /* Move the bit: */

        bits[i].x = bits[i].x + bits[i].xm;
        bits[i].y = bits[i].y + bits[i].ym;

        /* Wrap bit around edges of screen: */

        if (bits[i].x >= kScreenWidth)
        {
          bits[i].x = bits[i].x - kScreenWidth;
        }
        else if (bits[i].x < 0)
        {
          bits[i].x = bits[i].x + kScreenWidth;
        }

        if (bits[i].y >= kScreenHeight)
        {
          bits[i].y = bits[i].y - kScreenHeight;
        }
        else if (bits[i].y < 0)
        {
          bits[i].y = bits[i].y + kScreenHeight;
        }
      }
    }

    /* Draw ship: */

    if (player_alive)
    {
      draw_segment(kShipRadius, 0, mkcolor(128, 128, 255), kShipRadius / 2, 135, mkcolor(0, 0, 192), player_x >> 4, player_y >> 4, player_angle);

      draw_segment(kShipRadius / 2, 135, mkcolor(0, 0, 192), 0, 0, mkcolor(64, 64, 230), player_x >> 4, player_y >> 4, player_angle);

      draw_segment(0, 0, mkcolor(64, 64, 230), kShipRadius / 2, 225, mkcolor(0, 0, 192), player_x >> 4, player_y >> 4, player_angle);

      draw_segment(kShipRadius / 2, 225, mkcolor(0, 0, 192), kShipRadius, 0, mkcolor(128, 128, 255), player_x >> 4, player_y >> 4, player_angle);

      /* Draw flame: */

      if (up_pressed)
      {
        draw_segment(0, 0, mkcolor(255, 255, 255), (rand() % 20), 180, mkcolor(255, 0, 0), player_x >> 4, player_y >> 4, player_angle);
      }
    }

    /* Draw bullets: */

    for (size_t i = 0; i < kNumBullets; i++)
    {
      if (bullets[i].timer >= 0)
      {
        draw_line(bullets[i].x - (rand() % 3) - bullets[i].xm * 2,
                  bullets[i].y - (rand() % 3) - bullets[i].ym * 2,
                  mkcolor((rand() % 3) * 128,
                          (rand() % 3) * 128,
                          (rand() % 3) * 128),
                  bullets[i].x + (rand() % 3) - bullets[i].xm * 2,
                  bullets[i].y + (rand() % 3) - bullets[i].ym * 2,
                  mkcolor((rand() % 3) * 128,
                          (rand() % 3) * 128,
                          (rand() % 3) * 128));

        draw_line(bullets[i].x + (rand() % 3) - bullets[i].xm * 2,
                  bullets[i].y - (rand() % 3) - bullets[i].ym * 2,
                  mkcolor((rand() % 3) * 128,
                          (rand() % 3) * 128,
                          (rand() % 3) * 128),
                  bullets[i].x - (rand() % 3) - bullets[i].xm * 2,
                  bullets[i].y + (rand() % 3) - bullets[i].ym * 2,
                  mkcolor((rand() % 3) * 128,
                          (rand() % 3) * 128,
                          (rand() % 3) * 128));

        draw_thick_line(bullets[i].x - (rand() % 5),
                        bullets[i].y - (rand() % 5),
                        mkcolor((rand() % 3) * 128 + 64,
                                (rand() % 3) * 128 + 64,
                                (rand() % 3) * 128 + 64),
                        bullets[i].x + (rand() % 5),
                        bullets[i].y + (rand() % 5),
                        mkcolor((rand() % 3) * 128 + 64,
                                (rand() % 3) * 128 + 64,
                                (rand() % 3) * 128 + 64));

        draw_thick_line(bullets[i].x + (rand() % 5),
                        bullets[i].y - (rand() % 5),
                        mkcolor((rand() % 3) * 128 + 64,
                                (rand() % 3) * 128 + 64,
                                (rand() % 3) * 128 + 64),
                        bullets[i].x - (rand() % 5),
                        bullets[i].y + (rand() % 5),
                        mkcolor((rand() % 3) * 128 + 64,
                                (rand() % 3) * 128 + 64,
                                (rand() % 3) * 128 + 64));
      }
    }

    /* Draw asteroids: */

    for (size_t i = 0; i < kNumAsteroids; i++)
    {
      if (asteroids[i].alive)
      {
        draw_asteroid(asteroids[i].size,
                      asteroids[i].x,
                      asteroids[i].y,
                      asteroids[i].angle,
                      asteroids[i].shape);
      }
    }

    /* Draw bits: */

    for (size_t i = 0; i < kNumBits; i++)
    {
      if (bits[i].timer > 0)
      {
        draw_line(bits[i].x, bits[i].y, mkcolor(255, 255, 255), bits[i].x + bits[i].xm, bits[i].y + bits[i].ym, mkcolor(255, 255, 255));
      }
    }

    /* Draw score: */

    sprintf(str, "%.6ld", score);
    draw_text(str, 3, 3, 14, mkcolor(255, 255, 255));
    draw_text(str, 4, 4, 14, mkcolor(255, 255, 255));

    /* Level: */

    sprintf(str, "%ld", level);
    draw_text(str, (kScreenWidth - 14) / 2, 3, 14, mkcolor(255, 255, 255));
    draw_text(str, (kScreenWidth - 14) / 2 + 1, 4, 14, mkcolor(255, 255, 255));

    /* Draw lives: */
    size_t k = 0;
    for (size_t i = 0; i < lives; i++, k++)
    {
      draw_segment(16, 0, mkcolor(255, 255, 255), 4, 135, mkcolor(255, 255, 255), kScreenWidth - 10 - i * 10, 20, 90);

      draw_segment(8, 135, mkcolor(255, 255, 255), 0, 0, mkcolor(255, 255, 255), kScreenWidth - 10 - i * 10, 20, 90);

      draw_segment(0, 0, mkcolor(255, 255, 255), 8, 225, mkcolor(255, 255, 255), kScreenWidth - 10 - i * 10, 20, 90);

      draw_segment(8, 225, mkcolor(255, 255, 255), 16, 0, mkcolor(255, 255, 255), kScreenWidth - 10 - i * 10, 20, 90);
    }

    if (player_die_timer > 0)
    {
      size_t j = 0;

      if (player_die_timer > 30)
      {
        j = 30;
      }
      else
      {
        j = player_die_timer;
      }

      draw_segment((16 * j) / 30, 0, mkcolor(255, 255, 255), (4 * j) / 30, 135, mkcolor(255, 255, 255), kScreenWidth - 10 - k * 10, 20, 90);

      draw_segment((8 * j) / 30, 135, mkcolor(255, 255, 255), 0, 0, mkcolor(255, 255, 255), kScreenWidth - 10 - k * 10, 20, 90);

      draw_segment(0, 0, mkcolor(255, 255, 255), (8 * j) / 30, 225, mkcolor(255, 255, 255), kScreenWidth - 10 - k * 10, 20, 90);

      draw_segment((8 * j) / 30, 225, mkcolor(255, 255, 255), (16 * j) / 30, 0, mkcolor(255, 255, 255), kScreenWidth - 10 - k * 10, 20, 90);
    }

    /* Zooming level effect: */

    if (text_zoom > 0)
    {
      if ((counter % 2) == 0)
      {
        text_zoom--;
      }

      draw_text(zoom_str, (kScreenWidth - (strlen(zoom_str) * text_zoom)) / 2, (kScreenHeight - text_zoom) / 2, text_zoom, mkcolor(text_zoom * (256 / kZoomStart), 0, 0));
    }

    /* Game over? */

    if (player_alive == 0 && lives == 0)
    {
      if (player_die_timer > 14)
      {
        draw_text("GAME OVER",
                  (kScreenWidth - 9 * player_die_timer) / 2,
                  (kScreenHeight - player_die_timer) / 2,
                  player_die_timer,
                  mkcolor(rand() % 255,
                          rand() % 255,
                          rand() % 255));
      }
      else
      {
        draw_text("GAME OVER",
                  (kScreenWidth - 9 * 14) / 2,
                  (kScreenHeight - 14) / 2,
                  14,
                  mkcolor(255, 255, 255));
      }
    }

    /* Go to next level? */

    if (num_asteroids_alive == 0)
    {
      level++;

      reset_level();
    }

    /* Flush and pause! */

    SDL_RenderPresent(g_renderer);

    now_time = SDL_GetTicks();

    if (now_time < last_time + (1000 / kScreenFPS))
    {
      SDL_Delay(last_time + 1000 / kScreenFPS - now_time);
    }
  } while (!done);

  /* Record, if a high score: */

  if (score >= high)
  {
    high = score;
  }

  /* Display mouse cursor: */

  if (fullscreen)
  {
    SDL_ShowCursor(1);
  }

  return (quit);
}

void
finish(void)
{
  SDL_Quit();
}

void
setup(const int argc, const char* argv[])
{
  /* Check command-line options: */

  for (size_t i = 1; i < (size_t)argc; i++)
  {
    if (strcmp(argv[i], "--fullscreen") == 0 || strcmp(argv[i], "-f") == 0)
    {
      fullscreen = true;
    }
    else if (strcmp(argv[i], "--nosound") == 0 || strcmp(argv[i], "-q") == 0)
    {
      use_sound = false;
    }
    else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
    {
      show_version();

      printf("\n"
             "Programming: Bill Kendrick, New Breed Software - bill@newbreedsoftware.com\n"
             "Music:       Mike Faltiss (Hadji/Digital Music Kings) - deadchannel@hotmail.com\n"
             "\n"
             "Keyboard controls:\n"
             "  Left/Right - Rotate ship\n"
             "  Up         - Thrust engines\n"
             "  Space      - Fire weapons\n"
             "  Shift      - Respawn after death (or wait)\n"
             "  Escape     - Return to title screen\n"
             "\n"
             "Joystick controls:\n"
             "  Left/Right - Rotate ship\n"
             "  Fire-A     - Thrust engines\n"
             "  Fire-B     - Fire weapons\n"
             "\n"
             "Run with \"--usage\" for command-line options...\n"
             "Run with \"--copying\" for copying information...\n"
             "\n");

      exit(0);
    }
    else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0)
    {
      show_version();
      printf("State format file version " kGameDate "\n");
      exit(0);
    }
    else if (strcmp(argv[i], "--copying") == 0 || strcmp(argv[i], "-c") == 0)
    {
      show_version();
      printf("\n"
             "This program is free software; you can redistribute it\n"
             "and/or modify it under the terms of the GNU General Public\n"
             "License as published by the Free Software Foundation;\n"
             "either version 2 of the License, or (at your option) any\n"
             "later version.\n"
             "\n"
             "This program is distributed in the hope that it will be\n"
             "useful and entertaining, but WITHOUT ANY WARRANTY; without\n"
             "even the implied warranty of MERCHANTABILITY or FITNESS\n"
             "FOR A PARTICULAR PURPOSE.  See the GNU General Public\n"
             "License for more details.\n"
             "\n");
      printf("You should have received a copy of the GNU General Public\n"
             "License along with this program; if not, write to the Free\n"
             "Software Foundation, Inc., 59 Temple Place, Suite 330,\n"
             "Boston, MA  02111-1307  USA\n"
             "\n");
      exit(0);
    }
    else if (strcmp(argv[i], "--usage") == 0 || strcmp(argv[i], "-u") == 0)
    {
      show_usage(stdout, argv[0]);
      exit(0);
    }
    else
    {
      show_usage(stderr, argv[0]);
      exit(1);
    }
  }

  /* Seed random number generator: */

  srand(SDL_GetTicks());

  /* Init SDL video: */

  if (SDL_Init(SDL_INIT_VIDEO) < 0)
  {
    fprintf(stderr,
            "\nError: I could not initialize video!\n"
            "The Simple DirectMedia error that occured was:\n"
            "%s\n\n",
            SDL_GetError());
    exit(1);
  }

  /* Init joysticks: */

#ifdef JOY_YES
  use_joystick = true;

  if (SDL_Init(SDL_INIT_JOYSTICK) < 0)
  {
    fprintf(stderr,
            "\nWarning: I could not initialize joystick.\n"
            "The Simple DirectMedia error that occured was:\n"
            "%s\n\n",
            SDL_GetError());

    use_joystick = false;
  }
  else
  {
    /* Look for joysticks: */

    if (SDL_NumJoysticks() <= 0)
    {
      fprintf(stderr,
              "\nWarning: No joysticks available.\n");

      use_joystick = false;
    }
    else
    {
      /* Open joystick: */

      js = SDL_JoystickOpen(0);

      if (!js)
      {
        fprintf(stderr,
                "\nWarning: Could not open joystick 1.\n"
                "The Simple DirectMedia error that occured was:\n"
                "%s\n\n",
                SDL_GetError());

        use_joystick = false;
      }
      else
      {
        /* Check for proper stick configuration: */

        if (SDL_JoystickNumAxes(js) < 2)
        {
          fprintf(stderr,
                  "\nWarning: Joystick doesn't have enough axes!\n");

          use_joystick = false;
        }
        else
        {
          if (SDL_JoystickNumButtons(js) < 2)
          {
            fprintf(stderr,
                    "\nWarning: Joystick doesn't have enough "
                    "buttons!\n");

            use_joystick = false;
          }
        }
      }
    }
  }
#else
  use_joystick = false;
#endif

  /* Open window: */

  g_window = SDL_CreateWindow(kGameName " v" kGameVersion, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, kScreenWidth, kScreenHeight, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);

  if (!g_window)
  {
    fprintf(stderr, "Window creation error: %s\n", SDL_GetError());
    SDL_Quit();
    exit(EXIT_FAILURE);
  }

  SDL_SetWindowFullscreen(g_window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);

  g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!g_renderer)
  {
    fprintf(stderr, "Renderer creation error; %s\n", SDL_GetError());
    SDL_DestroyWindow(g_window);
    SDL_Quit();
    exit(EXIT_FAILURE);
  }

  /* Load background image: */

  g_texture = IMG_LoadTexture(g_renderer, DATA_PREFIX "images/redspot.jpg");

  if (!g_texture)
  {
    fprintf(stderr,
            "\nError: I could not open the background image:\n" DATA_PREFIX "images/redspot.jpg\n"
            "The Simple DirectMedia error that occured was:\n"
            "%s\n\n",
            SDL_GetError());
    exit(1);
  }

  SDL_RenderSetLogicalSize(g_renderer, kScreenWidth, kScreenHeight);

  /* Init sound: */

  if (use_sound)
  {
    if (Mix_OpenAudio(22050, AUDIO_S16, 2, 512) < 0)
    {
      fprintf(stderr,
              "\nWarning: I could not set up audio for 22050 Hz "
              "16-bit stereo.\n"
              "The Simple DirectMedia error that occured was:\n"
              "%s\n\n",
              SDL_GetError());
      use_sound = false;
    }
  }

  /* Load sound files: */

  if (use_sound)
  {
    for (size_t i = 0; i < NUM_SOUNDS; i++)
    {
      sounds[i] = Mix_LoadWAV(sound_names[i]);
      if (!sounds[i])
      {
        fprintf(stderr,
                "\nError: I could not load the sound file:\n"
                "%s\n"
                "The Simple DirectMedia error that occured was:\n"
                "%s\n\n",
                sound_names[i],
                SDL_GetError());
        exit(1);
      }
    }

    game_music = Mix_LoadMUS(mus_game_name);
    if (!game_music)
    {
      fprintf(stderr,
              "\nError: I could not load the music file:\n"
              "%s\n"
              "The Simple DirectMedia error that occured was:\n"
              "%s\n\n",
              mus_game_name,
              SDL_GetError());
      exit(1);
    }
  }
}

/* Fast approximate-integer, table-based cosine! Whee! */

int32_t
fast_cos(int32_t angle)
{
  angle = (angle % 45);

  if (angle < 12)
  {
    return (trig[angle]);
  }
  else if (angle < 23)
  {
    return (-trig[10 - (angle - 12)]);
  }
  else if (angle < 34)
  {
    return (-trig[angle - 22]);
  }
  else
  {
    return (trig[45 - angle]);
  }
}

/* Sine based on fast cosine... */

int32_t
fast_sin(int32_t angle)
{
  return (-fast_cos((angle + 11) % 45));
}

/* Draw a line: */

void
draw_line(int32_t x1, int32_t y1, color_type c1, int32_t x2, int32_t y2, color_type c2)
{
  sdl_drawline(x1, y1, c1, x2, y2, c2);

  if (x1 < 0 || x2 < 0)
  {
    sdl_drawline(x1 + kScreenWidth, y1, c1, x2 + kScreenWidth, y2, c2);
  }
  else if (x1 >= kScreenWidth || x2 >= kScreenWidth)
  {
    sdl_drawline(x1 - kScreenWidth, y1, c1, x2 - kScreenWidth, y2, c2);
  }

  if (y1 < 0 || y2 < 0)
  {
    sdl_drawline(x1, y1 + kScreenHeight, c1, x2, y2 + kScreenHeight, c2);
  }
  else if (y1 >= kScreenHeight || y2 >= kScreenHeight)
  {
    sdl_drawline(x1, y1 - kScreenHeight, c1, x2, y2 - kScreenHeight, c2);
  }
}

/* Create a color_type struct out of RGB values: */

color_type
mkcolor(int32_t r, int32_t g, int32_t b)
{
  color_type c;

  if (r > 255)
  {
    r = 255;
  }
  if (g > 255)
  {
    g = 255;
  }
  if (b > 255)
  {
    b = 255;
  }

  c.r = (uint8_t)r;
  c.g = (uint8_t)g;
  c.b = (uint8_t)b;

  return c;
}

/* Draw a line on an SDL surface: */

void
sdl_drawline(int32_t x1, int32_t y1, color_type c1, int32_t x2, int32_t y2, color_type c2)
{
  int32_t dx = 0, dy = 0;
  double cr = NAN, cg = NAN, cb = NAN, rd = NAN, gd = NAN, bd = NAN;
  double m = NAN, b = NAN;

  if (clip(&x1, &y1, &x2, &y2))
  {
    dx = x2 - x1;
    dy = y2 - y1;

    if (dx != 0)
    {
      m = ((double)dy) / ((double)dx);
      b = y1 - m * x1;

      if (x2 >= x1)
      {
        dx = 1;
      }
      else
      {
        dx = -1;
      }

      cr = c1.r;
      cg = c1.g;
      cb = c1.b;

      rd = (double)(c2.r - c1.r) / (double)(x2 - x1) * dx;
      gd = (double)(c2.g - c1.g) / (double)(x2 - x1) * dx;
      bd = (double)(c2.b - c1.b) / (double)(x2 - x1) * dx;

      while (x1 != x2)
      {
        y1 = m * x1 + b;
        y2 = m * (x1 + dx) + b;

        drawvertline(x1, y1, mkcolor(cr, cg, cb), y2, mkcolor(cr + rd, cg + gd, cb + bd));

        x1 = x1 + dx;

        cr = cr + rd;
        cg = cg + gd;
        cb = cb + bd;
      }
    }
    else
    {
      drawvertline(x1, y1, c1, y2, c2);
    }
  }
}

/* Clip lines to window: */

int32_t
clip(int32_t* x1, int32_t* y1, int32_t* x2, int32_t* y2)
{
  double fx1 = NAN, fx2 = NAN, fy1 = NAN, fy2 = NAN, tmp = NAN;
  double m = NAN;
  uint8_t code1 = 0, code2 = 0;
  int32_t done = 0, draw = 0, swapped = 0;
  uint8_t ctmp = 0;
  fx1 = (double)*x1;
  fy1 = (double)*y1;
  fx2 = (double)*x2;
  fy2 = (double)*y2;

  done = false;
  draw = false;
  m = 0;
  swapped = false;

  while (!done)
  {
    code1 = encode(fx1, fy1);
    code2 = encode(fx2, fy2);

    if (!(code1 | code2))
    {
      done = true;
      draw = true;
    }
    else if (code1 & code2)
    {
      done = true;
    }
    else
    {
      if (!code1)
      {
        swapped = true;
        tmp = fx1;
        fx1 = fx2;
        fx2 = tmp;

        tmp = fy1;
        fy1 = fy2;
        fy2 = tmp;

        ctmp = code1;
        code1 = code2;
        code2 = ctmp;
      }

      if (fx2 != fx1)
      {
        m = (fy2 - fy1) / (fx2 - fx1);
      }
      else
      {
        m = 1;
      }

      if (code1 & LEFT_EDGE)
      {
        fy1 += ((0 - (fx1)) * m);
        fx1 = 0;
      }
      else if (code1 & RIGHT_EDGE)
      {
        fy1 += (((kScreenWidth - 1) - (fx1)) * m);
        fx1 = (kScreenWidth - 1);
      }
      else if (code1 & TOP_EDGE)
      {
        if (fx2 != fx1)
        {
          fx1 += ((0 - (fy1)) / m);
        }
        fy1 = 0;
      }
      else if (code1 & BOTTOM_EDGE)
      {
        if (fx2 != fx1)
        {
          fx1 += (((kScreenHeight - 1) - (fy1)) / m);
        }
        fy1 = (kScreenHeight - 1);
      }
    }
  }

  if (swapped)
  {
    tmp = fx1;
    fx1 = fx2;
    fx2 = tmp;

    tmp = fy1;
    fy1 = fy2;
    fy2 = tmp;
  }

  *x1 = (int32_t)fx1;
  *y1 = (int32_t)fy1;
  *x2 = (int32_t)fx2;
  *y2 = (int32_t)fy2;

  return (draw);
}

/* Where does this line clip? */

uint8_t
encode(double x, double y)
{
  uint8_t code = 0;

  code = 0x00;

  if (x < 0.0)
  {
    code = code | LEFT_EDGE;
  }
  else if (x >= (double)kScreenWidth)
  {
    code = code | RIGHT_EDGE;
  }

  if (y < 0.0)
  {
    code = code | TOP_EDGE;
  }
  else if (y >= (double)kScreenHeight)
  {
    code = code | BOTTOM_EDGE;
  }

  return code;
}

/* Draw a verticle line: */

void
drawvertline(int32_t x, int32_t y1, color_type c1, int32_t y2, color_type c2)
{
  int32_t tmp = 0, dy = 0;
  double cr = NAN, cg = NAN, cb = NAN, rd = NAN, gd = NAN, bd = NAN;

  if (y1 > y2)
  {
    tmp = y1;
    y1 = y2;
    y2 = tmp;

    tmp = c1.r;
    c1.r = c2.r;
    c2.r = tmp;

    tmp = c1.g;
    c1.g = c2.g;
    c2.g = tmp;

    tmp = c1.b;
    c1.b = c2.b;
    c2.b = tmp;
  }

  cr = c1.r;
  cg = c1.g;
  cb = c1.b;

  if (y1 != y2)
  {
    rd = (double)(c2.r - c1.r) / (double)(y2 - y1);
    gd = (double)(c2.g - c1.g) / (double)(y2 - y1);
    bd = (double)(c2.b - c1.b) / (double)(y2 - y1);
  }
  else
  {
    rd = 0;
    gd = 0;
    bd = 0;
  }

  for (dy = y1; dy <= y2; dy++)
  {
    putpixel(x + 1, dy + 1, (color_type){.r = 0, .g = 0, .b = 0});

    putpixel(x, dy, (color_type){.r = (uint8_t)cr, .g = (uint8_t)cg, .b = (uint8_t)cb});

    cr = cr + rd;
    cg = cg + gd;
    cb = cb + bd;
  }
}

/* Draw a single pixel into the surface: */

void
putpixel(int32_t x, int32_t y, color_type color)
{
#if 0
  int32_t bpp = 0;
  uint8_t* p = 0;
#endif

  /* Assuming the X/Y values are within the bounds of this surface... */

  if (x >= 0 && y >= 0 && x < kScreenWidth && y < kScreenHeight)
  {
    SDL_SetRenderDrawColor(g_renderer, color.r, color.g, color.b, 255);
    SDL_RenderDrawPoint(g_renderer, x, y);
#if 0
      /* Determine bytes-per-pixel for the surface in question: */

      bpp = surface->format->BytesPerPixel;

      /* Set a pointer to the exact location in memory of the pixel
         in question: */

      p = (((uint8_t*)surface->pixels) + /* Start at beginning of RAM */
           (y * surface->pitch) +        /* Go down Y lines */
           (x * bpp));                   /* Go in X pixels */

      /* Set the (correctly-sized) piece of data in the surface's RAM
         to the pixel value sent in: */

      if (bpp == 1)
        {
          *p = pixel;
        }
      else if (bpp == 2)
        {
          *(uint16_t*)p = pixel;
        }
      else if (bpp == 3)
        {
          if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            {
              p[0] = (pixel >> 16) & 0xff;
              p[1] = (pixel >> 8) & 0xff;
              p[2] = pixel & 0xff;
            }
          else
            {
              p[0] = pixel & 0xff;
              p[1] = (pixel >> 8) & 0xff;
              p[2] = (pixel >> 16) & 0xff;
            }
        }
      else if (bpp == 4)
        {
          *(uint32_t*)p = pixel;
        }
#endif
  }
}

/* Draw a line segment, rotated around a center point: */

void
draw_segment(int32_t r1, int32_t a1, color_type c1, int32_t r2, int32_t a2, color_type c2, int32_t cx, int32_t cy, int32_t a)
{
  draw_line(((fast_cos((a1 + a) >> 3) * r1) >> 10) + cx,
            cy - ((fast_sin((a1 + a) >> 3) * r1) >> 10),
            c1,
            ((fast_cos((a2 + a) >> 3) * r2) >> 10) + cx,
            cy - ((fast_sin((a2 + a) >> 3) * r2) >> 10),
            c2);
}

/* Add a bullet: */

void
add_bullet(int32_t x, int32_t y, int32_t a, int32_t xm, int32_t ym)
{
  int32_t found = 0;

  found = -1;

  for (size_t i = 0; i < kNumBullets && found == -1; i++)
  {
    if (bullets[i].timer <= 0)
    {
      found = i;
    }
  }

  if (found != -1)
  {
    bullets[found].timer = 50;

    bullets[found].x = x;
    bullets[found].y = y;

    bullets[found].xm = ((fast_cos(a >> 3) * 5) >> 10) + (xm >> 4);
    bullets[found].ym = -((fast_sin(a >> 3) * 5) >> 10) + (ym >> 4);

    playsound(SND_BULLET);
  }
}

/* Add an asteroid: */

void
add_asteroid(int32_t x, int32_t y, int32_t xm, int32_t ym, int32_t size)
{
  int32_t found = 0;

  /* Find a slot: */

  found = -1;

  for (size_t i = 0; i < kNumAsteroids && found == -1; i++)
  {
    if (asteroids[i].alive == 0)
    {
      found = i;
    }
  }

  /* Hack: No asteroids should be stationary! */

  while (xm == 0)
  {
    xm = (rand() % 3) - 1;
  }

  if (found != -1)
  {
    asteroids[found].alive = 1;

    asteroids[found].x = x;
    asteroids[found].y = y;
    asteroids[found].xm = xm;
    asteroids[found].ym = ym;

    asteroids[found].angle = (rand() % 360);
    asteroids[found].angle_m = (rand() % 6) - 3;

    asteroids[found].size = size;

    for (size_t i = 0; i < kAsteroidsSides; i++)
    {
      asteroids[found].shape[i].radius = (rand() % 3);
      asteroids[found].shape[i].angle = i * 60 + (rand() % 40);
    }
  }
}

/* Add a bit: */

void
add_bit(int32_t x, int32_t y, int32_t xm, int32_t ym)
{
  int32_t found = 0;

  found = -1;

  for (size_t i = 0; i < kNumBits && found == -1; i++)
  {
    if (bits[i].timer <= 0)
    {
      found = i;
    }
  }

  if (found != -1)
  {
    bits[found].timer = 16;

    bits[found].x = x;
    bits[found].y = y;
    bits[found].xm = xm;
    bits[found].ym = ym;
  }
}

/* Draw an asteroid: */

void
draw_asteroid(int32_t size, int32_t x, int32_t y, int32_t angle, shape_type* shape)
{
  int32_t b1 = 0, b2 = 0;
  int32_t div = 0;

  div = 240;

  for (size_t i = 0; i < kAsteroidsSides - 1; i++)
  {
    b1 = (((shape[i].angle + angle) % 180) * 255) / div;
    b2 = (((shape[i + 1].angle + angle) % 180) * 255) / div;

    draw_segment((size * (kAsteroidsRadius - shape[i].radius)),
                 shape[i].angle,
                 mkcolor(b1, b1, b1),
                 (size * (kAsteroidsRadius - shape[i + 1].radius)),
                 shape[i + 1].angle,
                 mkcolor(b2, b2, b2),
                 x,
                 y,
                 angle);
  }

  b1 = (((shape[kAsteroidsSides - 1].angle + angle) % 180) * 255) / div;
  b2 = (((shape[0].angle + angle) % 180) * 255) / div;

  draw_segment((size * (kAsteroidsRadius - shape[kAsteroidsSides - 1].radius)),
               shape[kAsteroidsSides - 1].angle,
               mkcolor(b1, b1, b1),
               (size * (kAsteroidsRadius - shape[0].radius)),
               shape[0].angle,
               mkcolor(b2, b2, b2),
               x,
               y,
               angle);
}

/* Queue a sound! */

void
playsound(int32_t snd)
{
  int32_t which = 0;

  if (use_sound)
  {
    which = (rand() % 3) + CHAN_THRUST;
    for (size_t i = CHAN_THRUST; i < 4; i++)
    {
      if (!Mix_Playing(i))
      {
        which = i;
      }
    }

    Mix_PlayChannel(which, sounds[snd], 0);
  }
}

/* Break an asteroid and add an explosion: */

void
hurt_asteroid(int32_t j, int32_t xm, int32_t ym, size_t exp_size)
{
  add_score(100 / (asteroids[j].size + 1));

  if (asteroids[j].size > 1)
  {
    /* Break the rock into two smaller ones! */

    add_asteroid(asteroids[j].x,
                 asteroids[j].y,
                 ((asteroids[j].xm + xm) / 2),
                 (asteroids[j].ym + ym),
                 asteroids[j].size - 1);

    add_asteroid(asteroids[j].x,
                 asteroids[j].y,
                 (asteroids[j].xm + xm),
                 ((asteroids[j].ym + ym) / 2),
                 asteroids[j].size - 1);
  }

  /* Make the original go away: */

  asteroids[j].alive = 0;

  /* Add explosion: */

  playsound(SND_AST1 + (asteroids[j].size) - 1);

  for (size_t k = 0; k < exp_size; k++)
  {
    add_bit((asteroids[j].x - (asteroids[j].size * kAsteroidsRadius) + (rand() % (kAsteroidsRadius * 2))),
            (asteroids[j].y - (asteroids[j].size * kAsteroidsRadius) + (rand() % (kAsteroidsRadius * 2))),
            ((rand() % (asteroids[j].size * 3)) - (asteroids[j].size) + ((xm + asteroids[j].xm) / 3)),
            ((rand() % (asteroids[j].size * 3)) - (asteroids[j].size) + ((ym + asteroids[j].ym) / 3)));
  }
}

/* Increment score: */

void
add_score(int32_t amount)
{
  /* See if they deserve a new life: */

  if (score / kOneUpScore < (score + amount) / kOneUpScore)
  {
    lives++;
    strcpy(zoom_str, "EXTRA LIFE");
    text_zoom = kZoomStart;
    playsound(SND_EXTRALIFE);
  }

  /* Add to score: */

  score = score + amount;
}

/* Draw a character: */

void
draw_char(char c, int32_t x, int32_t y, int32_t r, color_type cl)
{
  int32_t v = 0;

  /* Which vector is this character? */

  v = -1;
  if (c >= '0' && c <= '9')
  {
    v = (c - '0');
  }
  else if (c >= 'A' && c <= 'Z')
  {
    v = (c - 'A') + 10;
  }

  if (v != -1)
  {
    for (size_t i = 0; i < 5; i++)
    {
      if (char_vectors[v][i][0] != -1)
      {
        draw_line(x + (char_vectors[v][i][0] * r),
                  y + (char_vectors[v][i][1] * r),
                  cl,
                  x + (char_vectors[v][i][2] * r),
                  y + (char_vectors[v][i][3] * r),
                  cl);
      }
    }
  }
}

void
draw_text(char* str, int32_t x, int32_t y, int32_t s, color_type c)
{
  for (size_t i = 0; i < strlen(str); i++)
  {
    draw_char(str[i], i * (s + 3) + x, y, s, c);
  }
}

void
draw_thick_line(int32_t x1, int32_t y1, color_type c1, int32_t x2, int32_t y2, color_type c2)
{
  draw_line(x1, y1, c1, x2, y2, c2);
  draw_line(x1 + 1, y1 + 1, c1, x2 + 1, y2 + 1, c2);
}

void
reset_level(void)
{
  for (size_t i = 0; i < kNumBullets; i++)
  {
    bullets[i].timer = 0;
  }

  for (size_t i = 0; i < kNumAsteroids; i++)
  {
    asteroids[i].alive = 0;
  }

  for (size_t i = 0; i < kNumBits; i++)
  {
    bits[i].timer = 0;
  }

  for (size_t i = 0; i < (level + 1) && i < 10; i++)
  {
    add_asteroid(/* x */ (rand() % 40) + ((kScreenWidth - 40) * (rand() % 2)),
                 /* y */ (rand() % kScreenHeight),
                 /* xm */ (rand() % 9) - 4,
                 /* ym */ ((rand() % 9) - 4) * 4,
                 /* size */ (rand() % 3) + 2);
  }

  sprintf(zoom_str, "LEVEL %ld", level);

  text_zoom = kZoomStart;
}

/* Show program version: */

void
show_version(void)
{
  printf("%s - v%s (%s)\n", kGameName, kGameVersion, kGameDate);
}

/* Show usage display: */

void
show_usage(FILE* f, const char* prg)
{
  fprintf(f, "Usage: %s {--help | --usage | --version | --copying }\n"
             "       %s [--fullscreen] [--nosound]\n\n",
          prg,
          prg);
}

/* Draw text, centered horizontally: */

void
draw_centered_text(char* str, int32_t y, int32_t s, color_type c)
{
  draw_text(str, (kScreenWidth - strlen(str) * (s + 3)) / 2, y, s, c);
}
