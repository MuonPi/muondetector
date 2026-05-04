@0xce1fbe371fd8fc6d;

struct GnssSatelliteCapnp {
  gnssId @0 :UInt8;
  svId   @1 :UInt8;
  cno    @2 :UInt8;
}

struct NavSatCapnp {
  iTOW       @0 :UInt32;
  version    @1 :UInt8;
  hasVersion @2 :Bool;
  globFlags  @3 :UInt8;
  hasGlobFlags @4 :Bool;
  numSvs     @5 :UInt8;
  goodSats   @6 :UInt64;
  satellites @7 :List(GnssSatelliteCapnp);
}