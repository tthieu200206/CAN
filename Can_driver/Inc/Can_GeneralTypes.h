/*
 * Can_GeneralTypes.h
 *
 *  Created on: Nov 19, 2026
 *      Author: ht200
 */

#ifndef INC_CAN_GENERALTYPES_H_
#define INC_CAN_GENERALTYPES_H_

#include "StandardTypes.h"
#include "ComStackTypes.h"
#define CAN_BUSY  0x02u

typedef uint32 Can_IdType;
typedef uint8 Can_HwHandleType;

typedef struct {
	PduIdType swPduHandle;
	uint8 length;
	Can_IdType id;
	uint8 *sdu;
} Can_PduType;

typedef struct {
	Can_IdType CanId;
	Can_HwHandleType Hoh;
	uint8 ControllerId;
} Can_HwType;

typedef enum{
	CAN_ERRORSTATE_ACTIVE,
	CAN_ERRORSTATE_PASSIVE,
	CAN_ERRORSTATE_BUSOFF,
}Can_ErrorStateType;

typedef enum{
	CAN_CS_UNINIT = 0x00,
	CAN_CS_STARTED = 0x01,
	CAN_CS_STOPPED = 0x02,
	CAN_CS_SLEEP = 0x03,
}Can_ControllerStateType;
#endif /* INC_CAN_GENERALTYPES_H_ */
