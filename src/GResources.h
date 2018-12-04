#ifndef GRESOURCES_H
#define GRESOURCES_H

#include <BTypes.h>
#include "Resources.h"

// RESOURVE MANAGER

// BBitmap Slots
enum {
  BKG_SLOT,                     // 0
  BKG2_SLOT,                    // 1
  BKG3_SLOT,                    // 2
  BKG4_SLOT,                    // 3
  BKG5_SLOT,                    // 4
  BKG6_SLOT,                    // 5

  PLAYER_SLOT,                  // 6
  COMMON_SLOT,                  // 7
// Font Slots
  FONT_8x8_SLOT,                // 8
  FONT_16x16_SLOT,              // 9

  GAME_OVER_SLOT,               // 10
  CREDITS_TEAM_SLOT,            // 11
  CREDITS_MODUS_LABS_SLOT,      // 12
};

// BRaw slots
enum {
  SONG_SLOT,
  SFX1_SLOT,
  SFX2_SLOT,
  SFX3_SLOT,
  SFX4_SLOT,
  SFX5_SLOT,
  SFX6_SLOT,
  SFX7_SLOT,
};

// image numbers on sprite sheets
// SPRITES.BMP
static const TUint16 IMG_TILE1 = 0;   // pink
static const TUint16 IMG_TILE2 = 16;   // green

static const TUint16 IMG_BGTILE1 = 6; // this is the grid background tile
static const TUint16 IMG_FRAMEL  = 14;
static const TUint16 IMG_FRAMER  = 15;

// COMMON.BMP
static const TUint16 IMG_POWERUP_MODUS_BOMB = 16;  // m bomb
static const TUint16 IMG_POWERUP_COLORSWAP  = 24;
static const TUint16 IMG_LASSO_UL           = 8;
static const TUint16 IMG_LASSO_UR           = 9;
static const TUint16 IMG_LASSO_LL           = 10;
static const TUint16 IMG_LASSO_LR           = 11;

// SPLASH_SPRITES.BMP
static const TUint16 IMG_DROP1 = 0;
static const TUint16 IMG_DROP2 = 4;
static const TUint16 IMG_DROP3 = 8;
static const TUint16 IMG_DROP4 = 16;
static const TUint16 IMG_DROP5 = 24;
static const TUint16 IMG_DROP6 = 32;

// GAME_OVER_SPRITES1.BMP
static const TUint16 IMG_GAME_OVER1 = 0;

// HIGH_SCORES_ANIMATION1.BMP
static const TUint16 IMG_HIGH_SCORES1 = 0;

// COLOR PALETTE INDEXES
static const TUint8 COLOR_BORDER1 = 153;
static const TUint8 COLOR_BORDER2 = 155;

static const TUint8 COLOR_TIMER_INNER  = 250;
static const TUint8 COLOR_TIMER_BORDER = 251;

// user interface/widgets colors
static const TUint8 LASSO_1           = 247;
static const TUint8 LASSO_2           = 248;
static const TUint8 COLOR_DIALOG_BG   = 249;
static const TUint8 COLOR_DIALOG_FG   = 250;
static const TUint8 COLOR_MENU_TITLE  = 251;
static const TUint8 COLOR_TEXT        = 252;
static const TUint8 COLOR_TEXT_SHADOW = 253;
static const TUint8 COLOR_TEXT_BG     = 254;

#endif //GRESOURCES_H
