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
#include "OneShotDygma.h"

#include "kaleidoscope/keyswitch_state.h"
#include "kaleidoscope/layers.h"

#include <string.h>

namespace kaleidoscope {
namespace plugin {

namespace {

struct Mods
{
  bool shift;
  bool gui;
};

/* Modifier keys physically held right now, as mapped in the live keymap. */
Mods physicalMods(void)
{
  Mods mods = { false, false };

  for (auto key_addr : KeyAddr::all())
  {
    if (!Runtime.device().isKeyswitchPressed(key_addr))
    {
      continue;
    }

    Key key = Layer.lookup(key_addr);

    if (key.getFlags() & (SYNTHETIC | RESERVED))
    {
      continue;
    }

    if (key == Key_LeftShift || key == Key_RightShift)
    {
      mods.shift = true;
    }
    else if (key == Key_LeftGui || key == Key_RightGui)
    {
      mods.gui = true;
    }
  }

  return mods;
}

Mods oneShotMods(void)
{
  return {
    ::OneShot.isModifierActive(Key_LeftShift) || ::OneShot.isModifierActive(Key_RightShift),
    ::OneShot.isModifierActive(Key_LeftGui) || ::OneShot.isModifierActive(Key_RightGui),
  };
}

/* Modifiers in the HID report currently being built. */
Mods reportMods(void)
{
  auto &keyboard = Runtime.hid().keyboard();

  return {
    keyboard.isModifierKeyActive(Key_LeftShift) || keyboard.isModifierKeyActive(Key_RightShift),
    keyboard.isModifierKeyActive(Key_LeftGui) || keyboard.isModifierKeyActive(Key_RightGui),
  };
}

}  // namespace

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
  Mods physical = physicalMods();
  Mods one_shot = oneShotMods();
  Mods report   = reportMods();

  bool shift = physical.shift || one_shot.shift || report.shift || injected_shift_;
  bool gui   = physical.gui   || one_shot.gui   || report.gui   || injected_gui_;

  return shift || gui;
}

EventHandlerResult GraveEscapeDygma::onKeyswitchEvent(Key &mapped_key, KeyAddr key_addr, uint8_t key_state)
{
  if (mapped_key.getRaw() != ranges::GRAVE_ESCAPE)
  {
    return EventHandlerResult::OK;
  }

  /* Events without a location (dynamic macros) share one slot: a macro presses
   * and releases a key in order, so the release reuses the press's choice and
   * never leaves the other key behind in the report. */
  if (!key_addr.isValid())
  {
    if (keyToggledOn(key_state) || !unlocated_decided_)
    {
      unlocated_decided_ = true;
      unlocated_grave_   = shouldSendGrave();
    }

    mapped_key = unlocated_grave_ ? Key_Backtick : Key_Escape;

    if (keyToggledOff(key_state))
    {
      unlocated_decided_ = false;
    }

    return EventHandlerResult::OK;
  }

  uint8_t i = key_addr.toInt();

  /* Decide on a fresh toggle-on, or if the key shows up already held without
   * one. A second toggle-on in the same cycle is the timeline's duplicate
   * delivery and keeps the choice. Re-deciding on later toggle-ons, rather
   * than trusting decided_, also recovers from a release that never arrived. */
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
  /* Remember only the modifiers in this report that the live checks in
   * shouldSendGrave() can't see on their own. Physically held keys and
   * one-shots are always read live, so a stale copy of them can never make
   * the next press pick ` after they have been released. */
  Mods report   = reportMods();
  Mods physical = physicalMods();
  Mods one_shot = oneShotMods();

  injected_shift_ = report.shift && !physical.shift && !one_shot.shift;
  injected_gui_   = report.gui   && !physical.gui   && !one_shot.gui;

  memset(seen_this_cycle_, 0, sizeof(seen_this_cycle_));

  return EventHandlerResult::OK;
}

}
}

kaleidoscope::plugin::GraveEscapeDygma GraveEscape;
