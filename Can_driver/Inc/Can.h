/*
 * can.h
 *
 *  Created on: Nov 23, 2026
 *      Author: ht200
 */
#ifndef INC_CAN_H_
#define INC_CAN_H_
#include "Can_GeneralTypes.h"



typedef struct {
    /* --- controller / HW unit ------------------------------------------- */
    uint8                  ControllerId;  /* CanControllerId: 0            */
    uint8                  HwUnit;        /* instance FLEXCAN: 0 = CAN0    */

    /* --- CanControllerBitTiming (DoControllerBitTiming) ----------------- */
    uint32                 Prescaler;     /* CanControllerPrescaler        */
    uint32                 PropSeg;       /* CanControllerPropSeg          */
    uint32                 PhaseSeg1;     /* CanControllerPhaseSeg1        */
    uint32                 PhaseSeg2;     /* CanControllerPhaseSeg2        */
    uint32                 SyncJumpWidth; /* CanControllerSyncJumpWidth    */

    /* --- Hardware Object (HOH) -- dung struct Can_GeneralTypes.h -------- */
    const Can_HwType      *Hth;           /* HTH (gui): Hoh = so MB dau    */
    uint8                  HthCount;
    const Can_HwType      *Hrh;           /* HRH (nhan): Hoh = so MB       */
    uint8                  HrhCount;
    const Can_IdType      *HrhMask;       /* mask loc ung tung HRH         */

    /* --- gioi han phan cung -------------------------------------------- */
    uint8                  MaxMsgBuff;    /* CanControllerMaxMsgBufNum = 16 */
} Can_ConfigType;

extern const Can_ConfigType Can_ConfigSet;

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


