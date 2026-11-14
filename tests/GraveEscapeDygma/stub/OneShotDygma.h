#pragma once
#include "kaleidoscope/Runtime.h"
struct FakeOneShot { std::set<uint16_t> active; bool isModifierActive(Key k) { return active.count(k.raw); } };
extern FakeOneShot OneShot;
