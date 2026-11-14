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

/* Optional build-time tweaks, named after QMK's equivalents. Define any of
 * them to 1 to enable it. */
#ifndef GRAVE_ESC_SHIFT_OVERRIDE
#define GRAVE_ESC_SHIFT_OVERRIDE 0  /* Shift no longer turns the key into ` */
#endif
#ifndef GRAVE_ESC_GUI_OVERRIDE
#define GRAVE_ESC_GUI_OVERRIDE   0  /* GUI no longer turns the key into ` */
#endif
#ifndef GRAVE_ESC_CTRL_OVERRIDE
#define GRAVE_ESC_CTRL_OVERRIDE  0  /* Ctrl held always sends Escape (Ctrl+Shift+Esc) */
#endif
#ifndef GRAVE_ESC_ALT_OVERRIDE
#define GRAVE_ESC_ALT_OVERRIDE   0  /* Alt held always sends Escape */
#endif

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
 * Placement: registered AFTER keyRoleManager and OneShot.
 *
 *   - After keyRoleManager, so a pending qukey (e.g. hold = Shift) resolves
 *     before the choice is made: Qukeys queues this key and replays it once
 *     the Shift is in the report. The SuperKeys timeline also passes the key
 *     through as an ordinary one, and a Grave Escape placed inside a superkey
 *     action reaches us as an INJECTED event.
 *
 *   - After OneShot, so the modifier snapshot taken in beforeReportingState()
 *     already includes one-shot modifiers, which OneShot adds there.
 *
 *   - Before KbdInterface, which looks its key table up by the exact raw value
 *     and expects the final Key_Escape / Key_Backtick.
 *
 * The choice is made on the toggle-on of a press and kept until the key is
 * released, so releasing Shift before the key never leaves a stuck or swapped
 * key in the report. A second toggle-on within the same cycle (the timeline's
 * duplicate delivery) keeps the choice already made.
 */
class GraveEscapeDygma : public kaleidoscope::Plugin {
 public:
  GraveEscapeDygma(void) {}

  EventHandlerResult onKeyswitchEvent(Key &mapped_key, KeyAddr key_addr, uint8_t key_state);
  EventHandlerResult beforeReportingState();

 private:
  static constexpr uint8_t STATE_BYTES = (KeyAddr::upper_limit + 7) / 8;

  /* Modifiers present in the last fully built report. */
  bool shift_active_ = false;
  bool gui_active_   = false;
  bool ctrl_active_  = false;
  bool alt_active_   = false;

  /* One bit per key address: the key is down and its choice is made ... */
  uint8_t decided_[STATE_BYTES] = {0};
  /* ... and that choice was ` rather than Escape. */
  uint8_t grave_[STATE_BYTES]   = {0};
  /* Seen during the current scan; cleared in beforeReportingState(). Tells a
   * duplicate toggle-on in the same cycle apart from a new press. */
  uint8_t seen_this_cycle_[STATE_BYTES] = {0};

  bool shouldSendGrave(void) const;

  static bool stateGet(const uint8_t *bits, uint8_t i) { return bits[i >> 3] & (1 << (i & 7)); }
  static void stateSet(uint8_t *bits, uint8_t i, bool on);
};

}
}

extern kaleidoscope::plugin::GraveEscapeDygma GraveEscape;
