/*
 * Can.c
 *
 *  Created on: Nov 23, 2026
 *      Author: ht200
 */
#include <stddef.h>
#include "Can.h"                    /* Can_ConfigType + 9 API                */
#include "Can_GeneralTypes.h"       /* Can_PduType � Can_HwType � trang thai */
#include "SchM_Can.h"               /* 4 Can_MainFunction_*                  */
#include "CanIf_Can.h"              /* CanIf_RxIndication / TxConfirmation   */

#include "flexcan_driver.h"         /* NXP S32 SDK  (API public)             */
#include "interrupt_manager.h"      /* INT_SYS_DisableIRQ / Global           */
                                     /* + device_registers.h -> CAN0, S32K144 */

/* ---------------------------------------------------------------------------
 *  Ma so module va service id  (SWS_Can 7.2)
 * -------------------------------------------------------------------------*/
#define CAN_MODULE_ID                      (80u)   /* DET: module Can = 80  */
#define CAN_INIT_SID                       (0x00u)
#define CAN_MAINFUNCTION_WRITE_SID         (0x01u)
#define CAN_SETCONTROLLERMODE_SID          (0x03u)
#define CAN_WRITE_SID                      (0x06u)
#define CAN_GETVERSIONINFO_SID             (0x07u)
#define CAN_MAINFUNCTION_READ_SID          (0x08u)
#define CAN_MAINFUNCTION_BUSOFF_SID        (0x09u)
#define CAN_GETCONTROLLERERRORSTATE_SID    (0x0cu)
#define CAN_GETCONTROLLERMODE_SID          (0x0du)
#define CAN_GETCONTROLLERRXERRCNT_SID      (0x0eu)
#define CAN_DEINIT_SID                     (0x0fu)
#define CAN_GETCONTROLLERTXERRCNT_SID      (0x10u)
#define CAN_MAINFUNCTION_MODE_SID          (0x11u)

/* --- Development errors (SWS_Can -- bang loi phat trien) ----------------- */
#define CAN_E_PARAM_POINTER                 (0x01u)
#define CAN_E_PARAM_HANDLE                  (0x02u)
#define CAN_E_PARAM_DLC                     (0x03u)
#define CAN_E_PARAM_CONTROLLER              (0x04u)
#define CAN_E_UNINIT                        (0x05u)
#define CAN_E_TRANSITION                    (0x06u)
#define CAN_E_INIT_FAILED                   (0x09u)

#define CAN_VENDOR_ID                       (0x0000u)
#define CAN_SW_MAJOR_VERSION                (1u)
#define CAN_SW_MINOR_VERSION                (0u)
#define CAN_SW_PATCH_VERSION                (0u)
#define CAN_CONTROLLER_CNT                  (1u)
#define CAN_MAX_MB_CNT                      (16u)
#define CAN_FREEZE_TIMEOUT                  (180u)
#define CAN_DEV_ERROR_DETECT                STD_ON

static uint8 Can_DetServiceId  = 0xFFu;   /* API cuoi bao loi               */
static uint8 Can_DetErrorId    = 0x00u;   /* CAN_E_xxx cuoi cung            */
static uint8 Can_DetErrorCount = 0x00u;   /* so loi ke tu Can_Init          */

#if (CAN_DEV_ERROR_DETECT == STD_ON)
#define CAN_DET(sid, err)          do {Can_DetServiceId  = (sid);   Can_DetErrorId    = (err);  Can_DetErrorCount++;        } while (0)
#else
#define CAN_DET(sid, err)   ((void)0)
#endif


#define CAN_STATE_UNINIT        (0x00u)
#define CAN_STATE_READY         (0x01u)

static uint8                   Can_ModuleState = CAN_STATE_UNINIT;
static const Can_ConfigType   *Can_ConfigPtr   = NULL;
static Can_ControllerStateType Can_CtrlState[CAN_CONTROLLER_CNT];
static flexcan_state_t         Can_FlexcanState[CAN_CONTROLLER_CNT];
static PduIdType               Can_TxPduHandle[CAN_CONTROLLER_CNT][CAN_MAX_MB_CNT];

/* Can_Init la Non Reentrant -> che toan cuc NVIC (SchM_Can.h chua co EA)   */
#define SchM_Enter_Can_CAN_EXCLUSIVE_AREA_0()   INT_SYS_DisableIRQGlobal()
#define SchM_Exit_Can_CAN_EXCLUSIVE_AREA_0()    INT_SYS_EnableIRQGlobal()


#define CAN_MB_WORD_CNT             (4u)
#define CAN_MB_CS_DLC_MASK          (0x000F0000u)
#define CAN_MB_CS_DLC_SHIFT         (16u)
#define CAN_MB_CS_IDE_MASK          (0x00200000u)
#define CAN_ID_STD_SHIFT            (18u)
#define CAN_ID_EXT_FLAG             (0x80000000u)
#define CAN_MB_DATA_BYTES           (8u)


void Can_Init(const Can_ConfigType* Config)
{
    flexcan_user_config_t hwCfg;
    uint8 hwUnit;
    uint8 i;
    Std_ReturnType rv = E_OK;

#if (CAN_DEV_ERROR_DETECT == STD_ON)
    if (Config == NULL)
    {
        CAN_DET(CAN_INIT_SID, CAN_E_PARAM_POINTER);
        return;
    }
    if (Can_ModuleState != CAN_STATE_UNINIT)             /* [SWS_Can_00174]      */
    {
        CAN_DET(CAN_INIT_SID, CAN_E_TRANSITION);
        return;
    }
    if (Can_CtrlState[0] != CAN_CS_UNINIT)
    {
        CAN_DET(CAN_INIT_SID, CAN_E_TRANSITION);           /* [SWS_Can_00408]      */
        return;
    }
    if (((Config->HthCount > 0u) && (Config->Hth == NULL)) ||
        ((Config->HrhCount > 0u) && (Config->Hrh == NULL)))
    {
        CAN_DET(CAN_INIT_SID, CAN_E_PARAM_POINTER);
        return;
    }
#endif

    SchM_Enter_Can_CAN_EXCLUSIVE_AREA_0();

    Can_ConfigPtr = Config;
    hwUnit        = Config->HwUnit;

    /* -- 2. Cau hinh phan cung  [SWS_Can_00245] -------------------------- */
    (void)FLEXCAN_DRV_GetDefaultConfig(&hwCfg);

    /* CanControllerBitTiming */
    hwCfg.bitrate.propSeg    = Config->PropSeg;
    hwCfg.bitrate.phaseSeg1  = Config->PhaseSeg1;
    hwCfg.bitrate.phaseSeg2  = Config->PhaseSeg2;
    hwCfg.bitrate.preDivider = Config->Prescaler;
    hwCfg.bitrate.rJumpwidth = Config->SyncJumpWidth;

    hwCfg.max_num_mb         = (uint32)Config->MaxMsgBuff;
    hwCfg.is_rx_fifo_needed  = false;

    /* FREEZE_MODE == CAN_CS_STOPPED  [SWS_Can_00259] */
    hwCfg.flexcanMode        = FLEXCAN_FREEZE_MODE;

    if (FLEXCAN_DRV_Init(hwUnit, &Can_FlexcanState[hwUnit], &hwCfg) != STATUS_SUCCESS)
    {
        rv = E_NOT_OK;
    }

    /* -- 3. Hardware Object (HOH) ---------------------------------------- */
    for (i = 0u; (i < Config->HrhCount) && (rv == E_OK); i++)
    {
        flexcan_data_info_t info;

        info.msg_id_type = ((Config->Hrh[i].CanId & CAN_ID_EXT_FLAG) != 0u)
                           ? FLEXCAN_MSG_ID_EXT : FLEXCAN_MSG_ID_STD;
        info.data_length = CAN_MB_DATA_BYTES;
        info.is_remote   = false;

        if (FLEXCAN_DRV_ConfigRxMb(hwUnit, Config->Hrh[i].Hoh, &info,
                                    Config->Hrh[i].CanId) != STATUS_SUCCESS)
        {
            rv = E_NOT_OK;
        }
        if ((Config->HrhMask != NULL) && (rv == E_OK))
        {
            if (FLEXCAN_DRV_SetRxIndividualMask(hwUnit, info.msg_id_type,
                                                Config->Hrh[i].Hoh,
                                                Config->HrhMask[i]) != STATUS_SUCCESS)
            {
                rv = E_NOT_OK;
            }
        }
    }

    for (i = 0u; (i < Config->HthCount) && (rv == E_OK); i++)
    {
        flexcan_data_info_t info;

        info.msg_id_type = ((Config->Hth[i].CanId & CAN_ID_EXT_FLAG) != 0u)
                           ? FLEXCAN_MSG_ID_EXT : FLEXCAN_MSG_ID_STD;
        info.data_length = CAN_MB_DATA_BYTES;
        info.is_remote   = false;

        if (FLEXCAN_DRV_ConfigTxMb(hwUnit, Config->Hth[i].Hoh, &info,
        		Config->Hth[i].CanId) != STATUS_SUCCESS)
        {
            rv = E_NOT_OK;
        }
    }

    /*  Che chan NVIC: NXP Init da goi FLEXCAN_EnableIRQs ------------ */
    if (rv == E_OK)
    {
        INT_SYS_DisableIRQ(CAN0_ORed_0_15_MB_IRQn);
        INT_SYS_DisableIRQ(CAN0_ORed_16_31_MB_IRQn);
        INT_SYS_DisableIRQ(CAN0_ORed_IRQn);
        INT_SYS_DisableIRQ(CAN0_Error_IRQn);
        INT_SYS_DisableIRQ(CAN0_Wake_Up_IRQn);
    }

    /* Trang thai module  [SWS_Can_00246] --------------------------- */
    if (rv == E_OK)
    {
        Can_CtrlState[0] = CAN_CS_STOPPED;         /* [SWS_Can_00259]      */
        Can_ModuleState  = CAN_STATE_READY;        /* [SWS_Can_00246]      */
    }
    else
    {
        (void)FLEXCAN_DRV_Deinit(hwUnit);
        Can_ConfigPtr  = NULL;
        Can_ModuleState = CAN_STATE_UNINIT;
        CAN_DET(CAN_INIT_SID, CAN_E_INIT_FAILED);
    }

    SchM_Exit_Can_CAN_EXCLUSIVE_AREA_0();
}

/* =============================================================================
 *  Std_ReturnType Can_SetControllerMode(uint8 Controller,
 *                                      Can_ControllerStateType Transition)
 *  [SWS_Can_00183] � Service ID 0x03
 *  Vao/thoat freeze bang thanh ghi MCR truc tiep (khong goi ham cua SDK).
 * ========================================================================== */
Std_ReturnType Can_SetControllerMode( uint8 Controller, Can_ControllerStateType Transition)
{
    CAN_Type *base = CAN0;          /* S32K144: hwUnit 0 == CAN0            */
    uint32    aux;

#if (CAN_DEV_ERROR_DETECT == STD_ON)
    if (Can_ModuleState != CAN_STATE_READY)         /* [SWS_Can_00198]      */
    {
        CAN_DET(CAN_SETCONTROLLERMODE_SID, CAN_E_UNINIT);
        return E_NOT_OK;
    }
    if (Controller >= CAN_CONTROLLER_CNT)           /* [SWS_Can_00199]      */
    {
        CAN_DET(CAN_SETCONTROLLERMODE_SID, CAN_E_PARAM_CONTROLLER);
        return E_NOT_OK;
    }
#endif

    switch (Transition)
    {
        case CAN_CS_STARTED:            /* STOPPED -> STARTED [SWS_Can_00261] */
            if (Can_CtrlState[Controller] != CAN_CS_STOPPED)
            {
                CAN_DET(CAN_SETCONTROLLERMODE_SID, CAN_E_TRANSITION);
                return E_NOT_OK;
            }
            /* xoa co ESR1 cu truoc khi tham gia bus (tranh bao ngay)       */
            base->ESR1 = CAN_ESR1_BOFFINT_MASK  | CAN_ESR1_BOFFDONEINT_MASK |
                         CAN_ESR1_TWRNINT_MASK  | CAN_ESR1_RWRNINT_MASK     |
                         CAN_ESR1_ERRINT_MASK;

            /* thoat freeze: MCR[HALT]=0, MCR[FRZ]=0, doi MCR[FRZACK]=0     */
            base->MCR = base->MCR & ~CAN_MCR_HALT_MASK;
            base->MCR = base->MCR & ~CAN_MCR_FRZ_MASK;
            aux = CAN_FREEZE_TIMEOUT;

            while (((base->MCR & CAN_MCR_FRZACK_MASK) != 0u) &&
                   (aux > 0u))
            {
                aux--;
            }
            if ((base->MCR & CAN_MCR_FRZACK_MASK) != 0u)
            {
                return E_NOT_OK;
            }
            Can_CtrlState[Controller] = CAN_CS_STARTED;
            break;

        case CAN_CS_STOPPED:
        	if ((Can_CtrlState[Controller] != CAN_CS_STARTED) &&
        	    (Can_CtrlState[Controller] != CAN_CS_SLEEP))
        	{
        	    CAN_DET(CAN_SETCONTROLLERMODE_SID, CAN_E_TRANSITION);
        	    return E_NOT_OK;
        	}
            base->MCR = (base->MCR & ~CAN_MCR_FRZ_MASK)  | CAN_MCR_FRZ_MASK;
            base->MCR = (base->MCR & ~CAN_MCR_HALT_MASK) | CAN_MCR_HALT_MASK;
            aux = CAN_FREEZE_TIMEOUT;

                while (((base->MCR & CAN_MCR_FRZACK_MASK) == 0u) &&
                       (aux > 0u))
                {
                    aux--;
                }

                /* Kiem tra timeout */
                if ((base->MCR & CAN_MCR_FRZACK_MASK) == 0u)
                {
                    return E_NOT_OK;
                }
            INT_SYS_DisableIRQ(CAN0_ORed_0_15_MB_IRQn);
            INT_SYS_DisableIRQ(CAN0_ORed_16_31_MB_IRQn);
            INT_SYS_DisableIRQ(CAN0_ORed_IRQn);
            INT_SYS_DisableIRQ(CAN0_Error_IRQn);
            INT_SYS_DisableIRQ(CAN0_Wake_Up_IRQn);
            Can_CtrlState[Controller] = CAN_CS_STOPPED;
            break;

        case CAN_CS_SLEEP:              /* logical sleep [SWS_Can_00290]     */
        	if (Can_CtrlState[Controller] != CAN_CS_STOPPED)
        	    {
        	        CAN_DET(CAN_SETCONTROLLERMODE_SID, CAN_E_TRANSITION);
        	        return E_NOT_OK;
        	    }

        	    /* Hardware remains in STOPPED (Freeze Mode) */
        	    Can_CtrlState[Controller] = CAN_CS_SLEEP;
        	    break;

        default:
            CAN_DET(CAN_SETCONTROLLERMODE_SID, CAN_E_TRANSITION);
            return E_NOT_OK;
    }

    CanIf_ControllerModeIndication(Controller, Transition);   /* [SWS_Can_00183] */
    return E_OK;
}

/* =============================================================================
 *  Std_ReturnType Can_Write(Can_HwHandleType Hth,
 *                           const Can_PduType* PduInfo)     [SWS_Can_00212]
 *  Service ID 0x06
 *  Theo Can.h cua repo: Std_ReturnType. Het MB ranh thi tra CAN_BUSY (0x02)
 *  -- macro nay Can_GeneralTypes.h da dinh nghia, khong can kieu moi.
 * ========================================================================== */

void Can_MainFunction_Mode(void)
{
    uint32 wuMtc;

#if (CAN_DEV_ERROR_DETECT == STD_ON)
    if (Can_ModuleState != CAN_STATE_READY)
    {
        CAN_DET(CAN_MAINFUNCTION_MODE_SID, CAN_E_UNINIT);
        return;
    }
#endif
    /* S32K1xx KHONG co ESR1.WAKINT nhu MPC5xxx: co wakeup nam trong thanh
     * ghi WU_MTC, chi co gia tri khi Pretended Networking duoc bat.        */
    wuMtc = CAN0->WU_MTC;
    if (((wuMtc & CAN_WU_MTC_WTOF_MASK) != 0u) ||     /* het thoi gian cho */
        ((wuMtc & CAN_WU_MTC_WUMF_MASK) != 0u))       /* khop noi dung loc */
    {
        CAN0->ESR1 = CAN_ESR1_ERRINT_MASK;            /* xoa co ngat loi   */
        CanIf_ControllerModeIndication(Can_ConfigPtr->ControllerId,
                                       CAN_CS_STOPPED);
    }
}

/* =============================================================================
 *  Std_ReturnType Can_GetControllerErrorState(uint8 ControllerId,
 *                                    Can_ErrorStateType* ErrorStatePtr)
 *  Service ID 0x0c -- doc ESR1
 * ========================================================================== */

Std_ReturnType Can_GetControllerMode( uint8 Controller, Can_ControllerStateType* ControllerModePtr)
{
#if (CAN_DEV_ERROR_DETECT == STD_ON)
    if (Can_ModuleState != CAN_STATE_READY)
    {
        CAN_DET(CAN_GETCONTROLLERMODE_SID, CAN_E_UNINIT);   /* [SRS_BSW_00406,SRS_BSW_00416] */
        return E_NOT_OK;
    }
    if (ControllerModePtr == NULL)
    {
        CAN_DET(CAN_GETCONTROLLERMODE_SID, CAN_E_PARAM_POINTER);      /*  [SRS_BSW_00323]  */
        return E_NOT_OK;
    }
    if (Controller >= CAN_CONTROLLER_CNT)
    {
        CAN_DET(CAN_GETCONTROLLERMODE_SID, CAN_E_PARAM_CONTROLLER);     /*  [SRS_BSW_00323]  */
        return E_NOT_OK;
    }
#endif
    *ControllerModePtr = Can_CtrlState[Controller];
    return E_OK;
}



void Can_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (CAN_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL)
    {
        CAN_DET(CAN_GETVERSIONINFO_SID, CAN_E_PARAM_POINTER);     /* [SWS_Can_00177]  */
        return;
    }
#endif
    versioninfo->vendorID         = CAN_VENDOR_ID;
    versioninfo->moduleID         = CAN_MODULE_ID;
    versioninfo->sw_major_version = CAN_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = CAN_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = CAN_SW_PATCH_VERSION;
}

/* =============================================================================
 *  void Can_DeInit(void)      [SWS_Can_00215] � Service ID 0x0f
 *  Chi duoc goi khi moi controller o STOPPED  [SWS_Can_00290]
 * ========================================================================== */
void Can_DeInit(void)
{
    uint8 ctrl;

#if (CAN_DEV_ERROR_DETECT == STD_ON)
    if (Can_ModuleState != CAN_STATE_READY)
    {
        CAN_DET(CAN_DEINIT_SID, CAN_E_TRANSITION);    /*   [SRS_BSW_00369]   */
        return;
    }
#endif
    for (ctrl = 0u; ctrl < CAN_CONTROLLER_CNT; ctrl++)
    {
        if (Can_CtrlState[ctrl] == CAN_CS_STARTED)
        {
            CAN_DET(CAN_DEINIT_SID, CAN_E_TRANSITION);  /*   [SRS_BSW_00369]   */
            return;
        }
    }
    (void)FLEXCAN_DRV_Deinit(Can_ConfigPtr->HwUnit);

    Can_ModuleState = CAN_STATE_UNINIT;
    Can_ConfigPtr   = NULL;
}

