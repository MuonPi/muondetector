@0xcaf8efa86d2e5302;

struct CalibEventCapnp {
  valid @0 :Bool = false;
  eepromValid @1 :Bool = false;
  id @2 :UInt64 = 0;
  calibList @3 :List(CalibStructCapnp);
}

struct CalibStructCapnp {
  name @0 :Text;
  type @1 :Text;
  address @2 :UInt16 = 0;
  value @3 :Text;
}