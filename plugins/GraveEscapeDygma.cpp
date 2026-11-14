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

#include "GraveEscapeDygma.h"

#include "kaleidoscope/keyswitch_state.h"

#include <string.h>

namespace kaleidoscope {
namespace plugin {

static bool modifierActive(Key left, Key right)
{
  auto &keyboard = Runtime.hid().keyboard();

  return keyboard.isModifierKeyActive(left) || keyboard.isModifierKeyActive(right);
}

void GraveEscapeDygma::stateSet(uint8_t *bits, uint8_t i, bool on)
{
  if (on)
  {
    bits[i >> 3] |= (1 << (i & 7));
  }
  else
  {
    bits[i >> 3] &= ~(1 << (i & 7));
  }
}

bool GraveEscapeDygma::shouldSendGrave(void) const
{
  /* The snapshot covers the previous report. Modifiers handled earlier in the
   * current scan are already in the report being built, so look there too:
   * otherwise a Shift pressed in the same cycle as this key would be missed. */
  bool shift = shift_active_ || modifierActive(Key_LeftShift, Key_RightShift);
  bool gui   = gui_active_   || modifierActive(Key_LeftGui, Key_RightGui);
  bool ctrl  = ctrl_active_  || modifierActive(Key_LeftControl, Key_RightControl);
  bool alt   = alt_active_   || modifierActive(Key_LeftAlt, Key_RightAlt);

  if (GRAVE_ESC_CTRL_OVERRIDE && ctrl)
  {
    return false;
  }

  if (GRAVE_ESC_ALT_OVERRIDE && alt)
  {
    return false;
  }

  return (!GRAVE_ESC_SHIFT_OVERRIDE && shift) ||
         (!GRAVE_ESC_GUI_OVERRIDE && gui);
}

EventHandlerResult GraveEscapeDygma::onKeyswitchEvent(Key &mapped_key, KeyAddr key_addr, uint8_t key_state)
{
  if (mapped_key.getRaw() != ranges::GRAVE_ESCAPE)
  {
    return EventHandlerResult::OK;
  }

  /* Events without a physical location (superkey actions, macros) can't be
   * tracked per key, so each one is decided on its own. */
  if (!key_addr.isValid())
  {
    mapped_key = shouldSendGrave() ? Key_Backtick : Key_Escape;
    return EventHandlerResult::OK;
  }

  uint8_t i = key_addr.toInt();

  /* Decide on a fresh toggle-on, or if the key shows up already held without
   * one. A second toggle-on in the same cycle is the timeline's duplicate
   * delivery and keeps the choice. Re-deciding on later toggle-ons, rather
   * than trusting decided_, also recovers from a release that never arrived
   * (e.g. a side disconnecting mid-press). */
  bool fresh_press = keyToggledOn(key_state) && !stateGet(seen_this_cycle_, i);

  if (fresh_press || !stateGet(decided_, i))
  {
    stateSet(decided_, i, true);
    stateSet(grave_, i, shouldSendGrave());
  }

  stateSet(seen_this_cycle_, i, true);

  /* The report is rebuilt every cycle, so the key has to be translated on
   * every event of the press, holds and release included. */
  mapped_key = stateGet(grave_, i) ? Key_Backtick : Key_Escape;

  if (keyToggledOff(key_state))
  {
    stateSet(decided_, i, false);
    stateSet(grave_, i, false);
  }

  return EventHandlerResult::OK;
}

EventHandlerResult GraveEscapeDygma::beforeReportingState()
{
  shift_active_ = modifierActive(Key_LeftShift, Key_RightShift);
  gui_active_   = modifierActive(Key_LeftGui, Key_RightGui);
  ctrl_active_  = modifierActive(Key_LeftControl, Key_RightControl);
  alt_active_   = modifierActive(Key_LeftAlt, Key_RightAlt);

  memset(seen_this_cycle_, 0, sizeof(seen_this_cycle_));

  return EventHandlerResult::OK;
}

}
}

kaleidoscope::plugin::GraveEscapeDygma GraveEscape;
