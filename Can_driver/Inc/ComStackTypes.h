/*
 * ComStackTypes.h
 *
 *  Created on: Nov 21, 2026
 *      Author: ht200
 */

#ifndef INC_COMSTACKTYPES_H_
#define INC_COMSTACKTYPES_H_
#include "StandardTypes.h"

typedef uint16 PduIdType;
typedef uint8 PduLengthType;
typedef struct{
	uint8* SduDataPtr;
	uint8* MetaDataPtr;
	PduLengthType SduLength;
}PduInfoType;

#endif /* INC_COMSTACKTYPES_H_ */
