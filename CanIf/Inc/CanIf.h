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
    uint8 DefaultPduMode;
    uint8 ControllerCnt;
    uint8 TxPduCnt;
    uint8 RxPduCnt;

    uint8 PublicReadTxPduNotifyStatusApi;
    const uint8 *TxPduReadNotifyStatus;

    const Can_IdType *TxPduCanId;
    const Can_HwHandleType *TxPduHth;
    const uint8 *TxPduCtrl;
    const Can_HwHandleType *RxPduHoh;
    const Can_IdType *RxPduCanId;
    const PduIdType *RxPduId;
} CanIf_ConfigType;

/* --- Bien cau hinh post-build (khai bao bien, khong phai ham) ---------- */
extern const CanIf_ConfigType CanIf_ConfigSet;

/* --- Nhanh chot & DET ------------------------------------------------- */
#define CANIF_DEV_ERROR_DETECT   STD_ON
#define CANIF_MODULE_ID          (60u)     /* DET: CanIf = 60 (0x3C)       */
#define CANIF_TX_PDU_CNT         (2u)
#define CANIF_RX_PDU_CNT         (2u)
#define CANIF_CONTROLLER_CNT     (1u)

/* Service IDs  (SWS_CanIf 8.3 -- bang dinh nghia dich vu) */
#define CANIF_INIT_SID                            (0x01u)   /* [SWS_CANIF_00001]       */
#define CANIF_TRANSMIT_SID                        (0x49u)   /* [SWS_CANIF_00005]       */
#define CANIF_TXCONFIRMATION_SID                  (0x13u)   /* [SWS_CANIF_00007]       */
#define CANIF_RXINDICATION_SID                    (0x14u)
#define CANIF_CONTROLLERBUSOFF_SID                (0x16u)/* [SWS_CANIF_00006]       */
#define CANIF_CONTROLLERMODEINDICATION_SID        (0x17u)
/* Development errors  (SWS_CanIf -- bang loi phat trien) */
#define CANIF_E_PARAM_HOH             (12u)
#define CANIF_E_PARAM_LPDU            (13u)
#define CANIF_E_PARAM_CONTROLLERID    (15u)
#define CANIF_E_PARAM_POINTER         (20u)
#define CANIF_E_PARAM_CTRLMODE        (21u)
#define CANIF_E_PARAM_PDU_MODE        (22u)
#define CANIF_E_UNINIT                (30u)
#define CANIF_E_INVALID_TXPDUID       (50u)
#define CANIF_E_INVALID_RXPDUID       (60u)
#define CANIF_E_INVALID_DATA_LENGTH   (61u)
#define CANIF_E_DATA_LENGTH_MISMATCH  (62u)
#define CANIF_E_STOPPED               (70u)
#define CANIF_E_INIT_FAILED           (80u)
#define CANIF_E_TXPDU_LENGTH_EXCEEDED (90u)

/* PDU mode -- dung macro thay vi enum moi */
#define CANIF_OFFLINE                 (0x00u)
#define CANIF_TX_OFFLINE              (0x01u)
#define CANIF_TX_OFFLINE_ACTIVE       (0x02u)
#define CANIF_ONLINE                  (0x03u)

void CanIf_Init(const CanIf_ConfigType* ConfigPtr);
Std_ReturnType CanIf_Transmit(PduIdType TxPduId,const PduInfoType* PduInfoPtr);

#endif /* INC_CANIF_H_ */
