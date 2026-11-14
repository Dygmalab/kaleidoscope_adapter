/* -*- mode: c++ -*-
 * Kaleidoscope-GraveEscapeDygma -- Grave Escape key for Dygma keyboards
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

/* Keymap helper for the Grave Escape key. */
#define Key_GraveEscape Key(kaleidoscope::ranges::GRAVE_ESCAPE)

namespace kaleidoscope {
namespace plugin {

/* Grave Escape, after QMK's QK_GESC: one key that sends
 *
 *   Escape           on its own,
 *   Shift + `  (~)   while Shift is held,
 *   GUI + `          while GUI is held (e.g. Cmd+` window cycling on macOS).
 *
 * It only acts on its own keycode, ranges::GRAVE_ESCAPE, so plain Escape keys
 * are never affected. The key is turned into Key_Escape or Key_Backtick and the
 * rest of the chain sees an ordinary key.
 *
 * Which modifiers count as held is read from the current state, never from a
 * past report alone:
 *
 *   - Modifier keys physically held right now. The matrix is scanned row by
 *     row and one wireless packet can carry several changes, so a Shift on a
 *     lower row may not be in the report yet when this key is handled -- or
 *     may still be in last cycle's report after being released.
 *   - One-shot modifiers that are active right now (OneShot::isModifierActive).
 *   - Modifiers already in the report being built in this scan.
 *   - Modifiers that were in the last report but come from neither of the first
 *     two sources -- e.g. a qukey resolved to its Shift hold, which Qukeys adds
 *     at that key's own position in the scan, possibly after this key.
 *
 * Placement: registered AFTER keyRoleManager and OneShot.
 *
 *   - After keyRoleManager, so a pending qukey (e.g. hold = Shift) resolves
 *     before the choice is made: Qukeys queues this key and replays it once
 *     the Shift is in the report. A Grave Escape inside a superkey action is
 *     delivered at the superkey's position, like a physical press.
 *
 *   - After OneShot, so the report snapshot taken in beforeReportingState()
 *     already includes the one-shot modifiers OneShot adds there.
 *
 * The choice is made on the toggle-on of a press and kept until the key is
 * released, so releasing Shift before the key never leaves a stuck or swapped
 * key in the report. A second toggle-on within the same cycle (the timeline's
 * duplicate delivery) keeps the choice already made. Events without a
 * location (dynamic macros) keep the choice from their press for their
 * release.
 */
class GraveEscapeDygma : public kaleidoscope::Plugin {
 public:
  GraveEscapeDygma(void) {}

  EventHandlerResult onKeyswitchEvent(Key &mapped_key, KeyAddr key_addr, uint8_t key_state);
  EventHandlerResult beforeReportingState();

 private:
  static constexpr uint8_t STATE_BYTES = (KeyAddr::upper_limit + 7) / 8;

  /* Shift / GUI that were in the last report without being a physically held
   * modifier key or an active one-shot (e.g. a qukey's resolved hold). */
  bool injected_shift_ = false;
  bool injected_gui_   = false;

  /* One bit per key address: the key is down and its choice is made ... */
  uint8_t decided_[STATE_BYTES] = {0};
  /* ... and that choice was ` rather than Escape. */
  uint8_t grave_[STATE_BYTES]   = {0};
  /* Seen during the current scan; cleared in beforeReportingState(). Tells a
   * duplicate toggle-on in the same cycle apart from a new press. */
  uint8_t seen_this_cycle_[STATE_BYTES] = {0};

  /* The same choice for events without a location (dynamic macros). */
  bool unlocated_decided_ = false;
  bool unlocated_grave_   = false;

  bool shouldSendGrave(void) const;

  static bool stateGet(const uint8_t *bits, uint8_t i) { return bits[i >> 3] & (1 << (i & 7)); }
  static void stateSet(uint8_t *bits, uint8_t i, bool on);
};

}
}

extern kaleidoscope::plugin::GraveEscapeDygma GraveEscape;
