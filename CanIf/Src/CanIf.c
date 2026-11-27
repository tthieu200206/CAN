
#include <stddef.h>
#include "CanIf.h"
#include "CanIf_Can.h"
#include "Can.h"

#define CANIF_STATE_UNINIT       (0x00u)
#define CANIF_STATE_INIT         (0x01u)
#define CANIF_NO_NOTIFICATION    (0u)
#define CANIF_TX_RX_NOTIFICATION (1u)
#define TRUE                     (1u)

static uint8                     CanIf_ModuleState = CANIF_STATE_UNINIT;
static const CanIf_ConfigType   *CanIf_ConfigPtr   = NULL;


static Can_ControllerStateType   CanIf_CtrlState[CANIF_CONTROLLER_CNT];
static Can_ErrorStateType        CanIf_CtrlError[CANIF_CONTROLLER_CNT];
static uint8                     CanIf_PduMode[CANIF_CONTROLLER_CNT];
static uint8                     CanIf_TxBusy[CANIF_TX_PDU_CNT];
static uint16                    CanIf_TxConfirmCnt[CANIF_TX_PDU_CNT];
static uint8                     CanIf_TxNotifStatus[CANIF_TX_PDU_CNT];
static uint16                    CanIf_RxCnt[CANIF_RX_PDU_CNT];
static Can_IdType                CanIf_RxLastId[CANIF_RX_PDU_CNT];
static PduLengthType             CanIf_RxLastLen[CANIF_RX_PDU_CNT];
static uint8                     CanIf_RxLastData[CANIF_RX_PDU_CNT][8];
static uint16                    CanIf_BusOffCnt[CANIF_CONTROLLER_CNT];

/* --- DET noi bo -------------------------------------------------------- */
static uint8  CanIf_DetServiceId  = 0xFFu;
static uint8  CanIf_DetErrorId    = 0x00u;
static uint16 CanIf_DetErrorCount = 0x00u;

#if (CANIF_DEV_ERROR_DETECT == STD_ON)
#define CANIF_DET(sid, err) do { CanIf_DetServiceId = (sid); CanIf_DetErrorId = (err); CanIf_DetErrorCount++; } while (0)
#else
#define CANIF_DET(sid, err)   ((void)0)
#endif


void CanIf_Init(const CanIf_ConfigType* ConfigPtr)
{
    uint8 i;

#if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL)
    {
        CANIF_DET(CANIF_INIT_SID, CANIF_E_PARAM_POINTER);
        return;
    }
    if (ConfigPtr->TxPduCnt > CANIF_TX_PDU_CNT ||
        ConfigPtr->RxPduCnt > CANIF_RX_PDU_CNT ||
        ConfigPtr->ControllerCnt > CANIF_CONTROLLER_CNT)
    {
        CANIF_DET(CANIF_INIT_SID, CANIF_E_INIT_FAILED);
        return;
    }
#endif

    CanIf_ConfigPtr   = ConfigPtr;

    for (i = 0u; i < CANIF_CONTROLLER_CNT; i++)
    {
        CanIf_CtrlState[i]  = CAN_CS_STOPPED;       /* Can moi vua STOPPED  */
        CanIf_CtrlError[i]  = CAN_ERRORSTATE_ACTIVE;
        CanIf_PduMode[i]    = CANIF_OFFLINE;        /* cho den khi STARTED  */
        CanIf_BusOffCnt[i]  = 0u;
    }
    for (i = 0u; i < CANIF_TX_PDU_CNT; i++)
    {
        CanIf_TxBusy[i]        = 0u;
        CanIf_TxConfirmCnt[i]  = 0u;
        CanIf_TxNotifStatus[i] = CANIF_NO_NOTIFICATION;
    }
    for (i = 0u; i < CANIF_RX_PDU_CNT; i++)
    {
        CanIf_RxCnt[i]    = 0u;
        CanIf_RxLastId[i] = 0u;
        CanIf_RxLastLen[i] = 0u;
        for (uint8 k = 0u; k < 8u; k++)
        {
            CanIf_RxLastData[i][k] = 0u;
        }
    }
    CanIf_ModuleState = CANIF_STATE_INIT;
}



/* =============================================================================
 *  void CanIf_ControllerBusOff( uint8 ControllerId )
 *  duoc Can_MainFunction_BusOff (polling) goi khi ESR1[BOFFINT] dat
 * ========================================================================== */
void CanIf_ControllerBusOff( uint8 ControllerId )
{
#if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_ModuleState != CANIF_STATE_INIT)
    {
        return;
    }
    if (ControllerId >= CanIf_ConfigPtr->ControllerCnt)
    {
    	CANIF_DET(CANIF_CONTROLLERBUSOFF_SID,CANIF_E_PARAM_CONTROLLERID);   /* [SWS_CANIF_00429] */
        return;
    }
#endif
    CanIf_BusOffCnt[ControllerId]++;
    CanIf_CtrlError[ControllerId] = CAN_ERRORSTATE_BUSOFF;
    /* [SWS_CANIF_00062] khi bus-off, CanIf dua PDU mode ve OFFLINE         */
    CanIf_PduMode[ControllerId] = CANIF_OFFLINE;
}

/* =============================================================================
 *  void CanIf_ControllerModeIndication( uint8 ControllerId,
 *                                Can_ControllerStateType ControllerMode )
 *  duoc Can_SetControllerMode cua Can driver goi
 * ========================================================================== */
void CanIf_ControllerModeIndication( uint8 ControllerId, Can_ControllerStateType ControllerMode )
{
	if (CanIf_ModuleState != CANIF_STATE_INIT)
	{
	    return;
	}
    if (ControllerId >= CanIf_ConfigPtr->ControllerCnt)
    {
#if (CANIF_DEV_ERROR_DETECT == STD_ON)
    CANIF_DET(CANIF_CONTROLLERMODEINDICATION_SID, CANIF_E_PARAM_CONTROLLERID);  /* [SWS_CANIF_00700] */
#endif
        return;
    }
#if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if ((ControllerMode != CAN_CS_STOPPED) &&
        (ControllerMode != CAN_CS_STARTED) &&
        (ControllerMode != CAN_CS_SLEEP))
    {
        CANIF_DET(CANIF_INIT_SID, CANIF_E_PARAM_CTRLMODE);
        return;
    }
#endif

    CanIf_CtrlState[ControllerId] = ControllerMode;

    if (ControllerMode == CAN_CS_STARTED)
    {
        /* controller da tren bus -> mo PDU mode theo cau hinh mac dinh     */
        CanIf_PduMode[ControllerId] = CanIf_ConfigPtr->DefaultPduMode;
    }
    else
    {
        CanIf_PduMode[ControllerId] = CANIF_OFFLINE;
    }
}


/* --- bang phat: 2 TxL-PDU, dung 2 HTH cua Can_ConfigSet (MB 8 va MB 9) -- */
static const Can_IdType       CanIf_TxCanId[CANIF_TX_PDU_CNT]    = { 0x123u, 0x18FF0102u };
static const Can_HwHandleType CanIf_TxHth[CANIF_TX_PDU_CNT]      = { 8u, 9u };
static const uint8            CanIf_TxCtrl[CANIF_TX_PDU_CNT]     = { 0u, 0u };
static const uint8 CanIf_TxPduReadNotifyStatus[CANIF_TX_PDU_CNT] ={ TRUE,TRUE };

/* --- bang nhan: 2 RxL-PDU, ung HRH MB 12 (loc 0x201) va MB 13 (khong) -- */
static const Can_HwHandleType CanIf_RxHoh[CANIF_RX_PDU_CNT]   = { 12u, 13u };
static const Can_IdType       CanIf_RxCanId[CANIF_RX_PDU_CNT] = { 0x201u, 0x000u };
static const PduIdType        CanIf_RxPdu[CANIF_RX_PDU_CNT]   = { 10u, 11u };

const CanIf_ConfigType CanIf_ConfigSet = {
    .DefaultPduMode  = CANIF_ONLINE,     /* vao mode nay khi STARTED       */
    .ControllerCnt   = 1u,
    .TxPduCnt        = 2u,
    .RxPduCnt        = 2u,

    .TxPduCanId      = &CanIf_TxCanId[0],
    .TxPduHth        = &CanIf_TxHth[0],
    .TxPduCtrl       = &CanIf_TxCtrl[0],

    .RxPduHoh        = &CanIf_RxHoh[0],
    .RxPduCanId      = &CanIf_RxCanId[0],
    .RxPduId         = &CanIf_RxPdu[0],
	.PublicReadTxPduNotifyStatusApi = TRUE,
	.TxPduReadNotifyStatus = CanIf_TxPduReadNotifyStatus
};


