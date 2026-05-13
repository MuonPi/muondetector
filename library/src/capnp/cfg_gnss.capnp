@0x85ed3aa54bd74035;

struct GnssConfigStructCapnp {
  gnssId @0 :UInt8;
  resTrkCh @1 :UInt8;
  maxTrkCh @2 :UInt8;
  flags @3 :UInt32;
}

struct CfgGNSSCapnp {
  version @0 :UInt8 = 0;
  numTrkChHw @1 :UInt8 = 0;
  numTrkChUse @2 :UInt8 = 0;
  numConfigBlocks @3 :UInt8 = 0;
  configs @4 :List(GnssConfigStructCapnp);
}

struct UbxGnssConfigCmdCapnp {
  configs @0 :List(GnssConfigStructCapnp);
}