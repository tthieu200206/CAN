/*
 * CanIf_Can.h
 *
 *  Created on: Nov 21, 2026
 *      Author: ht200
 */

#ifndef INC_CANIF_CAN_H_
#define INC_CANIF_CAN_H_

#include "Can_GeneralTypes.h"

void CanIf_TxConfirmation( PduIdType CanTxPduId );
void CanIf_ControllerBusOff( uint8 ControllerId );
void CanIf_ControllerModeIndication( uint8 ControllerId, Can_ControllerStateType ControllerMode );
void CanIf_RxIndication(const Can_HwType* Mailbox,const PduInfoType* PduInfoPtr);
#endif /* INC_CANIF_CAN_H_ */
