/* -*- mode: c++ -*-
 * Kaleidoscope-EEPROM-Keymap -- EEPROM-based keymap support.
 * Copyright (C) 2017, 2018, 2019  Keyboard.io, Inc
 * Copyright (C) 2025 Dygma Lab S.L. www.dygma.com
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

namespace kaleidoscope {
namespace plugin {
class EEPROMKeymap : public kaleidoscope::Plugin {

 public:

  typedef struct PACK
  {
      uint8_t flags;
      uint8_t keyCode;
  } key_config_t;

  typedef struct PACK
  {
      bool_t ignore_hardcoded_layers;
      key_config_t keys[ APP_LAYERS_CNT * Runtime.device().numKeys() ];
  } keymap_config_t;

 public:
  enum class Mode {
    CUSTOM,
    EXTEND
  };

  EEPROMKeymap(void) {}

  EventHandlerResult onSetup();
  EventHandlerResult onFocusEvent(const char *command);

  static const EEPROMKeymap::keymap_config_t * getKeymapConfig(void);

  static Key getKey(uint8_t layer, KeyAddr key_addr);
  static Key getKeyExtended(uint8_t layer, KeyAddr key_addr);

  static void updateKey(uint16_t base_pos, Key key);

 private:
  static const keymap_config_t * p_keymap_config;

  static uint8_t max_layers_;
  static uint8_t progmem_layers_;

  static Key parseKey(void);
  static void printKey(Key key);
  static void dumpKeymap(uint8_t layers, Key(*getkey)(uint8_t, KeyAddr));

  static void cfgmem_ignore_hardcoded_layers_save( bool_t ignore_hardcoded_layers );
  static void cfgmem_key_save( const key_config_t * p_key_config, key_config_t * p_key );
};
}
}

extern kaleidoscope::plugin::EEPROMKeymap EEPROMKeymap;
