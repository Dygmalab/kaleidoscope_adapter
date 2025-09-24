/*
 * kbdfal_ll_kbdif.h - kbdfal_ll_kbdif module connects the kaleidoscope event handlers
 *                      to the keyboard API high-level interface kbdif.
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

#include "KbdInterface.h"

namespace kaleidoscope
{
namespace plugin
{
    static inline kbdapi_key_type_t kbdapi_key_type_get( Key &mappedKey )
    {
        switch( mappedKey.getRaw() )
        {
            case ranges::BATTERY_LEVEL:
                return KBDAPI_KEY_TYPE_BATTERY_LEVEL;
            case ranges::BLUETOOTH_PAIRING:
                return KBDAPI_KEY_TYPE_BLUETOOTH_PAIRING;
            default:
                return KBDAPI_KEY_TYPE_UNSPECIFIED;
        }
    }

    static inline EventHandlerResult EventHandlerResult_get( kbdapi_event_result_t event_result )
    {
        switch( event_result )
        {
            case KBDAPI_EVENT_RESULT_CONSUMED:
                return EventHandlerResult::EVENT_CONSUMED;

            case KBDAPI_EVENT_RESULT_IGNORED:
            default:
                return EventHandlerResult::OK;
        }
    }

    EventHandlerResult KbdInterface::onKeyswitchEvent(Key &mappedKey, KeyAddr key_addr, uint8_t keyState)
    {
        kbdapi_key_t kbdapi_key;
        kbdapi_event_result_t event_result;

        /* Populate Keyboard API key structure */
        kbdapi_key.type = kbdapi_key_type_get( mappedKey );
        kbdapi_key.col = key_addr.col();
        kbdapi_key.row = key_addr.row();

        kbdapi_key.is_pressed = keyIsPressed(keyState);
        kbdapi_key.was_pressed = keyWasPressed(keyState);
        kbdapi_key.toggled_on = keyToggledOn(keyState);
        kbdapi_key.toggled_off = keyToggledOff(keyState);

        event_result = kbdif_key_event( p_kbdif, &kbdapi_key );

        return EventHandlerResult_get( event_result );
    }

    result_t KbdInterface::kbdifRegister( kbdif_t * p_kbdif )
    {
        this->p_kbdif = p_kbdif;

        return RESULT_OK;
    }

} // namespace plugin
} //  namespace kaleidoscope

kaleidoscope::plugin::KbdInterface KbdInterface;
