@0xbf514e6c8d2e4a11;

struct CfgMsgCapnp {
  msgID @0 :UInt16;
  rate  @1 :Int32;
}

struct UbxMsgRatesCapnp {
  data @0 :List(CfgMsgCapnp);
}