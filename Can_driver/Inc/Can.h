/*
 * can.h
 *
 *  Created on: Nov 19, 2026
 *      Author: ht200
 */

#ifndef INC_CAN_H_
#define INC_CAN_H_
#include "Can_GeneralTypes.h"

typedef struct {

} Can_ConfigType;
void Can_Init( const Can_ConfigType* Config);
void Can_GetVersionInfo(Std_VersionInfoType* versioninfo);
void Can_DeInit(void);
Std_ReturnType Can_SetControllerMode( uint8 Controller, Can_ControllerStateType Transition);
Std_ReturnType Can_GetControllerErrorState( uint8 ControllerId, Can_ErrorStateType* ErrorStatePtr);
Std_ReturnType Can_GetControllerMode( uint8 Controller,Can_ControllerStateType* ControllerModePtr);
Std_ReturnType Can_GetControllerRxErrorCounter( uint8 ControllerId,uint8* RxErrorCounterPtr);
Std_ReturnType Can_GetControllerTxErrorCounter( uint8 ControllerId,uint8* TxErrorCounterPtr);
Std_ReturnType Can_Write( Can_HwHandleType Hth,const Can_PduType* PduInfo);

#endif /* INC_CAN_H_ */

