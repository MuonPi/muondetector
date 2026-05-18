@0xce1fbe371fd8fc6d;

struct GnssSatelliteCapnp {
  gnssId @0 :UInt8;
  satId  @1 :UInt8;
  cnr    @2 :UInt8;
  elev   @3 :Int8;
  azim   @4 :Int16;
  prRes  @5 :Float32;
  quality @6 :UInt8;
  health  @7 :UInt8;
  orbitSource @8 :UInt8;
  used        @9 :Bool;
  diffCorr    @10 :Bool;
  smoothed    @11 :Bool;
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