/* Kaleidoscope-MouseKeys - Mouse keys for Kaleidoscope.
 * Copyright (C) 2017-2018  Keyboard.io, Inc.
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
#include "kaleidoscope/plugin/MouseKeys/MouseKeyDefs.h"
#include "kaleidoscope/plugin/MouseKeys/MouseWarpModes.h"
#include "kaleidoscope/plugin/MouseKeys/MouseWrapper.h"

/* Dygma mouse keys. Bit 7 of the keycode is not used by the Kaleidoscope mouse
 * keys, so it marks the keys below. */
#define KEY_MOUSE_DYGMA          0b10000000
#define KEY_MOUSE_SPEED_SLOW     0b00000001
#define KEY_MOUSE_SPEED_MEDIUM   0b00000010
#define KEY_MOUSE_SPEED_FAST     0b00000100
#define KEY_MOUSE_BOOST          0b00001000

#define Key_mouseSpeedSlow    Key(KEY_MOUSE_DYGMA | KEY_MOUSE_SPEED_SLOW, KEY_FLAGS | SYNTHETIC | IS_MOUSE_KEY)
#define Key_mouseSpeedMedium  Key(KEY_MOUSE_DYGMA | KEY_MOUSE_SPEED_MEDIUM, KEY_FLAGS | SYNTHETIC | IS_MOUSE_KEY)
#define Key_mouseSpeedFast    Key(KEY_MOUSE_DYGMA | KEY_MOUSE_SPEED_FAST, KEY_FLAGS | SYNTHETIC | IS_MOUSE_KEY)
#define Key_mouseBoost        Key(KEY_MOUSE_DYGMA | KEY_MOUSE_BOOST, KEY_FLAGS | SYNTHETIC | IS_MOUSE_KEY)

namespace kaleidoscope {
namespace plugin {
class MouseKeys_ : public kaleidoscope::Plugin {
 public:
  MouseKeys_(void) {}

  static void setWarpGridSize(uint8_t grid_size);

  // Kaleidoscope Focus library functions
  EventHandlerResult onFocusEvent(const char *command);
  // On Setup handler function
  EventHandlerResult onSetup();

  EventHandlerResult beforeReportingState();
  EventHandlerResult afterEachCycle();
  EventHandlerResult onKeyswitchEvent(Key &mappedKey, KeyAddr key_addr, uint8_t keyState);

 private:
  typedef enum
  {
      SPEED_MODE_SLOW = 0,
      SPEED_MODE_MEDIUM,
      SPEED_MODE_FAST,

      SPEED_MODE_COUNT,
  } speed_mode_t;

  /* The configuration keeps the flash slot of the previous MouseKeys layout
   * (10 bytes, aligned to 12): the slots are allocated in request order, so a
   * bigger item would shift every configuration item requested after it. */
  typedef struct PACK
  {
      uint8_t speed[SPEED_MODE_COUNT];        /* Cursor speed, in units of 20 counts/s */
      uint8_t wheelSpeed[SPEED_MODE_COUNT];   /* Wheel speed, in notches/s */
      uint8_t rampTime;                       /* Time to reach the cursor speed, in ms */
      uint16_t boostDistance;                 /* Distance of one boost, in counts */
      uint8_t boostTime;                      /* Duration of one boost, in units of 4 ms */
      uint8_t reserved;
      uint8_t layoutVersion;                  /* Last byte: reads 0xFF on the images of the previous layout */
  } MouseKeys_config_t;

  static const MouseKeys_config_t * p_MouseKeys_config;

  static uint8_t mouseMoveIntent;
  static uint8_t wheelIntent;
  static uint8_t speedIntent;
  static bool boostPending;
  static bool active;

  static uint32_t lastCycleMs;
  static uint32_t lastReportMs;

  static float velocity[2];
  static float moveRemainder[2];
  static float boostVelocity[2];
  static float boostElapsed;
  static float boostDuration;
  static float wheelRemainder[2];
  static int8_t wheelPending[2];

  static speed_mode_t speedMode();
  static void boostFire(int8_t dirX, int8_t dirY);
  static void boostAdvance(float dt);
  static void wheelNotch(uint8_t keyCode);

  static speed_mode_t focusSpeedModeParse(const char *mode);

  static void mem_config_save( const MouseKeys_config_t * p_config );
  static void mem_speed_save( speed_mode_t mode, uint8_t speed );
  static void mem_wheel_speed_save( speed_mode_t mode, uint8_t wheelSpeed );
  static void mem_ramp_time_save( uint8_t rampTime );
  static void mem_boost_distance_save( uint16_t boostDistance );
  static void mem_boost_time_save( uint8_t boostTime );
};
}
}

extern kaleidoscope::plugin::MouseKeys_ MouseKeys;
