/*
 * StandardTypes.h
 *
 *  Created on: Nov 19, 2026
 *      Author: ht200
 */

#ifndef INC_STANDARDTYPES_H_
#define INC_STANDARDTYPES_H_
#include<stdint.h>

#define E_OK       0x00u
#define E_NOT_OK   0x01u
#define STD_LOW    0x00u
#define STD_HIGH   0x01u
#define STD_IDLE   0x00u
#define STD_ACTIVE 0x01u
#define STD_OFF    0x00u
#define STD_ON     0x01u

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef uint8 Std_ReturnType;
typedef struct {
	uint16 vendorID;
	uint16 moduleID;
	uint8 sw_major_version;
	uint8 sw_minor_version;
	uint8 sw_patch_version;
} Std_VersionInfoType;



#endif /* INC_STANDARDTYPES_H_ */
