/* IdleLEDsDygma - Turn off the LEDs when the keyboard's idle. Temporary plugin for
 *                 easing the LED control transfer.
 * Copyright (C) 2018, 2019  Keyboard.io, Inc
 * Copyright (C) 2019  Dygma, Inc
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

#ifdef ARDUINO_ARCH_NRF52

#include "kaleidoscope/Runtime.h"


namespace kaleidoscope {
namespace plugin {

class IdleLEDsDygma : public kaleidoscope::Plugin
{
    public:
        IdleLEDsDygma(void) {}

        struct IdleTime
        {
            bool activate_keybsides_sleep;      // Activate/Deactivate put to sleep the keyboard sides [bool].
            uint32_t sides_sleep_idle_t_ms;     // Timeout to put to sleep the keyboard sides [ms].
            uint32_t leds_off_usb_idle_t_ms;    // Power off time for LEDs, when the n2 is in USB mode [ms].
            uint32_t leds_off_ble_idle_t_ms;    // Power off time for LEDs, when the n2 is in BLE mode [ms].
        };

        static IdleTime Power_save;
        static constexpr const uint32_t leds_off_usb_idle_t_ms_default = 600000;  // 600.000 ms = 10 minutes
        static constexpr const uint32_t leds_off_ble_idle_t_ms_default = 300000;  // 300.000 ms = 5 minutes
        static constexpr const uint32_t sides_sleep_idle_t_ms_default = 60000;    // 60.000 ms = 1 minutes
        
        static void save_power_save_settings(const IdleTime& data);
//        static uint32_t ms_to_seconds(uint32_t time_in_ms);

//        void reset_timers();
//        void new_connection_set();

//        EventHandlerResult beforeEachCycle();
//        EventHandlerResult onKeyswitchEvent(Key &mapped_key, KeyAddr key_addr, uint8_t key_state);

    private:
//        static bool idle_;

    protected:
//        static uint32_t start_time_wired;
//        static uint32_t start_time_wireless;
//        static uint32_t start_time_true_sleep;
//        static uint32_t start_time_true_sleep_wired;
//        static bool sleep_;
//        static bool new_connection_;
};

class PersistentIdleDygmaLEDs : public IdleLEDsDygma
{
    public:
        EventHandlerResult onSetup();
//        EventHandlerResult onFocusEvent(const char *command);
        static void save_power_save_settings(const IdleTime& data);

        void true_sleep_save( bool true_sleep_enable );
        void true_sleep_time_ms_save( uint32_t true_sleep_time_ms );
        void leds_off_wired_time_ms_save( uint32_t leds_off_wired_time_ms );
        void leds_off_wireless_time_ms_save( uint32_t leds_off_wireless_time_ms );

        bool true_sleep_load( void );
        uint32_t true_sleep_time_ms_save( void );
        uint32_t leds_off_wired_time_ms_save( void );
        uint32_t leds_off_wireless_time_ms_save( void );

    private:
        static uint16_t settings_base_;
};

}   // plugin
}   // kaleidoscope

extern kaleidoscope::plugin::IdleLEDsDygma IdleLEDsDygma;
extern kaleidoscope::plugin::PersistentIdleDygmaLEDs PersistentIdleDygmaLEDs;

#endif
