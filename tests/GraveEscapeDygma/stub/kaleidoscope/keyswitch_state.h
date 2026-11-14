#pragma once
#include <stdint.h>
#define INJECTED   0b10000000
#define IS_PRESSED 0b00000010
#define WAS_PRESSED 0b00000001
#define keyIsPressed(s) ((s) & IS_PRESSED)
#define keyWasPressed(s) ((s) & WAS_PRESSED)
#define keyToggledOn(s) (keyIsPressed(s) && !keyWasPressed(s))
#define keyToggledOff(s) (keyWasPressed(s) && !keyIsPressed(s))
