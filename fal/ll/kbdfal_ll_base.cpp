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
#include "KbdInterface.h"

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

#ifndef KALEIDOSCOPE_ADAPTER_KEYMAP_DEFAULT
#error "Default Kaleidoscope Adapter Keymap is not specified"
#endif /* KALEIDOSCOPE_ADAPTER_KEYMAP_DEFAULT */

KEYMAPS
(
    KALEIDOSCOPE_ADAPTER_KEYMAP_DEFAULT
);

// kaleidoscope::plugin::EEPROMPadding JointPadding(8);

KALEIDOSCOPE_INIT_PLUGINS
(
    EEPROMSettings,
    EEPROMKeymap, FocusSettingsCommand, FocusEEPROMCommand, DynamicSuperKeys,
    LEDControl, FocusLEDCommand,
    LEDPaletteThemeDefy, ColormapEffectDefy,
    IdleLEDsDefy,PersistentIdleDefyLEDs, KeyboardFocus, Qukeys, DynamicMacros,
    /*SideFlash,*/ Focus, MouseKeys, OneShot, LayerFocus,
    HostPowerManagement,
    /*Keyboard API interface*/
    KbdInterface
);

result_t kbdfal_ll_base_init( void )
{
    Kaleidoscope.setup();

    return RESULT_OK;
}

result_t kbdfal_ll_base_kbdif_set( kbdif_t * p_kbdif )
{
    result_t result = RESULT_ERR;

    result = KbdInterface.kbdifRegister( p_kbdif );
    EXIT_IF_ERR( result, "KbdInterface.kbdifRegister failed" );

_EXIT:
    return result;
}
