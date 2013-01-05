/*
 * Copyright (c) 2011 Toshihisa T
 * Released under the MIT License: http://mbed.org/license/mit
 */

#ifndef __UBXPACKET_H__ /* { */

struct UBXPacket_s {
    unsigned char cls;
    unsigned char id;
    unsigned short len;
    unsigned char body[1024];
    unsigned char sum[2];
    int cjobst;
    int idx;
};

#ifdef __cplusplus
extern "C" {
#endif
void UBXPacket_CalcSum(struct UBXPacket_s *info,int c);
int UBXPacket_Parse(struct UBXPacket_s *info,int c);
#ifdef __cplusplus
}
#endif

#endif  /* __UBXPACKET_H__ } */
