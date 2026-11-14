// Host-side test of GraveEscapeDygma's decision logic.
//
// Compiles the real plugin source against small stand-ins (stub/) and replays
// keyboard scans the way the Raise 2 neuron delivers them: the whole hand state
// for a cycle is known first, then keys are handled in address (row) order, each
// held modifier key adding itself to the report as it is reached. OneShot adds
// its modifiers in beforeReportingState, before Grave Escape's own hook.
//
// Build & run: tests/GraveEscapeDygma/run.sh

#include "GraveEscapeDygma.h"
#include "OneShotDygma.h"
#include "kaleidoscope/keyswitch_state.h"
#include "kaleidoscope/layers.h"

#include <cstdio>
#include <map>
#include <set>

kaleidoscope::FakeRuntime kaleidoscope::Runtime;
FakeLayer Layer;
FakeOneShot OneShot;

using namespace kaleidoscope;

static int fails = 0;
static const Key GESC(ranges::GRAVE_ESCAPE);
static const Key QUKEY_SHIFT_A(0xC104);  // stands in for a dual-function key (tap A / hold Shift)

// Layout: row = addr / 16. Grave Escape keys on the top row, modifiers lower.
enum : uint8_t { GE1 = 1, GE2 = 9, PLAIN_ESC = 2, KEY_A = 3, SHIFT = 50, GUI = 51, CTRL = 52, ALT = 53, QUKEY = 60 };

struct Sim {
  std::set<uint8_t> prev;
  bool qukey_holds_shift = false;  // the qukey at QUKEY has resolved to its Shift hold

  // One scan cycle. Returns the translated key for every Grave Escape / test key
  // that had an event this cycle.
  std::map<uint8_t, Key> cycle(std::set<uint8_t> now) {
    std::map<uint8_t, Key> out;
    Runtime.device().pressed = now;
    auto &report = Runtime.hid().keyboard().mods;
    report.clear();  // releaseAllKeys() after the previous report

    for (uint8_t a = 0; a < KeyAddr::upper_limit; a++) {
      bool was = prev.count(a), is = now.count(a);
      if (!was && !is) continue;
      uint8_t state = (is ? IS_PRESSED : 0) | (was ? WAS_PRESSED : 0);
      Key k = Layer.lookup(KeyAddr(a));
      if (k == QUKEY_SHIFT_A) {
        if (is && qukey_holds_shift) report.insert(Key_LeftShift.raw);
        continue;
      }
      if (k.raw >= 0xE0 && k.raw <= 0xE7) {
        if (is) report.insert(k.raw);
        continue;
      }
      Key m = k;
      GraveEscape.onKeyswitchEvent(m, KeyAddr(a), state);
      out[a] = m;
    }

    for (uint16_t mod : OneShot.active) report.insert(mod);  // OneShot::beforeReportingState
    GraveEscape.beforeReportingState();
    prev = now;
    return out;
  }
};

static void expect(const char *name, Key got, Key want) {
  if (got != want) {
    printf("FAIL %s: got 0x%X want 0x%X\n", name, got.raw, want.raw);
    fails++;
  } else {
    printf("ok   %s\n", name);
  }
}

int main() {
  const Key E = Key_Escape, B = Key_Backtick;
  Layer.keymap = {{GE1, GESC}, {GE2, GESC}, {PLAIN_ESC, Key_Escape}, {KEY_A, Key_A},
                  {SHIFT, Key_LeftShift}, {GUI, Key_LeftGui}, {CTRL, Key_LeftControl},
                  {ALT, Key_LeftAlt}, {QUKEY, QUKEY_SHIFT_A}};
  printf("GRAVE_ESCAPE = %u (0x%X)\n\n", ranges::GRAVE_ESCAPE, ranges::GRAVE_ESCAPE);
  Sim s;

  // Plain tap
  expect("tap: press -> Esc", s.cycle({GE1})[GE1], E);
  expect("tap: hold -> Esc", s.cycle({GE1})[GE1], E);
  expect("tap: release -> Esc", s.cycle({})[GE1], E);

  // Shift held from earlier cycles; released before the key
  s.cycle({SHIFT});
  expect("shift held: press -> `", s.cycle({SHIFT, GE1})[GE1], B);
  expect("shift let go first: hold stays `", s.cycle({GE1})[GE1], B);
  expect("shift let go first: release is `", s.cycle({})[GE1], B);
  expect("next plain press -> Esc", s.cycle({GE1})[GE1], E);
  s.cycle({});

  // Same-scan changes: Shift is on a lower row than the key
  expect("shift + key in the same scan -> `", s.cycle({SHIFT, GE1})[GE1], B);
  s.cycle({});
  s.cycle({SHIFT});
  expect("shift released in the scan the key goes down -> Esc", s.cycle({GE1})[GE1], E);
  s.cycle({});

  // GUI / Ctrl / Alt
  expect("gui -> `", s.cycle({GUI, GE1})[GE1], B);
  s.cycle({});
  expect("ctrl -> Esc", s.cycle({CTRL, GE1})[GE1], E);
  s.cycle({});
  expect("alt -> Esc", s.cycle({ALT, GE1})[GE1], E);
  s.cycle({});
  expect("ctrl+shift -> `", s.cycle({CTRL, SHIFT, GE1})[GE1], B);
  s.cycle({});

  // One-shot Shift
  OneShot.active = {Key_LeftShift.raw};
  expect("one-shot shift active -> `", s.cycle({GE1})[GE1], B);
  s.cycle({});
  s.cycle({});                     // one-shot still in this report...
  OneShot.active.clear();          // ...then cancelled in afterEachCycle
  expect("one-shot cancelled last cycle -> Esc", s.cycle({GE1})[GE1], E);
  s.cycle({});

  // Qukey resolved to Shift (injected at a position after the key)
  s.qukey_holds_shift = true;
  s.cycle({QUKEY});
  expect("qukey holding shift (later row) -> `", s.cycle({QUKEY, GE1})[GE1], B);
  s.cycle({QUKEY});
  s.cycle({});
  s.qukey_holds_shift = false;
  s.cycle({});
  expect("after qukey released -> Esc", s.cycle({GE1})[GE1], E);
  s.cycle({});

  // Dynamic macro (no location): press, Shift arrives, release -> same key both times
  {
    Key m = GESC;
    GraveEscape.onKeyswitchEvent(m, KeyAddr(200), IS_PRESSED | INJECTED);
    expect("macro press (no shift) -> Esc", m, E);
    Runtime.hid().keyboard().mods.insert(Key_LeftShift.raw);
    m = GESC;
    GraveEscape.onKeyswitchEvent(m, KeyAddr(200), WAS_PRESSED | INJECTED);
    expect("macro release after shift -> still Esc", m, E);
    Runtime.hid().keyboard().mods.clear();
    s.cycle({});
  }

  // Duplicate toggle-on in the same cycle keeps the first choice
  {
    Runtime.device().pressed = {SHIFT, GE1};
    Key m = GESC;
    GraveEscape.onKeyswitchEvent(m, KeyAddr(GE1), IS_PRESSED);
    expect("duplicate: first toggle-on -> `", m, B);
    Runtime.device().pressed = {GE1};  // pretend Shift vanished between deliveries
    m = GESC;
    GraveEscape.onKeyswitchEvent(m, KeyAddr(GE1), IS_PRESSED);
    expect("duplicate: second toggle-on, same cycle -> `", m, B);
    GraveEscape.beforeReportingState();
    s.prev = {GE1};
    s.cycle({});
  }

  // Release that never arrived: next toggle-on decides again
  s.cycle({SHIFT, GE1});
  {
    s.prev.clear();  // the release event was lost
    expect("lost release: new press without shift -> Esc", s.cycle({GE1})[GE1], E);
    s.cycle({});
  }

  // Two Grave Escape keys decided independently
  s.cycle({SHIFT});
  expect("key 1 with shift -> `", s.cycle({SHIFT, GE1})[GE1], B);
  s.cycle({GE1});
  {
    auto r = s.cycle({GE1, GE2});
    expect("key 2 after shift released -> Esc", r[GE2], E);
    expect("key 1 still `", r[GE1], B);
  }
  s.cycle({});

  // Everything else untouched
  s.cycle({SHIFT});
  {
    auto r = s.cycle({SHIFT, PLAIN_ESC, KEY_A});
    expect("plain Escape untouched under shift", r[PLAIN_ESC], E);
    expect("Key_A untouched", r[KEY_A], Key_A);
  }

  printf(fails ? "\n%d FAILED\n" : "\nall passed\n", fails);
  return fails ? 1 : 0;
}
