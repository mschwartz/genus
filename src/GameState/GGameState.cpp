#include "Game.h"
#include "GGameBoard.h"
#include "GGameStateGameOverProcess.h"
#include "Playfields/GStage1Countryside.h"
#include "Playfields/GStage5Cyberpunk.h"
#include "Playfields/GStage2UnderWaterOne.h"
#include "Playfields/GStage3GlacialMountains.h"
#include "Playfields/GStage4UnderWaterFantasy.h"
#include "Playfields/GStage6Space.h"
#include "PauseModal/GPauseModal.h"
#include "PauseModal/GPauseProcess.h"

#ifdef __XTENSA__
#define PAUSE_MODAL_Y 50
#else
#define PAUSE_MODAL_Y 60
#endif

#ifdef CHICKEN_MODE
class GGameState;
class ChickenModeProcess : public BProcess {
  public:
    ChickenModeProcess(GGameState *aState) : BProcess() {
      mState = aState;
    }

    ~ChickenModeProcess() {}

    TBool RunBefore() {
      if (gControls.WasPressed(BUTTON_SELECT)) {
        while (mState->mLevel % 5 > 0) {
          mState->mLevel++;
        }
        mState->mBlocksRemaining = 0;
      }
      return ETrue;
    }

    TBool RunAfter() {
      return ETrue;
    }

    GGameState *mState;
};
#endif

/****************************************************************************************************************
 ****************************************************************************************************************
 ****************************************************************************************************************/

GGameState::GGameState() : BGameEngine(gViewPort) {
  mLevel      = 1;
  mGameOver   = EFalse;
  mIsPaused   = EFalse;
  mPauseModal = ENull;
  mPlayfield  = ENull;
  mPowerup    = ENull;
  mBonusTimer = -1;

  gResourceManager.LoadBitmap(COMMON_SPRITES_BMP, COMMON_SLOT, IMAGE_16x16);

  mFont8  = new BFont(gResourceManager.GetBitmap(FONT_8x8_SLOT), FONT_8x8);
  mFont16 = new BFont(gResourceManager.GetBitmap(FONT_16x16_SLOT), FONT_16x16);

  if (gOptions->gameProgress.savedState) {
    LoadState();
    SetBlocksPerLevel();
  } else {
    mGameBoard.Clear();
  }

  LoadLevel(gOptions->gameProgress.savedState);

  mSprite = new GPlayerSprite();
  AddSprite(mSprite);
  mSprite->x  = PLAYER_X;
  mSprite->y  = PLAYER_Y;
  mSprite->vy = 0;

  mNextSprite = new GPlayerSprite();
  AddSprite(mNextSprite);
  mNextSprite->flags |= SFLAG_RENDER | SFLAG_NEXT_BLOCK;
  mNextSprite->x = NEXT_BLOCK_X;
  mNextSprite->y = NEXT_BLOCK_Y;
  mNextSprite->Randomize();

  mGameProcess = new GNoPowerup(mSprite, this);
  AddProcess(mGameProcess);
  AddProcess(new GPauseProcess(this));

#ifdef CHICKEN_MODE
  AddProcess(new ChickenModeProcess(this));
#endif

  if (gOptions->gameProgress.savedState) {
    LoadPlayerState();
  } else {
    Next(EFalse);
  }
}

GGameState::~GGameState() {
  gResourceManager.ReleaseBitmapSlot(COMMON_SLOT);
  gResourceManager.ReleaseBitmapSlot(PLAYER_SLOT);
  delete mFont16;
  delete mFont8;
}

/**
 * Next - process next piece
 *
 * This is called by the various *Powerup processes when they are done with their work.
 * For example, M Bomb process will control the piece, place it, loop while blowing up the appropriate pieces,
 * then will call Next() before committing suicide (return EFalse from Run*).
 *
 * This routine will then either copy mNextSprite blocks to mPlayerSprite or if a powerup is possible, maybe
 * will spawn a random powerup.
 *
 * @param aCanPowerup true if Next piece can be a powerup
 */
void GGameState::Next(TBool aCanPowerup) {
  mSprite->x = PLAYER_X;
  mSprite->y = PLAYER_Y;

  if (mGameOver || mGameBoard.IsGameOver()) {
    mSprite->Copy(mNextSprite);
    mNextSprite->Randomize();
    mGameProcess->Signal();
    return;
  }

  if (aCanPowerup) {
    TInt maybe = 0;

    switch (gOptions->difficulty) {
      case DIFFICULTY_EASY:
        maybe = Random(15, 20);
        break;
      case DIFFICULTY_INTERMEDIATE:
        maybe = Random(15, 22);
        break;
      case DIFFICULTY_HARD:
        maybe = Random(15, 24);
        break;
      default:
        Panic("DifficultyString: invalid difficulty %d", gOptions->difficulty);
    }

    if (maybe == 16) {
      if (Random() & 1) {
        mPowerup = new GModusBombPowerup(mSprite, this);
        gOptions->gameProgress.playerType = PLAYER_MODUS_BOMB;
	  } else if (mGameBoard.HasColorSwappableBlocks()) {
        mPowerup = new GColorSwapPowerup(mSprite, this);
        gOptions->gameProgress.playerType = PLAYER_COLOR_SWAP;
      }
      if (mPowerup) {
        AddProcess(mPowerup);
      }
      return;
    }
  } else {
    mPowerup = ENull;
    gOptions->gameProgress.playerType = PLAYER_NO_POWERUP;
  }

  // NOT a powerup or powerup criteria not met
  mSprite->Copy(mNextSprite);
  mNextSprite->Randomize();
  mGameProcess->Signal();
}

/****************************************************************************************************************
 ****************************************************************************************************************
 ****************************************************************************************************************/

void GGameState::GameOver() {
  mSprite->flags &= ~(SFLAG_RENDER | SFLAG_ANIMATE);
  mGameProcess->Wait();
  mBonusTimer = -1;
  mGameOver = ETrue;
  AddProcess(new GGameStateGameOverProcess(this));
  THighScoreTable h;
  h.Load();
  h.lastScore[gOptions->difficulty].mValue = mScore.mValue;
  h.Save();
  gSoundPlayer.PlayMusic(GAMEOVER_XM);

  // Reset the game state and save it
  SaveState();
}

/****************************************************************************************************************
 ****************************************************************************************************************
 ****************************************************************************************************************/

void GGameState::PreRender() {
  if (mPauseModal) {
    mPauseModal->Run();
  }
}

/****************************************************************************************************************
 ****************************************************************************************************************
 ****************************************************************************************************************/

void GGameState::LoadLevel(TBool aForceStageLoad) {
  TBool newStage = EFalse;

  if ((mLevel % 5) == 1) {  // every 6th level
    newStage = ETrue;

    if (mPlayfield) {
      delete mPlayfield;
    }

    SetBlocksPerLevel();

    // Release only if bitmap was loaded
    if (gResourceManager.GetBitmap(PLAYER_SLOT)) {
      gResourceManager.ReleaseBitmapSlot(PLAYER_SLOT);
    }
  }

  if (newStage || aForceStageLoad) {
    TUint8 levelToLoad = mLevel <= 0 || newStage ? (mLevel / 5) % 6 : ((mLevel - 1) / 5) % 6;

    switch (levelToLoad) {
      case 0:
        mPlayfield = new GStage1Countryside(this);
        gResourceManager.LoadBitmap(STAGE1_SPRITES_BMP, PLAYER_SLOT, IMAGE_16x16);
        gSoundPlayer.PlayMusic(COUNTRYSIDE_XM);
        break;
      case 1:
        mPlayfield = new GStage2UnderWaterOne(this);
        gResourceManager.LoadBitmap(STAGE2_SPRITES_BMP, PLAYER_SLOT, IMAGE_16x16);
        gSoundPlayer.PlayMusic(UNDER_WATER_XM);
        break;
      case 2:
        mPlayfield = new GStage3GlacialMountains(this);
        gResourceManager.LoadBitmap(STAGE3_SPRITES_BMP, PLAYER_SLOT, IMAGE_16x16);
        gSoundPlayer.PlayMusic(GLACIAL_MOUNTAINS_XM);
        break;
      case 3:
        mPlayfield = new GStage4UnderWaterFantasy(this);
        gResourceManager.LoadBitmap(STAGE4_SPRITES_BMP, PLAYER_SLOT, IMAGE_16x16);
        gSoundPlayer.PlayMusic(UNDERWATERFANTASY_XM);
        break;
      case 4:
        mPlayfield = new GStage5Cyberpunk(this);
        gResourceManager.LoadBitmap(STAGE5_SPRITES_BMP, PLAYER_SLOT, IMAGE_16x16);
        gSoundPlayer.PlayMusic(CYBERPUNK_XM);
        break;
      case 5:
        mPlayfield = new GStage6Space(this);
        gResourceManager.LoadBitmap(STAGE6_SPRITES_BMP, PLAYER_SLOT, IMAGE_16x16);
        gSoundPlayer.PlayMusic(SPAAACE_XM);
        break;
      default:
        Panic("LoadLevel invalid level\n");
    }
  }

  if (newStage && mLevel > 1) {
    gSoundPlayer.SfxNextStage();
  }
  else if (mLevel > 1) {
    gSoundPlayer.SfxNextLevel();
  }

  BBitmap *playerBitmap = gResourceManager.GetBitmap(PLAYER_SLOT);
  mBackground = gResourceManager.GetBitmap(BKG_SLOT);

  gDisplay.SetPalette(mBackground, 0, 128);
  gDisplay.SetPalette(playerBitmap, 128, 128);
  gDisplay.SetColor(COLOR_TEXT, 255, 255, 255);
  gDisplay.SetColor(COLOR_TEXT_SHADOW, 0, 0, 0);

  SetPauseModalTheme();

  mBlocksRemaining = mBlocksThisLevel;
}

/****************************************************************************************************************
 ****************************************************************************************************************
 ****************************************************************************************************************/

void GGameState::RenderTimer() {
  BBitmap *bm = gDisplay.renderBitmap;
  if (mBonusTimer >= 0) {

    bm->DrawStringShadow(ENull, "Time", mFont16, TIMER_X, TIMER_Y, COLOR_TEXT, COLOR_TEXT_SHADOW, -1, -6);
    // frame
    bm->DrawRect(ENull, TIMER_BORDER.x1, TIMER_BORDER.y1, TIMER_BORDER.x2, TIMER_BORDER.y2, COLOR_TIMER_BORDER);
    // inner
    const TInt   timer_width = TIMER_INNER.x2 - TIMER_INNER.x1;
    const TFloat pct         = TFloat(mBonusTimer) / TFloat(mBonusTime);
    const TInt   width       = TInt(pct * timer_width);
    bm->FillRect(ENull, TIMER_INNER.x1, TIMER_INNER.y1, TIMER_INNER.x1 + width, TIMER_INNER.y2, COLOR_TIMER_INNER);
  }
}

void GGameState::RenderScore() {
  BBitmap *bm = gDisplay.renderBitmap;
  char    score_text[12];

  for (TInt i   = 0; i < 8; i++) {
    TInt v = (mScore.mValue >> ((7 - i) * 4)) & 0x0f;
    score_text[i] = '0' + char(v);
  }
  score_text[8] = '\0';
  bm->DrawStringShadow(ENull, score_text, mFont16, SCORE_X, SCORE_Y, COLOR_TEXT, COLOR_TEXT_SHADOW, -1, -6);
  bm->DrawStringShadow(ENull, gOptions->DifficultyString(), mFont8, SCORE_X+4, SCORE_Y+18, COLOR_TEXT, COLOR_TEXT_SHADOW, -1, -1);
}

void GGameState::RenderLevel() {
  BBitmap *bm = gDisplay.renderBitmap;
  TBCD level;
  level.FromUint32(mLevel);
  char    lev[20];
  level.ToString(lev, ENull);
  char out[32];
  switch (strlen(lev)) {
    case 1:
      strcpy(out, "Level  ");
      strcat(out, lev);
      break;
    case 2:
      strcpy(out, "Level ");
      strcat(out, lev);
      break;
    case 3:
      strcpy(out, "Level");
      strcat(out, lev);
      break;
    default:
      // Levels are limited to 999 in UI
      strcpy(out, "Level");
      strcat(out, "999");
  }

  bm->DrawStringShadow(ENull, out, mFont16, LEVEL_X, LEVEL_Y, COLOR_TEXT, COLOR_TEXT_SHADOW, -1, -6);
}

void GGameState::RenderNext() {
  BBitmap *bm = gDisplay.renderBitmap;
  bm->DrawStringShadow(ENull, "Next", mFont16, NEXT_X, NEXT_Y, COLOR_TEXT, COLOR_TEXT_SHADOW, -1, -6);

  // On HARD difficulty draw a "?" over a filled block
  if (gOptions->difficulty == DIFFICULTY_HARD) {
    bm->DrawStringShadow(ENull, "?", mFont16, NEXT_BLOCK_X + 8, NEXT_BLOCK_Y + 8, COLOR_TEXT, COLOR_TEXT_SHADOW, -1);
  }
}

void GGameState::RenderMovesLeft() {
  BBitmap *bm = gDisplay.renderBitmap;

  // frame
  bm->DrawRect(ENull, MOVES_BORDER.x1, MOVES_BORDER.y1, MOVES_BORDER.x2, MOVES_BORDER.y2, COLOR_BORDER1);
  // inner
  const TInt   moves_width = MOVES_INNER.x2 - MOVES_INNER.x1;
  const TFloat pct         = TFloat(mBlocksRemaining) / TFloat(mBlocksThisLevel);
  const TInt   width       = TInt(pct * moves_width);
  bm->FillRect(ENull, MOVES_INNER.x1, MOVES_INNER.y1, MOVES_INNER.x1 + width, MOVES_INNER.y2, COLOR_BORDER2);
}

void GGameState::RenderPauseModal() {
  if (mIsPaused) {
    if (!mPauseModal) {
      mPauseModal = new GPauseModal(20, PAUSE_MODAL_Y);
    }
    mGameBoard.Hide();
    mPauseModal->Render(30, 20);
  } else if (mPauseModal) {
    mGameBoard.Show();
    delete mPauseModal;
    mPauseModal = ENull;
  }
}

/****************************************************************************************************************
 ****************************************************************************************************************
 ****************************************************************************************************************/

// render on top of the background
void GGameState::PostRender() {
  if (!mGameOver && mBlocksRemaining < 1) {
    mLevel++;
    LoadLevel();
  }

  if (!mIsPaused) {
    RenderTimer();
    RenderScore();
    RenderLevel();
    RenderMovesLeft();
    RenderNext();
  }
  RenderPauseModal();
}

void GGameState::SetBlocksPerLevel() {
  mBlocksThisLevel = 20 + mLevel*5 + gOptions->difficulty * 10;
  switch(gOptions->difficulty) {
    case DIFFICULTY_EASY:
      mBonusTime = 20 * 30;
      break;
    case DIFFICULTY_INTERMEDIATE:
      mBonusTime = 15 * 30;
      break;
    case DIFFICULTY_HARD:
      mBonusTime = 10 * 30;
      break;
  }
}

void GGameState::SaveState() {
  if (mGameOver) {
    gOptions->ResetGameProgress();
    gOptions->Save();
    return;
  }

  gOptions->gameProgress.savedState = ETrue;
  gOptions->gameProgress.level = mLevel;
  gOptions->gameProgress.bonusTimer = mBonusTimer;
  gOptions->gameProgress.blocksRemaining = mBlocksRemaining;
  gOptions->gameProgress.score = mScore;
  gOptions->gameProgress.difficulty = gOptions->difficulty;

  memcpy(gOptions->gameProgress.board, mGameBoard.mBoard, sizeof(mGameBoard.mBoard));
  memcpy(gOptions->gameProgress.playerBlocks, mSprite->mBlocks, sizeof(mSprite->mBlocks));
  memcpy(gOptions->gameProgress.nextBlocks, mNextSprite->mBlocks, sizeof(mNextSprite->mBlocks));

  gOptions->Save();
}

void GGameState::LoadState() {
  mLevel = gOptions->gameProgress.level;
  mBonusTimer = gOptions->gameProgress.bonusTimer;
  mBlocksRemaining = gOptions->gameProgress.blocksRemaining;
  mScore = gOptions->gameProgress.score;

  // Reset previous game state's difficulty
  gOptions->difficulty = gOptions->gameProgress.difficulty;

  memcpy(mGameBoard.mBoard, gOptions->gameProgress.board, sizeof(mGameBoard.mBoard));
}

void GGameState::LoadPlayerState() {
  memcpy(mSprite->mBlocks, gOptions->gameProgress.playerBlocks, sizeof(mSprite->mBlocks));
  memcpy(mNextSprite->mBlocks, gOptions->gameProgress.nextBlocks, sizeof(mNextSprite->mBlocks));

  mGameProcess->Signal(gOptions->gameProgress.gameState);

  switch (gOptions->gameProgress.playerType) {
    case PLAYER_MODUS_BOMB:
      mPowerup = new GModusBombPowerup(mSprite, this);
      AddProcess(mPowerup);
      break;
    case PLAYER_COLOR_SWAP:
      mPowerup = new GColorSwapPowerup(mSprite, this);
      AddProcess(mPowerup);
      break;
    case PLAYER_NO_POWERUP:
    default:
      return;
  }
}

void GGameState::SetPauseModalTheme() {
  gWidgetTheme.Configure(
      WIDGET_TEXT_FONT, mFont16,
      WIDGET_TEXT_FG, COLOR_TEXT,
      WIDGET_TEXT_BG, COLOR_TEXT_BG,
      WIDGET_TITLE_FONT, mFont16,
      WIDGET_TITLE_FG, COLOR_TEXT,
      WIDGET_TITLE_BG, -1,
      WIDGET_WINDOW_BG, -1,
      WIDGET_WINDOW_FG, -1,
      WIDGET_SLIDER_FG, COLOR_TEXT_BG,
      WIDGET_SLIDER_BG, COLOR_TEXT,
      WIDGET_END_TAG);

  gDisplay.SetColor(COLOR_TEXT_BG, 255, 92, 93);
}
