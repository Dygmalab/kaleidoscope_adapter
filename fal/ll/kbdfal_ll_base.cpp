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

#include "BaseDygma.h"
#include "DynamicMacrosDygma.h"
#include "EEPROMKeymapDygma.h"
#include "kbdfal_ll_base.h"
#include "KbdInterface.h"
#include "LEDControlDygma.h"
#include "MouseKeysDygma.h"
#include "OneShotDygma.h"
#include "OverlayKeyDygma.h"

// Kaleidoscope
#include "Kaleidoscope-FocusSerial.h"
#include "Kaleidoscope-KeyRoleManager.h"
#include "Kaleidoscope-LayerFocus.h"
#include "Kaleidoscope.h"

#include "kaleidoscope/device/dygma/keyboardManager/universalModules/Focus.h"
#include "kaleidoscope/Runtime.h"

// Support for host power management (suspend & wakeup)
#include "Kaleidoscope-HostPowerManagement.h"

#ifndef KALEIDOSCOPE_ADAPTER_KEYMAP_DEFAULT
#error "Default Kaleidoscope Adapter Keymap is not specified"
#endif /* KALEIDOSCOPE_ADAPTER_KEYMAP_DEFAULT */

KEYMAPS
(
    KALEIDOSCOPE_ADAPTER_KEYMAP_DEFAULT
);

KALEIDOSCOPE_INIT_PLUGINS
(
    EEPROMKeymap, keyRoleManager,
    LEDControl, KeyboardFocus, DynamicMacros,
    /*SideFlash,*/ Focus, MouseKeys, OneShot, LayerFocus,
    HostPowerManagement,
    /*Dygma base plugin*/
    BaseDygma,
    /*Overlay key plugin — must precede KbdInterface to consume OVERLAY_KEY events first*/
    OverlayKey,
    /*Keyboard API interface*/
    KbdInterface
);

result_t kbdfal_ll_base_init( void )
{
    Kaleidoscope.setup();

    return RESULT_OK;
}

result_t kbdfal_ll_base_key_report_enable( void )
{
    return BaseDygma.key_report_enable();
}

result_t kbdfal_ll_base_key_report_disable( void )
{
    return BaseDygma.key_report_disable();
}

result_t kbdfal_ll_base_kbdif_set( kbdif_t * p_kbdif )
{
    result_t result = RESULT_ERR;

    result = KbdInterface.kbdifRegister( p_kbdif );
    EXIT_IF_ERR( result, "KbdInterface.kbdifRegister failed" );

_EXIT:
    return result;
}
