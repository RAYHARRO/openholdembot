//******************************************************************************
//
// This file is part of the OpenHoldem project
//   Download page:         http://code.google.com/p/openholdembot/
//   Forums:                http://www.maxinmontreal.com/forums/index.php
//   Licensed under GPL v3: http://www.gnu.org/licenses/gpl.html
//
//******************************************************************************
//
// Purpose: Shooting replay-frames 
//   (at the right time and only once per turn).
//   Not really a symbol-engine, but easy to implement with this concept 
//
//******************************************************************************

#include "stdafx.h"
#include "CSymbolEngineReplayFrameController.h"

#include <assert.h>
#include "CAutoPlayer.h"
#include "CFunctionCollection.h"
#include "CHeartbeatThread.h"
#include "CLazyScraper.h"
#include "CPreferences.h"
#include "CReplayFrame.h"
#include "CStableFramesCounter.h"
#include "CSymbolEngineAutoplayer.h"
#include "CTableState.h"
#include "MagicNumbers.h"
#include "CSymbolEngineCasino.h"

CSymbolEngineReplayFrameController *p_symbol_engine_replayframe_controller = NULL;

CSymbolEngineReplayFrameController::CSymbolEngineReplayFrameController() {
	// The values of some symbol-engines depend on other engines.
	// As the engines get later called in the order of initialization
	// we assure correct ordering by checking if they are initialized.
  assert(p_symbol_engine_autoplayer != NULL);
  _heartbeat_of_last_replay_frame = 0;
}

CSymbolEngineReplayFrameController::~CSymbolEngineReplayFrameController() {
}

void CSymbolEngineReplayFrameController::InitOnStartup() {
	UpdateOnConnection();
}

void CSymbolEngineReplayFrameController::UpdateOnConnection() {
}

void CSymbolEngineReplayFrameController::UpdateOnHandreset() {
}

void CSymbolEngineReplayFrameController::UpdateOnNewRound() {
}

void CSymbolEngineReplayFrameController::UpdateOnMyTurn() {
}

void CSymbolEngineReplayFrameController::UpdateOnHeartbeat() {
	if(p_symbol_engine_casino->ConnectedToOHReplay()){
    // No point in shooting frames when connected to OHReplay
    write_log(preferences.debug_replayframes(), "[CSymbolEngineReplayFrameController] No replay required, as connected to OHReplay\n");
		return;	
	}
  if(!p_autoplayer->autoplayer_engaged()){
    // No point in shooting frames if the autoplayer is turned off
    write_log(preferences.debug_replayframes(), "[CSymbolEngineReplayFrameController] No replay required, as autoplayer turned off\n");
		return;	
	}
	if (p_lazyscraper->IsIdenticalScrape()) {
	  // There is no benefit in duplicate frames, so we abort
	  write_log(preferences.debug_replayframes(), "[CSymbolEngineReplayFrameController] No replay required, as identical scrape\n");
	  return;
	}  
	if ((preferences.replay_record() == kShootReplyFramesOnEveryChangeWhilePlaying)	
		  && p_table_state->User()->HasKnownCards()) {
		write_log(preferences.debug_replayframes(), "[CSymbolEngineReplayFrameController] Replay required (on change while in hand)\n");
	  ShootReplayFrameIfNotYetDone();
	  return;
	}
	if (preferences.replay_record() == kShootReplyFramesOnEveryChange) {
	  write_log(preferences.debug_replayframes(), "[CSymbolEngineReplayFrameController] Replay required (on every change in table-state)\n");
	  ShootReplayFrameIfNotYetDone();
	  return;
	}
  bool shoot_replay_frame_by_formula = p_function_collection->Evaluate(
    k_standard_function_names[k_standard_function_shoot_replay_frame], 
    preferences.log_hopper_functions());
  if (shoot_replay_frame_by_formula) {
    write_log(preferences.debug_replayframes(), "[CSymbolEngineReplayFrameController] Replay required by f$shoot_replay_frame\n");
    ShootReplayFrameIfNotYetDone();
    return;
  }
}

void CSymbolEngineReplayFrameController::UpdateAfterAutoplayerAction(int autoplayer_action_code) {
  if (preferences.replay_record() == kShootReplyFramesOnMyTurn) {
    // Replay-frame already in scraper memory, it only needs to be stored.
    // Therefore it does not matter if we shoot "after" action.
    write_log(preferences.debug_replayframes(), "[CSymbolEngineReplayFrameController] Replay required (on my turn and time to act)\n");
    ShootReplayFrameIfNotYetDone();
  }
}

void CSymbolEngineReplayFrameController::ShootReplayFrameIfNotYetDone() {
  int heartbeat_counter = p_heartbeat_thread->heartbeat_counter();
  // Don't shoot replay-frames twice per heartbeat
  if (_heartbeat_of_last_replay_frame == heartbeat_counter) {
    write_log(preferences.debug_replayframes(), "[CSymbolEngineReplayFrameController] Not shooting a replay-frame, because we already shot one this heartbeat\n");
    return;
  }
  write_log(preferences.debug_replayframes(), "[CSymbolEngineReplayFrameController] Going to shooting a replay-frame\n");
  CReplayFrame crf;
  crf.CreateReplayFrame();
  _heartbeat_of_last_replay_frame = heartbeat_counter;
}