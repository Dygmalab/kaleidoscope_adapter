#pragma once
// Minimal stand-ins for the Kaleidoscope pieces GraveEscapeDygma touches.
#include <stdint.h>
#include <set>
#include <map>
#define SYNTHETIC 0x40
#define RESERVED  0x80
struct Key {
  uint16_t raw;
  constexpr Key(uint16_t r = 0) : raw(r) {}
  uint16_t getRaw() const { return raw; }
  uint8_t getFlags() const { return raw >> 8; }
  bool operator==(const Key &o) const { return raw == o.raw; }
  bool operator!=(const Key &o) const { return raw != o.raw; }
  bool operator<(const Key &o) const { return raw < o.raw; }
  bool operator>(const Key &o) const { return raw > o.raw; }
};
#define Key_Escape       Key(0x29)
#define Key_Backtick     Key(0x35)
#define Key_A            Key(0x04)
#define Key_LeftControl  Key(0xE0)
#define Key_LeftShift    Key(0xE1)
#define Key_LeftAlt      Key(0xE2)
#define Key_LeftGui      Key(0xE3)
#define Key_RightControl Key(0xE4)
#define Key_RightShift   Key(0xE5)
#define Key_RightAlt     Key(0xE6)
#define Key_RightGui     Key(0xE7)
struct KeyAddr {
  static constexpr uint8_t upper_limit = 80;
  uint8_t o;
  constexpr KeyAddr(uint8_t x) : o(x) {}
  constexpr uint8_t toInt() const { return o; }
  constexpr bool isValid() const { return o < upper_limit; }
  struct It { uint8_t i; KeyAddr operator*() const { return KeyAddr(i); } It &operator++() { ++i; return *this; } bool operator!=(const It &b) const { return i != b.i; } };
  struct Range { It begin() const { return {0}; } It end() const { return {upper_limit}; } };
  static Range all() { return {}; }
};
namespace kaleidoscope {
enum class EventHandlerResult { OK, EVENT_CONSUMED };
struct Plugin {};
struct FakeKbd { std::set<uint16_t> mods; bool isModifierKeyActive(Key k) { return mods.count(k.raw); } };
struct FakeHid { FakeKbd kb; FakeKbd &keyboard() { return kb; } };
struct FakeDevice { std::set<uint8_t> pressed; bool isKeyswitchPressed(KeyAddr a) { return pressed.count(a.o); } };
struct FakeRuntime { FakeHid h; FakeDevice d; FakeHid &hid() { return h; } FakeDevice &device() { return d; } };
extern FakeRuntime Runtime;
}
