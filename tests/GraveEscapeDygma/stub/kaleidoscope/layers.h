#pragma once
#include "kaleidoscope/Runtime.h"
struct FakeLayer { std::map<uint8_t, Key> keymap; Key lookup(KeyAddr a) { auto it = keymap.find(a.o); return it == keymap.end() ? Key(0) : it->second; } };
extern FakeLayer Layer;
