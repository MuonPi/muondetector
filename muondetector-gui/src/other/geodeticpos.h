#ifndef GEODETICPOS_H
#define GEODETICPOS_H

struct GeodeticPos {
    uint32_t iTOW;
    int32_t lon; // longitude 1e-7 scaling (increase by 1 means 100 nano degrees)
    int32_t lat; // latitude 1e-7 scaling (increase by 1 means 100 nano degrees)
    int32_t height; // height above ellipsoid
    int32_t hMSL; // height above main sea level
    uint32_t hAcc; // horizontal accuracy estimate
    uint32_t vAcc; // vertical accuracy estimate
};
#endif // GEODETICPOS_H
