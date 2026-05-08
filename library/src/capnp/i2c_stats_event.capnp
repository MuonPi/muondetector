@0x8fb1cb4c511a6ebb;

struct I2cDeviceEntryCapnp {
  address @0 :UInt8;
  name @1 :Text;
  status @2 :UInt8;

  nrBytesWritten @3 :UInt32;
  nrBytesRead @4 :UInt32;
  nrIoErrors @5 :UInt32;

  lastTransactionTime @6 :UInt32;  # microseconds
}

struct I2cStatsEventCapnp {
  nrDevices @0 :UInt8 = 0;
  bytesRead @1 :UInt32 = 0;
  bytesWritten @2 :UInt32 = 0;
  deviceList @3 :List(I2cDeviceEntryCapnp);
}