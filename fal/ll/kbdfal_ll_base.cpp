/*
 * kbdfal_ll_base.cpp - This file contains sources for configuring and enabling
 *                      kaleidoscope_adapter base.
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

#include "kbdfal_ll_base.h"


// Kaleidoscope
//#include "Kaleidoscope-Colormap.h"
#include "Kaleidoscope-DynamicMacros.h"
#include "Kaleidoscope-DynamicSuperKeys.h"
#include "Kaleidoscope-EEPROM-Keymap.h"
#include "Kaleidoscope-FocusSerial.h"
#include "Kaleidoscope-IdleLEDsDefy.h"
#include "Kaleidoscope-LayerFocus.h"
//#include "Kaleidoscope-MagicCombo.h"
#include "Kaleidoscope-MouseKeys.h"
#include "Kaleidoscope.h"

#include "kaleidoscope/device/dygma/keyboardManager/universalModules/Focus.h"

// Support for host power management (suspend & wakeup)
#include "Kaleidoscope-HostPowerManagement.h"

#include "Kaleidoscope-OneShot.h"
#include "Kaleidoscope-Qukeys.h"

// #include "LED-CapsLockLight.h"

// LED effects
#include "Colormap-Defy.h"
#include "LED-Palette-Theme-Defy.h"
#include "LEDEffect-BatteryStatus-Defy.h"
#include "LEDEffect-Bluetooth-Pairing-Defy.h"
//#include "LEDEffect-Breathe-Defy.h"
#include "LEDEffect-Rainbow-Defy.h"
#include "LEDEffect-SolidColor-Defy.h"
#include "LEDEffect-Stalker-Defy.h"
// #include "DefaultColormap.h"

#include "FirmwareVersion.h"
#include "Battery.h"
#include "Ble_manager.h"
#include "Radio_manager.h"
#include "Upgrade.h"

enum
{
    QWERTY,
    NUMPAD,
    _LAYER_MAX
}; // layers

/*
    This comments temporarily turns off astyle's indent enforcement so we can make
    the keymaps actually resemble the physical key layout better
*/
KEYMAPS
(
    [QWERTY] = KEYMAP_STACKED
    (
        /* Left Side */
        Key_Escape, Key_1, Key_2, Key_3, Key_4, Key_5, Key_6,
        Key_Tab, Key_Q, Key_W, Key_E, Key_R, Key_T, Consumer_VolumeIncrement,
        Key_CapsLock, Key_A, Key_S, Key_D, Key_F, Key_G, Consumer_VolumeDecrement,
        Key_Backslash, Key_Z, Key_X, Key_C, Key_V, Key_B,
        Key_LeftControl, Key_LeftGui, Key_Backspace, Key_Delete,
        Key_LeftShift, Key_LeftAlt, Key_Enter, Key_Space,

        /* Right Side */
        Key_7, Key_8, Key_9, Key_0, Key_Minus, Key_Equals, Key_Backspace,
        Key_Y, Key_U, Key_I, Key_O, Key_P, Key_LeftBracket, Key_RightBracket,
        Key_H, Key_J, Key_K, Key_L, Key_Semicolon, Key_Quote, Key_RightShift,
        Key_N, Key_M, Key_Comma, Key_Period, Key_Slash, Key_RightShift,
        Key_LEDEffectNext, Key_Home, Key_UpArrow, Key_End,
        Key_RightArrow, Key_DownArrow, Key_LeftArrow, Key_Enter
    ),

    [NUMPAD] = KEYMAP_STACKED
    (
        /* Left Side */
        Key_Escape, Key_F1, Key_F2, Key_F3, Key_F4, Key_F5, Key_F6,
        Key_Tab, Key_NoKey, Key_UpArrow, Key_NoKey, Key_NoKey, Key_NoKey, Key_NoKey,
        Key_CapsLock, Key_LeftArrow, Key_DownArrow, Key_RightArrow, Key_NoKey, Key_NoKey, Key_NoKey,
        Key_LeftShift, Key_Backslash, Key_NoKey, Key_NoKey, Key_NoKey, Key_NoKey,
        Key_LeftControl, Key_LeftGui, Key_LeftAlt, Key_Space,
        Key_Space, Key_Backspace, Key_Enter, Key_Delete,

        /* Right Side */
        Key_F7, Key_F8, Key_F9, Key_F10, Key_F11, Key_F12, Key_Backspace,
        Key_KeypadSubtract, Key_7, Key_8, Key_9, Key_KeypadDivide, Key_NoKey, Key_Enter,
        Key_KeypadAdd, Key_4, Key_5, Key_6, Key_KeypadMultiply, Key_NoKey, Key_Backslash,
        Key_KeypadDot, Key_1, Key_2, Key_3, Key_UpArrow, Key_RightShift,
        Key_0, Key_Space, Key_LeftArrow, Key_DownArrow,
        Key_RightArrow, Key_RightControl, Key_Delete, MoveToLayer(QWERTY)
    )
);

// kaleidoscope::plugin::EEPROMPadding JointPadding(8);
static kaleidoscope::plugin::LEDSolidColorDefy solidRedDefy(255, 0, 0, 0);
static kaleidoscope::plugin::LEDSolidColorDefy solidGreenDefy(0, 255, 0, 0);
static kaleidoscope::plugin::LEDSolidColorDefy solidBlueDefy(0, 0, 255, 0);
static kaleidoscope::plugin::LEDSolidColorDefy solidWhiteDefy(0, 0, 0, 255);
static kaleidoscope::plugin::LEDSolidColorDefy solidBlackDefy(0, 0, 0, 0);
static kaleidoscope::plugin::LEDBatteryStatusDefy batteryStatus{};
static kaleidoscope::plugin::LEDStalkerDefy stalkerDefy{};

KALEIDOSCOPE_INIT_PLUGINS
(
    EEPROMSettings,
    EEPROMKeymap, FirmwareVersion, FocusSettingsCommand, FocusEEPROMCommand, /*Upgrade,*/DynamicSuperKeys,
    LEDControl, FocusLEDCommand,
    LEDPaletteThemeDefy, ColormapEffectDefy,
    LEDRainbowWaveEffectDefy, LEDRainbowEffectDefy, stalkerDefy, solidRedDefy,
    solidGreenDefy, solidBlueDefy, solidWhiteDefy, solidBlackDefy, batteryStatus,ledBluetoothPairingDefy,
    IdleLEDsDefy,PersistentIdleDefyLEDs, KeyboardFocus, Qukeys, DynamicMacros,
    /*SideFlash,*/ Focus, MouseKeys, OneShot, LayerFocus,
    HostPowerManagement,Battery,
    /*BLE*/
    RadioManager, _BleManager
);

result_t kbdfal_ll_base_init( void )
{
    Kaleidoscope.setup();

    return RESULT_OK;
}
