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

#ifndef KALEIDOSCOPE_ADAPTER_KEYMAP_DEFAULT
#error "Default Kaleidoscope Adapter Keymap is not specified"
#endif /* KALEIDOSCOPE_ADAPTER_KEYMAP_DEFAULT */

KEYMAPS
(
    KALEIDOSCOPE_ADAPTER_KEYMAP_DEFAULT
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
