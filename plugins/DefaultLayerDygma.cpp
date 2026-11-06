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

#include "DefaultLayerDygma.h"
#include "kbdfal_ll_memory.h"

#include <Kaleidoscope-FocusSerial.h>
#include "kaleidoscope/layers.h"

namespace kaleidoscope {
namespace plugin {

EventHandlerResult DefaultLayerDygma::beforeEachCycle() {
  if( boot_layer_applied == true )
  {
    return EventHandlerResult::OK;
  }

  boot_layer_applied = true;

  /* This waits for the first cycle because Layer.setup() runs after the onSetup()
   * hooks and turns layer 0 on, and because the interfaces that follow the layer
   * changes (LEDs, overlay) are only registered once the setup has finished. */
  if( p_config != nullptr && p_config->layer < NO_DEFAULT_LAYER )
  {
    Layer.move( p_config->layer );
  }

  return EventHandlerResult::OK;
}

EventHandlerResult DefaultLayerDygma::onFocusEvent(const char *command) {
  if (::Focus.handleHelp(command, PSTR("settings.defaultLayer")))
    return EventHandlerResult::OK;

  if (strcmp_P(command, PSTR("settings.defaultLayer")) != 0)
    return EventHandlerResult::OK;

  if( p_config == nullptr )
  {
    return EventHandlerResult::OK;
  }

  if (::Focus.isEOL()) {
    ::Focus.send(p_config->layer);
    return EventHandlerResult::EVENT_CONSUMED;
  }

  uint8_t layer;

  ::Focus.read(layer);

  /* A layer that does not exist is ignored, and the values reserved for "no
   * default" are all stored as NO_DEFAULT_LAYER. */
  if (layer < layer_count) {
    Layer.move(layer);
    cfgmem_layer_save(layer);
  } else if (layer >= NO_DEFAULT_LAYER) {
    cfgmem_layer_save(NO_DEFAULT_LAYER);
  }

  return EventHandlerResult::EVENT_CONSUMED;
}

void DefaultLayerDygma::reserve_storage( void )
{
  result_t result = RESULT_ERR;

  result = kbdfal_ll_memory_item_request( KBDMEM_ITEM_TYPE_DEFAULTLAYER, (const void **)&p_config );
  ASSERT_DYGMA( result == RESULT_OK, "kbdfal_ll_memory_item_request failed" );

  /* Sanitize against the 0xFF pattern of erased flash: units upgrading from a
   * firmware without this block read the new tail as all-ones. They start on the
   * first layer, which is what they were already doing. */
  if( p_config->layer > NO_DEFAULT_LAYER )
  {
    cfgmem_layer_save( 0 );
  }

  UNUSED( result );
}

/****************************************************/
/*                   Config Memory                  */
/****************************************************/

void DefaultLayerDygma::cfgmem_layer_save( uint8_t layer )
{
  result_t result = RESULT_ERR;

  result = kbdfal_ll_memory_data_save( &p_config->layer, &layer, sizeof( p_config->layer ) );
  ASSERT_DYGMA( result == RESULT_OK, "kbdfal_ll_memory_data_save failed" );

  UNUSED( result );
}

}
}

kaleidoscope::plugin::DefaultLayerDygma DefaultLayer;
