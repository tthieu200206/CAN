/*
 * CanIf.h
 *
 *  Created on: Nov 21, 2026
 *      Author: ht200
 */

#ifndef INC_CANIF_H_
#define INC_CANIF_H_
#include "CanIf_Can.h"
typedef struct {

}CanIf_ConfigType;
void CanIf_Init(const CanIf_ConfigType* ConfigPtr);
Std_ReturnType CanIf_Transmit(PduIdType TxPduId,const PduInfoType* PduInfoPtr);

#endif /* INC_CANIF_H_ */
