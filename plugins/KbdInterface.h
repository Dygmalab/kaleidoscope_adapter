/*
 * kbdfal_ll_kbdif.h - kbdfal_ll_kbdif module connects the kaleidoscope event
 *                      handlers to the keyboard API high-level interface kbdif.
 *                      Must make sure all functions needed for keyboard_api kbdif
 *                      interface are declared here as well as the internal
 *                      functions used only in the kaleidoscope_adapter space.
 *
 * Copyright (C) 2025 Dygma Lab S.L. www.dygma.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "kbd_core.h"
#include "kbd_if.h"
#include "kaleidoscope/plugin.h"
#include "Kaleidoscope-Ranges.h"

namespace kaleidoscope {
namespace plugin {

class KbdInterface : public Plugin {

   public:

    /* Kaleidoscope plugin functions */
    EventHandlerResult onKeyswitchEvent(Key &mapped_Key, KeyAddr key_addr, uint8_t key_state);
    EventHandlerResult onFocusEvent(const char * p_command);

    /* The Interface functions */
    result_t kbdifRegister( kbdif_t * p_kbdif );

   private:
    kbdif_t * p_kbdif = NULL;
};

}  // namespace plugin
}  // namespace kaleidoscope

extern kaleidoscope::plugin::KbdInterface KbdInterface;
