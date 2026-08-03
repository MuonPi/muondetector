#ifndef _SEN0321_H_
#define _SEN0321_H_

#include "device_types.h"
#include "i2c/i2cdevice.h"

#include <cstdint>

class SEN0321 : public i2cDevice,
                public DeviceFunction<DeviceType::OTHER>,
                public static_device_base<SEN0321> {
  public:
    static constexpr std::uint8_t DEFAULT_ADDRESS{0x73};

    enum class MeasureMode : std::uint8_t {
        AUTOMATIC = 0x00,
        PASSIVE = 0x01,
    };

    SEN0321();
    SEN0321(const char* busAddress, std::uint8_t slaveAddress);
    SEN0321(std::uint8_t slaveAddress);
    virtual ~SEN0321();

    bool init(MeasureMode mode = MeasureMode::AUTOMATIC);
    bool setMode(MeasureMode mode);
    MeasureMode mode() const { return fModeSetting; }

    bool readOzoneRaw(std::uint16_t& ozone);
    bool getOzonRawValue(std::uint16_t& ozone);
    bool getOzonePpb(double& ozonePpb);
    bool getOzone(double& ozone);

    bool identify() override;
    bool probeDevicePresence() override { return devicePresent(); }
    bool devicePresent() override;

  private:
    enum REG : std::uint8_t {
        MODE = 0x03,
        PASSIVE_READ_COMMAND = 0x04,
        OZONE_DATA = 0x09,
    };

    MeasureMode fModeSetting{MeasureMode::AUTOMATIC};
};

#endif // _SEN0321_H_
