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

#include <Arduino.h>
#include <math.h>

#include "MouseKeysDygma.h"
#include "kbdfal_ll_memory.h"
#include "hidDefy.h"

#include "kaleidoscope/Runtime.h"
#include "Kaleidoscope-FocusSerial.h"
#include "kaleidoscope/keyswitch_state.h"

#define LAYOUT_VERSION                  2
#define LAYOUT_VERSION_SPEED_UNIT_10    1

#define DEFAULT_SPEED_SLOW              8       /* 160 counts/s */
#define DEFAULT_SPEED_MEDIUM            30      /* 600 counts/s */
#define DEFAULT_SPEED_FAST              90      /* 1800 counts/s */
#define DEFAULT_WHEEL_SPEED_SLOW        3
#define DEFAULT_WHEEL_SPEED_MEDIUM      10
#define DEFAULT_WHEEL_SPEED_FAST        30
#define DEFAULT_RAMP_TIME               80
#define DEFAULT_BOOST_DISTANCE          400
#define DEFAULT_BOOST_TIME              25      /* 100 ms */

#define SPEED_UNIT                      20      /* counts/s of one stored speed unit, up to 5100 counts/s */
#define BOOST_TIME_UNIT                 4       /* ms of one stored boost time unit */

/* Longest cycle integrated at once, so a stalled loop does not throw the cursor across the screen */
#define MAX_CYCLE_MS                    50
/* Minimum time between two cursor reports */
#define REPORT_INTERVAL_MS              8
/* Over BLE a cursor report waits for the previous one to be transmitted, so each connection event carries
 * exactly one report with all the movement since the previous event. This bounds the wait in case a
 * transmission is never confirmed. */
#define REPORT_WAIT_MAX_MS              100

#define REPORT_MOVE_MAX                 127
#define SQRT1_2                         0.70710678f

#define AXIS_X                          0
#define AXIS_Y                          1
#define WHEEL_V                         0
#define WHEEL_H                         1

namespace kaleidoscope {
namespace plugin {

const MouseKeys_::MouseKeys_config_t * MouseKeys_::p_MouseKeys_config = nullptr;

uint8_t MouseKeys_::mouseMoveIntent;
uint8_t MouseKeys_::wheelIntent;
uint8_t MouseKeys_::speedIntent;
bool MouseKeys_::boostPending;
bool MouseKeys_::active;

uint32_t MouseKeys_::lastCycleMs;
uint32_t MouseKeys_::lastReportMs;

float MouseKeys_::velocity[2];
float MouseKeys_::moveRemainder[2];
float MouseKeys_::boostVelocity[2];
float MouseKeys_::boostElapsed;
float MouseKeys_::boostDuration;
float MouseKeys_::wheelRemainder[2];
int8_t MouseKeys_::wheelPending[2];

/* Moves the velocity towards the target. Slowing down, stopping and reversing are instant, so the
 * cursor stops where the key is released; speeding up takes the ramp time. */
static float velocityApproach(float velocity, float target, float dt, uint8_t rampTime) {
  if (target == 0 || velocity * target < 0)
    velocity = 0;

  if (fabsf(velocity) >= fabsf(target) || rampTime == 0)
    return target;

  velocity += target * dt / rampTime;

  return (fabsf(velocity) > fabsf(target)) ? target : velocity;
}

/* Takes the whole counts out of a remainder. The fraction, and whatever exceeds one report, stay
 * for the next report. */
static int8_t remainderTake(float &remainder) {
  float whole = truncf(remainder);

  if (whole > REPORT_MOVE_MAX)
    whole = REPORT_MOVE_MAX;
  else if (whole < -REPORT_MOVE_MAX)
    whole = -REPORT_MOVE_MAX;

  remainder -= whole;

  return (int8_t)whole;
}

/* Converts a Focus value into stored units, rounding to the nearest unit. */
static uint8_t unitsFromValue(uint16_t value, uint8_t unit, uint8_t min) {
  uint32_t units = ((uint32_t)value + unit / 2) / unit;

  if (units < min)
    units = min;
  else if (units > UINT8_MAX)
    units = UINT8_MAX;

  return (uint8_t)units;
}

void MouseKeys_::setWarpGridSize(uint8_t grid_size) {
  MouseWrapper.warp_grid_size = grid_size;
}

MouseKeys_::speed_mode_t MouseKeys_::speedMode() {
  /* The slowest speed key held wins */
  if (speedIntent & KEY_MOUSE_SPEED_SLOW)
    return SPEED_MODE_SLOW;
  if (speedIntent & KEY_MOUSE_SPEED_MEDIUM)
    return SPEED_MODE_MEDIUM;
  if (speedIntent & KEY_MOUSE_SPEED_FAST)
    return SPEED_MODE_FAST;

  return SPEED_MODE_MEDIUM;
}

/* A boost glides with a velocity that decays linearly to zero over the boost time,
 * v(t) = v0 * (1 - t / T), so it covers v0 * T / 2. */
void MouseKeys_::boostFire(int8_t dirX, int8_t dirY) {
  const int8_t dir[2] = { dirX, dirY };
  float distance = p_MouseKeys_config->boostDistance;
  float left[2] = { 0, 0 };

  if (dirX != 0 && dirY != 0)
    distance *= SQRT1_2;

  /* A boost fired while another one glides takes over the distance the previous one had left */
  if (boostElapsed < boostDuration) {
    float time_left = boostDuration - boostElapsed;

    for (uint8_t axis = AXIS_X; axis <= AXIS_Y; axis++)
      left[axis] = boostVelocity[axis] * time_left * time_left / (2 * boostDuration);
  }

  boostDuration = p_MouseKeys_config->boostTime * BOOST_TIME_UNIT;
  boostElapsed = 0;

  for (uint8_t axis = AXIS_X; axis <= AXIS_Y; axis++) {
    float total = left[axis] + dir[axis] * distance;

    if (boostDuration == 0) {
      moveRemainder[axis] += total;
      boostVelocity[axis] = 0;
    } else {
      boostVelocity[axis] = 2 * total / boostDuration;
    }
  }
}

void MouseKeys_::boostAdvance(float dt) {
  if (boostElapsed >= boostDuration)
    return;

  float t1 = boostElapsed;
  float t2 = (t1 + dt < boostDuration) ? t1 + dt : boostDuration;
  /* Integral of (1 - t / T) between t1 and t2 */
  float covered = (t2 - t1) - (t2 * t2 - t1 * t1) / (2 * boostDuration);

  for (uint8_t axis = AXIS_X; axis <= AXIS_Y; axis++)
    moveRemainder[axis] += boostVelocity[axis] * covered;

  boostElapsed = t2;
}

/* The press of a wheel key scrolls one notch right away, so a tap always scrolls. The notch is
 * also kept for the cycle report, which overwrites the whole mouse report. */
void MouseKeys_::wheelNotch(uint8_t keyCode) {
  if (keyCode & KEY_MOUSE_UP)
    wheelPending[WHEEL_V] = 1;
  else if (keyCode & KEY_MOUSE_DOWN)
    wheelPending[WHEEL_V] = -1;
  else if (keyCode & KEY_MOUSE_LEFT)
    wheelPending[WHEEL_H] = -1;
  else if (keyCode & KEY_MOUSE_RIGHT)
    wheelPending[WHEEL_H] = 1;

  wheelRemainder[WHEEL_V] = 0;
  wheelRemainder[WHEEL_H] = 0;

  kaleidoscope::Runtime.hid().mouse().move(0, 0, wheelPending[WHEEL_V], wheelPending[WHEEL_H]);
}

EventHandlerResult MouseKeys_::afterEachCycle() {
  kaleidoscope::Runtime.hid().mouse().sendReport();
  kaleidoscope::Runtime.hid().mouse().releaseAllButtons();
  mouseMoveIntent = 0;
  wheelIntent = 0;
  speedIntent = 0;
  wheelPending[WHEEL_V] = 0;
  wheelPending[WHEEL_H] = 0;

  return EventHandlerResult::OK;
}

EventHandlerResult MouseKeys_::beforeReportingState() {
  uint32_t now = kbdtimer_get_system_ms();
  float dt = 0;

  /* The time since an idle cycle is not movement, so the timing starts again from here */
  if (active) {
    uint32_t elapsed = now - lastCycleMs;
    dt = (elapsed > MAX_CYCLE_MS) ? MAX_CYCLE_MS : elapsed;
  }
  lastCycleMs = now;

  int8_t dirX = ((mouseMoveIntent & KEY_MOUSE_RIGHT) ? 1 : 0) - ((mouseMoveIntent & KEY_MOUSE_LEFT) ? 1 : 0);
  int8_t dirY = ((mouseMoveIntent & KEY_MOUSE_DOWN) ? 1 : 0) - ((mouseMoveIntent & KEY_MOUSE_UP) ? 1 : 0);

  /* A boost pressed before the arrow waits for it while the boost key is held */
  if (boostPending && (dirX != 0 || dirY != 0)) {
    boostFire(dirX, dirY);
    boostPending = false;
  }

  speed_mode_t mode = speedMode();

  /* Cursor, in counts/ms */
  float speed = p_MouseKeys_config->speed[mode] * SPEED_UNIT / 1000.0f;

  if (dirX != 0 && dirY != 0)
    speed *= SQRT1_2;

  const float target[2] = { dirX * speed, dirY * speed };

  for (uint8_t axis = AXIS_X; axis <= AXIS_Y; axis++) {
    velocity[axis] = velocityApproach(velocity[axis], target[axis], dt, p_MouseKeys_config->rampTime);
    moveRemainder[axis] += velocity[axis] * dt;
  }

  boostAdvance(dt);

  /* Wheel, in notches/ms */
  float wheelRate = p_MouseKeys_config->wheelSpeed[mode] / 1000.0f;
  const int8_t wheelDir[2] = {
    (int8_t)(((wheelIntent & KEY_MOUSE_UP) ? 1 : 0) - ((wheelIntent & KEY_MOUSE_DOWN) ? 1 : 0)),
    (int8_t)(((wheelIntent & KEY_MOUSE_RIGHT) ? 1 : 0) - ((wheelIntent & KEY_MOUSE_LEFT) ? 1 : 0)),
  };

  for (uint8_t axis = WHEEL_V; axis <= WHEEL_H; axis++) {
    if (wheelDir[axis] == 0)
      wheelRemainder[axis] = 0;
    else
      wheelRemainder[axis] += wheelDir[axis] * wheelRate * dt;
  }

  int8_t vWheel = wheelPending[WHEEL_V] + remainderTake(wheelRemainder[WHEEL_V]);
  int8_t hWheel = wheelPending[WHEEL_H] + remainderTake(wheelRemainder[WHEEL_H]);

  int8_t moveX = 0, moveY = 0;
  bool movePending = fabsf(moveRemainder[AXIS_X]) >= 1 || fabsf(moveRemainder[AXIS_Y]) >= 1;
  uint32_t sinceReport = now - lastReportMs;

  if (movePending && sinceReport >= REPORT_INTERVAL_MS &&
      (HID().mouseReportReady() || sinceReport >= REPORT_WAIT_MAX_MS)) {
    moveX = remainderTake(moveRemainder[AXIS_X]);
    moveY = remainderTake(moveRemainder[AXIS_Y]);
    lastReportMs = now;

    MouseWrapper.end_warping();
  }

  if (moveX != 0 || moveY != 0 || vWheel != 0 || hWheel != 0)
    kaleidoscope::Runtime.hid().mouse().move(moveX, moveY, vWheel, hWheel);

  /* The loop has to keep cycling, without the MCU sleeping in between, while anything moves */
  active = mouseMoveIntent != 0 || wheelIntent != 0 ||
           boostElapsed < boostDuration ||
           fabsf(moveRemainder[AXIS_X]) >= 1 || fabsf(moveRemainder[AXIS_Y]) >= 1;

  if (active)
    kbdpwr_sleep_postpone();

  return EventHandlerResult::OK;
}

EventHandlerResult MouseKeys_::onKeyswitchEvent(Key &mappedKey, KeyAddr key_addr, uint8_t keyState) {
  if (mappedKey.getFlags() != (SYNTHETIC | IS_MOUSE_KEY))
    return EventHandlerResult::OK;

  uint8_t keyCode = mappedKey.getKeyCode();

  if (keyCode & KEY_MOUSE_DYGMA) {
    if (keyCode & KEY_MOUSE_BOOST) {
      /* One boost per press: it fires with the arrows held, or with the first arrow pressed
       * while the boost key is still held */
      if (keyToggledOn(keyState)) {
        boostPending = true;
        kbdpwr_sleep_postpone();
      } else if (keyToggledOff(keyState)) {
        boostPending = false;
      }
    } else if (keyIsPressed(keyState)) {
      speedIntent |= keyCode & ~KEY_MOUSE_DYGMA;
    }
  } else if (keyCode & KEY_MOUSE_BUTTON && !(keyCode & KEY_MOUSE_WARP)) {
    uint8_t button = keyCode & ~KEY_MOUSE_BUTTON;

    if (keyIsPressed(keyState)) {
      // Reset warp state on initial mouse button key-down only so we can use
      // warp keys to drag-and-drop:
      if (keyToggledOn(keyState)) {
        MouseWrapper.reset_warping();
      }

      kaleidoscope::Runtime.hid().mouse().pressButtons(button);
    } else if (keyToggledOff(keyState)) {
      kaleidoscope::Runtime.hid().mouse().releaseButtons(button);
      MouseWrapper.end_warping();
    }
  } else if (!(keyCode & KEY_MOUSE_WARP)) {
    if (keyToggledOn(keyState)) {
      kbdpwr_sleep_postpone();

      if (keyCode & KEY_MOUSE_WHEEL)
        wheelNotch(keyCode);
    }

    if (keyIsPressed(keyState)) {
      if (keyCode & KEY_MOUSE_WHEEL) {
        wheelIntent |= keyCode & ~KEY_MOUSE_WHEEL;
      } else {
        mouseMoveIntent |= keyCode;
      }
    } else if (keyToggledOff(keyState)) {
      /* Macros send several reports per cycle, so the notch of a released wheel key is
       * explicitly removed from the report. */
      if (keyCode & KEY_MOUSE_WHEEL) {
        bool vWheel = keyCode & (KEY_MOUSE_UP | KEY_MOUSE_DOWN);

        wheelPending[vWheel ? WHEEL_V : WHEEL_H] = 0;
        kaleidoscope::Runtime.hid().mouse().stop(false, false, vWheel, !vWheel);
      } else {
        mouseMoveIntent &= ~keyCode;
      }
    }
  } else if (keyToggledOn(keyState)) {
    if (keyCode & KEY_MOUSE_WARP && mappedKey.getFlags() & IS_MOUSE_KEY) {
      MouseWrapper.warp(((keyCode & KEY_MOUSE_WARP_END) ? WARP_END : 0x00) |
                        ((keyCode & KEY_MOUSE_UP) ? WARP_UP : 0x00) |
                        ((keyCode & KEY_MOUSE_DOWN) ? WARP_DOWN : 0x00) |
                        ((keyCode & KEY_MOUSE_LEFT) ? WARP_LEFT : 0x00) |
                        ((keyCode & KEY_MOUSE_RIGHT) ? WARP_RIGHT : 0x00));
    }
  }

  return EventHandlerResult::EVENT_CONSUMED;
}

MouseKeys_::speed_mode_t MouseKeys_::focusSpeedModeParse(const char *mode)
{
  if (strcmp_P(mode, PSTR("Slow")) == 0)
    return SPEED_MODE_SLOW;
  if (strcmp_P(mode, PSTR("Medium")) == 0)
    return SPEED_MODE_MEDIUM;
  if (strcmp_P(mode, PSTR("Fast")) == 0)
    return SPEED_MODE_FAST;

  return SPEED_MODE_COUNT;
}

EventHandlerResult MouseKeys_::onFocusEvent(const char *command)
{
  if (::Focus.handleHelp(command, PSTR("mouse.speedSlow\nmouse.speedMedium\nmouse.speedFast\nmouse.wheelSlow\nmouse.wheelMedium\nmouse.wheelFast\nmouse.ramp\nmouse.boostDistance\nmouse.boostTime")))
    return EventHandlerResult::OK;

  if (strncmp_P(command, PSTR("mouse."), 6) != 0)
    return EventHandlerResult::OK;

  const char * p_setting = command + 6;
  speed_mode_t mode;

  /* Cursor speed, in counts/s */
  if (strncmp_P(p_setting, PSTR("speed"), 5) == 0 && ( mode = focusSpeedModeParse(p_setting + 5) ) != SPEED_MODE_COUNT)
  {
    if (::Focus.isEOL())
    {
      ::Focus.send((uint16_t)(p_MouseKeys_config->speed[mode] * SPEED_UNIT));
    }
    else
    {
      uint16_t speed = 0;
      ::Focus.read(speed);

      mem_speed_save( mode, unitsFromValue( speed, SPEED_UNIT, 1 ) );
    }
  }

  /* Wheel speed, in notches/s */
  if (strncmp_P(p_setting, PSTR("wheel"), 5) == 0 && ( mode = focusSpeedModeParse(p_setting + 5) ) != SPEED_MODE_COUNT)
  {
    if (::Focus.isEOL())
    {
      ::Focus.send(p_MouseKeys_config->wheelSpeed[mode]);
    }
    else
    {
      uint16_t wheelSpeed = 0;
      ::Focus.read(wheelSpeed);

      mem_wheel_speed_save( mode, unitsFromValue( wheelSpeed, 1, 1 ) );
    }
  }

  /* Ramp time, in ms */
  if (strcmp_P(p_setting, PSTR("ramp")) == 0)
  {
    if (::Focus.isEOL())
    {
      ::Focus.send(p_MouseKeys_config->rampTime);
    }
    else
    {
      uint16_t rampTime = 0;
      ::Focus.read(rampTime);

      mem_ramp_time_save( unitsFromValue( rampTime, 1, 0 ) );
    }
  }

  /* Boost distance, in counts */
  if (strcmp_P(p_setting, PSTR("boostDistance")) == 0)
  {
    if (::Focus.isEOL())
    {
      ::Focus.send(p_MouseKeys_config->boostDistance);
    }
    else
    {
      uint16_t boostDistance = 0;
      ::Focus.read(boostDistance);

      mem_boost_distance_save( boostDistance );
    }
  }

  /* Boost time, in ms */
  if (strcmp_P(p_setting, PSTR("boostTime")) == 0)
  {
    if (::Focus.isEOL())
    {
      ::Focus.send((uint16_t)(p_MouseKeys_config->boostTime * BOOST_TIME_UNIT));
    }
    else
    {
      uint16_t boostTime = 0;
      ::Focus.read(boostTime);

      mem_boost_time_save( unitsFromValue( boostTime, BOOST_TIME_UNIT, 0 ) );
    }
  }

  return EventHandlerResult::EVENT_CONSUMED;
}

EventHandlerResult MouseKeys_::onSetup(void) {
  result_t result = RESULT_ERR;

  static_assert( sizeof( MouseKeys_config_t ) == 12, "MouseKeys_config_t must keep the 12-byte flash slot of the previous layout" );

  kaleidoscope::Runtime.hid().mouse().setup();
  kaleidoscope::Runtime.hid().absoluteMouse().setup();

  result = kbdfal_ll_memory_item_request( (const void **)&p_MouseKeys_config, sizeof( MouseKeys_config_t ) );
  ASSERT_DYGMA( result == RESULT_OK, "kbdfal_ll_memory_item_request failed" );

  /* Layout 1 stored the speeds in units of 10 counts/s: only the unit changed, so the settings are kept */
  if( p_MouseKeys_config->layoutVersion == LAYOUT_VERSION_SPEED_UNIT_10 )
  {
    MouseKeys_config_t config = *p_MouseKeys_config;

    for( uint8_t mode = 0; mode < SPEED_MODE_COUNT; mode++ )
    {
      config.speed[mode] = ( config.speed[mode] + 1 ) / 2;
    }
    config.layoutVersion = LAYOUT_VERSION;

    mem_config_save( &config );
  }

  /* Erased flash and the Kaleidoscope MouseKeys layout both read the version as 0xFF */
  if( p_MouseKeys_config->layoutVersion != LAYOUT_VERSION )
  {
    MouseKeys_config_t config;

    config.speed[SPEED_MODE_SLOW] = DEFAULT_SPEED_SLOW;
    config.speed[SPEED_MODE_MEDIUM] = DEFAULT_SPEED_MEDIUM;
    config.speed[SPEED_MODE_FAST] = DEFAULT_SPEED_FAST;
    config.wheelSpeed[SPEED_MODE_SLOW] = DEFAULT_WHEEL_SPEED_SLOW;
    config.wheelSpeed[SPEED_MODE_MEDIUM] = DEFAULT_WHEEL_SPEED_MEDIUM;
    config.wheelSpeed[SPEED_MODE_FAST] = DEFAULT_WHEEL_SPEED_FAST;
    config.rampTime = DEFAULT_RAMP_TIME;
    config.boostDistance = DEFAULT_BOOST_DISTANCE;
    config.boostTime = DEFAULT_BOOST_TIME;
    config.reserved = 0xFF;
    config.layoutVersion = LAYOUT_VERSION;

    mem_config_save( &config );
  }

  return EventHandlerResult::OK;

  UNUSED( result );
}

void MouseKeys_::mem_config_save( const MouseKeys_config_t * p_config )
{
    result_t result = RESULT_ERR;

    result = kbdfal_ll_memory_data_save( p_MouseKeys_config, p_config, sizeof( MouseKeys_config_t ) );
    ASSERT_DYGMA( result == RESULT_OK, "kbdfal_ll_memory_save failed" );

    UNUSED( result );
}

void MouseKeys_::mem_speed_save( speed_mode_t mode, uint8_t speed )
{
    result_t result = RESULT_ERR;

    result = kbdfal_ll_memory_data_save( &p_MouseKeys_config->speed[mode], &speed, sizeof(p_MouseKeys_config->speed[mode]) );
    ASSERT_DYGMA( result == RESULT_OK, "kbdfal_ll_memory_save failed" );

    UNUSED( result );
}

void MouseKeys_::mem_wheel_speed_save( speed_mode_t mode, uint8_t wheelSpeed )
{
    result_t result = RESULT_ERR;

    result = kbdfal_ll_memory_data_save( &p_MouseKeys_config->wheelSpeed[mode], &wheelSpeed, sizeof(p_MouseKeys_config->wheelSpeed[mode]) );
    ASSERT_DYGMA( result == RESULT_OK, "kbdfal_ll_memory_save failed" );

    UNUSED( result );
}

void MouseKeys_::mem_ramp_time_save( uint8_t rampTime )
{
    result_t result = RESULT_ERR;

    result = kbdfal_ll_memory_data_save( &p_MouseKeys_config->rampTime, &rampTime, sizeof(p_MouseKeys_config->rampTime) );
    ASSERT_DYGMA( result == RESULT_OK, "kbdfal_ll_memory_save failed" );

    UNUSED( result );
}

void MouseKeys_::mem_boost_distance_save( uint16_t boostDistance )
{
    result_t result = RESULT_ERR;

    result = kbdfal_ll_memory_data_save( &p_MouseKeys_config->boostDistance, &boostDistance, sizeof(p_MouseKeys_config->boostDistance) );
    ASSERT_DYGMA( result == RESULT_OK, "kbdfal_ll_memory_save failed" );

    UNUSED( result );
}

void MouseKeys_::mem_boost_time_save( uint8_t boostTime )
{
    result_t result = RESULT_ERR;

    result = kbdfal_ll_memory_data_save( &p_MouseKeys_config->boostTime, &boostTime, sizeof(p_MouseKeys_config->boostTime) );
    ASSERT_DYGMA( result == RESULT_OK, "kbdfal_ll_memory_save failed" );

    UNUSED( result );
}

}
}

kaleidoscope::plugin::MouseKeys_ MouseKeys;
