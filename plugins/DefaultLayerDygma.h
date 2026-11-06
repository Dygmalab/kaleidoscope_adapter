/* -*- mode: c++ -*-
 * DefaultLayerDygma -- Layer the keyboard switches to when it boots
 * Copyright (C) 2026 Dygma Lab S.L. www.dygma.com
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

/* Implements the `settings.defaultLayer` Focus command that Bazecor uses, which
 * used to live in the Kaleidoscope EEPROM-Settings plugin. The value is kept in
 * the ConfigManager memory instead of the Kaleidoscope storage. */
class DefaultLayerDygma : public kaleidoscope::Plugin {
 public:
  DefaultLayerDygma(void) {}

  EventHandlerResult beforeEachCycle();
  EventHandlerResult onFocusEvent(const char *command);

  /* The application must keep this block at the tail of its configuration image
   * to leave the layout of older images untouched. */
  typedef struct PACK
  {
      uint8_t layer;
      uint8_t reserved[3];

  } default_layer_config_t;

  /* Requests the configuration block. Called from the application once the
   * configuration memory is available. */
  void reserve_storage( void );

  /* The value Bazecor sends for its "No default" option: the keyboard boots on
   * the first layer, and stays where it is when the setting is changed. */
  static constexpr uint8_t NO_DEFAULT_LAYER = 126;

 private:
  const default_layer_config_t * p_config = nullptr;
  bool_t boot_layer_applied = false;

  void cfgmem_layer_save( uint8_t layer );
};

}
}

extern kaleidoscope::plugin::DefaultLayerDygma DefaultLayer;
