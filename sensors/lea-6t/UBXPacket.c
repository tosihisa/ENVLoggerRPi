/*
 * Copyright (c) 2011 Toshihisa T
 * Released under the MIT License: http://mbed.org/license/mit
 */

#include "UBXPacket.h"

void UBXPacket_CalcSum(struct UBXPacket_s *info,int c)
{
    *(info->sum + 0) = *(info->sum + 0) + ((unsigned char)c);
    *(info->sum + 1) = *(info->sum + 1) + *(info->sum + 0);
}

int UBXPacket_Parse(struct UBXPacket_s *info,int c)
{
    if(info->cjobst == 0){
        info->cjobst = (c == 0xB5) ? 1 /* OK */ : 0 /*Err*/;
    } else if(info->cjobst == 1){
        info->cjobst = (c == 0x62) ? 2 /* OK */ : 0 /*Err*/;
        info->sum[0] = info->sum[1] = 0;
    } else if(info->cjobst == 2){
        UBXPacket_CalcSum(info,c);
        info->cls = (unsigned char)c;
        info->cjobst++;
    } else if(info->cjobst == 3){
        UBXPacket_CalcSum(info,c);
        info->id = (unsigned char)c;
        info->cjobst++;
    } else if(info->cjobst == 4){
        UBXPacket_CalcSum(info,c);
        info->len = (unsigned char)c;
        info->cjobst++;
    } else if(info->cjobst == 5){
        UBXPacket_CalcSum(info,c);
        info->len |= (((unsigned short)c) << 8);
        info->idx = 0;
        info->cjobst++;
    } else if(info->cjobst == 6){
        UBXPacket_CalcSum(info,c);
        info->body[info->idx] = (unsigned char)c;
        info->idx++;
        if(info->idx >= info->len){
            info->cjobst = 7;
        }
    } else if(info->cjobst == 7){
        info->cjobst = (c == info->sum[0]) ? 8 /* OK */ : 0 /*Err*/;
    } else if(info->cjobst == 8){
        info->cjobst = (c == info->sum[1]) ? 100 /* OK */ : 0 /*Err*/;
    }
    return info->cjobst;
}



