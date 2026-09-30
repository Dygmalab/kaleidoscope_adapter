/* -*- mode: c++ -*-
 * Kaleidoscope-CombosDygma -- Combo keys for Dygma keyboards
 * Copyright (C) 2026 Dygma Lab S.L. www.dygma.com
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "kbd_core.h"

#include "kaleidoscope/Runtime.h"
#include <Kaleidoscope-Ranges.h>

namespace kaleidoscope {
namespace plugin {

/* Pressing several keys at once fires a single action.
 *
 * Two decisions shape the whole implementation:
 *
 * 1. Combos are indexed by PHYSICAL POSITION, not by the resolved key. Matching
 *    therefore does not depend on the keymap, which is what lets this plugin
 *    run first in the chain without interacting with the Qukeys/SuperKeys
 *    resolution at all. It also makes a combo stable across layers and makes
 *    the host UI direct: you tick physical keys.
 *
 * 2. The action is emitted by REWRITING the event of the last member pressed
 *    (the "anchor"), NOT by injecting a new event with INJECTED. Injection
 *    would silently kill two action types: SuperKeys and OneShot both ignore
 *    INJECTED events. Rewriting means the action flows through Qukeys,
 *    SuperKeys, DynamicMacros, OneShot and Layer::eventHandler exactly like a
 *    real keypress, with a valid KeyAddr -- so every action type works.
 */
class CombosDygma : public kaleidoscope::Plugin {
 public:
  static constexpr uint8_t MAX_COMBOS  = 32;
  static constexpr uint8_t MAX_MEMBERS = 4;
  static constexpr uint8_t MIN_MEMBERS = 2;

  /* 0xFF in a position slot means "unused"; 0xFF in `layer` means "any". */
  static constexpr uint8_t POSITION_UNUSED = 0xFF;
  static constexpr uint8_t LAYER_ANY       = 0xFF;

  /* flags bits */
  static constexpr uint8_t FLAG_ENABLED = 0x01;

  static constexpr uint16_t DEFAULT_MATCH_WINDOW_MS = 10;
  static constexpr uint16_t MIN_MATCH_WINDOW_MS     = 5;
  static constexpr uint16_t MAX_MATCH_WINDOW_MS     = 500;

  typedef struct PACK
  {
      uint8_t  positions[MAX_MEMBERS];  /* KeyAddr::toInt(), 0xFF = unused   */
      uint8_t  layer;                   /* 0xFF = every layer                */
      uint8_t  flags;                   /* bit0 enabled                      */
      uint16_t action;                  /* Key raw value                     */
  } combo_entry_t;                      /* 8 B                               */

  typedef struct PACK
  {
      uint16_t      match_window_ms;    /* default 10; 0xFFFF => auto-heal   */
      uint8_t       combos_count;       /* 0xFF => 0                         */
      uint8_t       reserved;
      combo_entry_t combos[MAX_COMBOS];
  } combos_config_t;                    /* 260 B                             */

 public:
  CombosDygma(void) {}

  EventHandlerResult onKeyswitchEvent(Key &mapped_key, KeyAddr key_addr, uint8_t key_state);
  EventHandlerResult beforeReportingState();
  EventHandlerResult onFocusEvent(const char *command);
  EventHandlerResult onSetup();

 private:
  enum class State : uint8_t {
    IDLE,        /* nothing buffered                                   */
    COLLECTING,  /* some members down, waiting for a match or timeout  */
    ACTIVE,      /* a combo matched; the action is being held          */
  };

  struct BufferedPress {
    KeyAddr addr;
    Key     key;    /* the RESOLVED key, kept so a flush re-emits what the
                     * keymap said at press time rather than what it says at
                     * flush time -- a layer may have changed in between. */
  };

  State   state_ = State::IDLE;

  BufferedPress buffer_[MAX_MEMBERS];
  uint8_t       buffer_len_   = 0;
  uint32_t      buffer_start_ = 0;

  /* Active combo bookkeeping. */
  uint8_t member_positions_[MAX_MEMBERS];
  uint8_t member_count_ = 0;
  KeyAddr anchor_;
  Key     action_key_;

  /* Re-entrancy guard. It must cover the WHOLE flush loop and mean "do not
   * buffer", not merely "do not consume": when the Qukeys queue fills up it
   * calls processQueue() synchronously from inside onKeyswitchEvent, which
   * re-enters handleKeyswitchEvent for a DIFFERENT address while we are still
   * inside our own flush. */
  bool flushing_ = false;

  /* ----------------------------------------------------------------------
   * Address ownership, one bit per key.
   *
   * `flushed_`: keys this plugin has already released to the chain. Qukeys
   * swallows them into its queue and replays them later WITHOUT the INJECTED
   * flag, so a replay looks exactly like a fresh physical press -- and
   * re-buffering it would loop forever, with the key never reaching the host.
   * Marked keys pass straight through until they are physically released.
   *
   * `owned_`: members of a combo that fired. They stay owned until physically
   * released, even after the combo ends: the user is still holding the chord,
   * and those keys must not start typing themselves. This covers the anchor
   * too, whose address still resolves to the action through the live composite
   * keymap.
   * -------------------------------------------------------------------- */
  static constexpr uint8_t MAX_TRACKED_KEYS = 128;

  uint8_t flushed_[MAX_TRACKED_KEYS / 8] = {0};
  uint8_t owned_[MAX_TRACKED_KEYS / 8]   = {0};

  static bool addrFlagGet(const uint8_t *map, KeyAddr key_addr) {
    if (!key_addr.isValid()) return false;
    uint8_t offset = key_addr.toInt();
    if (offset >= MAX_TRACKED_KEYS) return false;
    return (map[offset >> 3] & (1 << (offset & 7))) != 0;
  }

  static void addrFlagSet(uint8_t *map, KeyAddr key_addr, bool value) {
    if (!key_addr.isValid()) return;
    uint8_t offset = key_addr.toInt();
    if (offset >= MAX_TRACKED_KEYS) return;
    if (value) {
      map[offset >> 3] |= (1 << (offset & 7));
    } else {
      map[offset >> 3] &= ~(1 << (offset & 7));
    }
  }

  static void offsetFlagSet(uint8_t *map, uint8_t offset, bool value) {
    if (offset >= MAX_TRACKED_KEYS) return;
    if (value) {
      map[offset >> 3] |= (1 << (offset & 7));
    } else {
      map[offset >> 3] &= ~(1 << (offset & 7));
    }
  }

  const combos_config_t * p_combos_config = nullptr;

  /* --- matching ------------------------------------------------------- */

  bool  isEnabled(const combo_entry_t &combo) const;
  bool  comboAppliesToActiveLayer(const combo_entry_t &combo) const;
  bool  isMemberPosition(uint8_t offset) const;
  bool  positionAllowed(KeyAddr key_addr, Key mapped_key) const;
  bool  actionAllowed(uint16_t raw) const;
  int   findCompletedCombo() const;

  /* --- buffer --------------------------------------------------------- */

  bool isBuffered(KeyAddr key_addr) const;
  void bufferPress(KeyAddr key_addr, Key mapped_key);
  void flushBuffer();
  void clearBuffer();

  /* --- active combo --------------------------------------------------- */

  void activateCombo(uint8_t index, KeyAddr anchor, Key &mapped_key);
  bool isActiveMember(KeyAddr key_addr) const;
  void endCombo(bool release_came_from_anchor);

  /* Puts every piece of volatile state back to power-on values. A real reboot
   * gets this for free from static initialisation; doing it explicitly makes
   * onSetup() idempotent, which is what the host test harness relies on to
   * keep tests independent of each other. */
  bool isPhysicalRelease(uint8_t key_state) const;

  void resetVolatileState();

  void cfgmem_match_window_save( uint16_t match_window_ms );
  void cfgmem_combo_save( uint8_t index, const combo_entry_t * p_entry );
  void cfgmem_count_save( uint8_t count );
};

}
}

extern kaleidoscope::plugin::CombosDygma Combos;
