#include <Arduino.h>
#include "driver/gpio.h"
#include "driver/twai.h"

/* =========================================================================
 * 1. AUTOSAR 4.4 STANDARD & GENERAL TYPES
 * ========================================================================= */
typedef uint8_t Std_ReturnType;

#define E_OK        ((Std_ReturnType)0x00U)
#define E_NOT_OK    ((Std_ReturnType)0x01U)

typedef struct {
    uint16_t vendorID;
    uint16_t moduleID;
    uint8_t  sw_major_version;
    uint8_t  sw_minor_version;
    uint8_t  sw_patch_version;
} Std_VersionInfoType;

typedef enum {
    CAN_CS_UNINIT  = 0x00U,
    CAN_CS_STARTED = 0x01U,
    CAN_CS_STOPPED = 0x02U,
    CAN_CS_SLEEP   = 0x03U
} Can_ControllerStateType;

/* =========================================================================
 * 2. CONFIG STRUCTURES & PROTOTYPES (Can.h & CanIf.h)
 * ========================================================================= */
#define CAN_VENDOR_ID          0x0045U
#define CAN_MODULE_ID          0x0050U
#define CAN_SW_MAJOR_VERSION   1U
#define CAN_SW_MINOR_VERSION   0U
#define CAN_SW_PATCH_VERSION   0U

typedef struct {
    uint8_t    ControllerId;
    gpio_num_t TxPin;
    gpio_num_t RxPin;
    uint32_t   BaudrateKbps;
} Can_ControllerConfigType;

typedef struct {
    const Can_ControllerConfigType* Controllers;
    uint8_t NumControllers;
} Can_ConfigType;

typedef struct {
    const Can_ConfigType* CanConfigPtr;
} CanIf_ConfigType;

/* Khai báo các hàm API phần của Nghĩa */
void           Can_Init(const Can_ConfigType* Config);
void           Can_GetVersionInfo(Std_VersionInfoType* versioninfo);
Std_ReturnType Can_SetControllerMode(uint8_t Controller, Can_ControllerStateType Transition);
Std_ReturnType Can_GetControllerMode(uint8_t Controller, Can_ControllerStateType* ControllerModePtr);
void           Can_MainFunction_Mode(void);

void           CanIf_Init(const CanIf_ConfigType* ConfigPtr);
void           CanIf_ControllerModeIndication(uint8_t ControllerId, Can_ControllerStateType ControllerMode);
void           CanIf_ControllerBusOff(uint8_t ControllerId);

/* =========================================================================
 * 3. TẦNG ECU ABSTRACTION (CanIf)
 * ========================================================================= */
static Can_ControllerStateType CanIf_CtrlMode   = CAN_CS_UNINIT;
static bool                    CanIf_BusOffFlag = false;

void CanIf_Init(const CanIf_ConfigType* ConfigPtr) {
    if ((ConfigPtr == NULL) || (ConfigPtr->CanConfigPtr == NULL)) {
        return;
    }
    Can_Init(ConfigPtr->CanConfigPtr);
    CanIf_CtrlMode   = CAN_CS_STOPPED;
    CanIf_BusOffFlag = false;
}

Std_ReturnType CanIf_SetControllerMode(uint8_t ControllerId, Can_ControllerStateType ControllerMode) {
    return Can_SetControllerMode(ControllerId, ControllerMode);
}

void CanIf_ControllerModeIndication(uint8_t ControllerId, Can_ControllerStateType ControllerMode) {
    (void)ControllerId;
    CanIf_CtrlMode = ControllerMode;
    if (ControllerMode == CAN_CS_STARTED) {
        CanIf_BusOffFlag = false;
    }
}

void CanIf_ControllerBusOff(uint8_t ControllerId) {
    (void)ControllerId;
    CanIf_BusOffFlag = true;
    CanIf_CtrlMode   = CAN_CS_STOPPED;
}

Can_ControllerStateType CanIf_GetCachedControllerMode(uint8_t ControllerId) {
    (void)ControllerId;
    return CanIf_CtrlMode;
}

bool CanIf_IsBusOff(uint8_t ControllerId) {
    (void)ControllerId;
    return CanIf_BusOffFlag;
}

/* =========================================================================
 * 4. TẦNG MCAL CAN DRIVER (Can)
 * ========================================================================= */
static Can_ControllerStateType Can_CurrentState = CAN_CS_UNINIT;
static Can_ControllerStateType Can_TargetState  = CAN_CS_UNINIT;
static bool                    Can_ModePending  = false;
static uint8_t                 Can_ActiveCtrlId = 0U;

void Can_Init(const Can_ConfigType* Config) {
    if ((Config == NULL) || (Config->Controllers == NULL) || (Config->NumControllers == 0U)) {
        return;
    }
    if (Can_CurrentState != CAN_CS_UNINIT) {
        return;
    }

    const Can_ControllerConfigType* ctrl = &Config->Controllers[0];
    Can_ActiveCtrlId = ctrl->ControllerId;

    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(ctrl->TxPin, ctrl->RxPin, TWAI_MODE_NORMAL);
    twai_timing_config_t  t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t  f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        Can_CurrentState = CAN_CS_STOPPED;
        Can_TargetState  = CAN_CS_STOPPED;
        Can_ModePending  = false;
    }
}

void Can_GetVersionInfo(Std_VersionInfoType* versioninfo) {
    if (versioninfo == NULL) {
        return;
    }
    versioninfo->vendorID         = CAN_VENDOR_ID;
    versioninfo->moduleID         = CAN_MODULE_ID;
    versioninfo->sw_major_version = CAN_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = CAN_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = CAN_SW_PATCH_VERSION;
}

Std_ReturnType Can_SetControllerMode(uint8_t Controller, Can_ControllerStateType Transition) {
    if ((Can_CurrentState == CAN_CS_UNINIT) || (Controller != Can_ActiveCtrlId)) {
        return E_NOT_OK;
    }

    switch (Transition) {
        case CAN_CS_STARTED:
            if (Can_CurrentState == CAN_CS_STOPPED) {
                if (twai_start() == ESP_OK) {
                    Can_TargetState = CAN_CS_STARTED;
                    Can_ModePending = true;
                    return E_OK;
                }
            }
            break;

        case CAN_CS_STOPPED:
            if (Can_CurrentState == CAN_CS_STARTED) {
                if (twai_stop() == ESP_OK) {
                    Can_TargetState = CAN_CS_STOPPED;
                    Can_ModePending = true;
                    return E_OK;
                }
            } else if (Can_CurrentState == CAN_CS_SLEEP) {
                Can_TargetState = CAN_CS_STOPPED;
                Can_ModePending = true;
                return E_OK;
            }
            break;

        case CAN_CS_SLEEP:
            if (Can_CurrentState == CAN_CS_STOPPED) {
                Can_TargetState = CAN_CS_SLEEP;
                Can_ModePending = true;
                return E_OK;
            }
            break;

        default:
            break;
    }

    return E_NOT_OK;
}

Std_ReturnType Can_GetControllerMode(uint8_t Controller, Can_ControllerStateType* ControllerModePtr) {
    if ((Can_CurrentState == CAN_CS_UNINIT) || (Controller != Can_ActiveCtrlId) || (ControllerModePtr == NULL)) {
        return E_NOT_OK;
    }
    *ControllerModePtr = Can_CurrentState;
    return E_OK;
}

void Can_MainFunction_Mode(void) {
    if (Can_CurrentState == CAN_CS_UNINIT) {
        return;
    }

    twai_status_info_t status_info;
    if (twai_get_status_info(&status_info) == ESP_OK) {
        if (status_info.state == TWAI_STATE_BUS_OFF) {
            Can_CurrentState = CAN_CS_STOPPED;
            CanIf_ControllerBusOff(Can_ActiveCtrlId);
            return;
        }

        if (Can_ModePending) {
            bool state_reached = false;
            if ((Can_TargetState == CAN_CS_STARTED) && (status_info.state == TWAI_STATE_RUNNING)) {
                state_reached = true;
            } else if ((Can_TargetState == CAN_CS_STOPPED) && (status_info.state == TWAI_STATE_STOPPED)) {
                state_reached = true;
            } else if (Can_TargetState == CAN_CS_SLEEP) {
                state_reached = true;
            }

            if (state_reached) {
                Can_CurrentState = Can_TargetState;
                Can_ModePending  = false;
                CanIf_ControllerModeIndication(Can_ActiveCtrlId, Can_CurrentState);
            }
        }
    }
}

/* =========================================================================
 * 5. CẤU HÌNH & CHƯƠNG TRÌNH CHÍNH (UNIT TEST)
 * ========================================================================= */
const Can_ControllerConfigType esp32CanCtrlConfig = {
    .ControllerId = 0U,
    .TxPin        = GPIO_NUM_21,
    .RxPin        = GPIO_NUM_22,
    .BaudrateKbps = 500U
};

const Can_ConfigType canConfig = {
    .Controllers    = &esp32CanCtrlConfig,
    .NumControllers = 1U
};

const CanIf_ConfigType canIfConfig = {
    .CanConfigPtr = &canConfig
};

void setup() {
    Serial.begin(115200);
    delay(1000);

    Std_VersionInfoType ver;
    Can_GetVersionInfo(&ver);
    Serial.printf("[TEST 1] Version: %d.%d.%d (ModuleID: 0x%X)\n", 
                  ver.sw_major_version, ver.sw_minor_version, ver.sw_patch_version, ver.moduleID);

    CanIf_Init(&canIfConfig);
    Can_ControllerStateType mode;
    Can_GetControllerMode(0, &mode);
    Serial.printf("[TEST 2] Sau Init -> Mode: %d (Ky vong: 2 - STOPPED)\n", mode);

    CanIf_SetControllerMode(0, CAN_CS_STARTED);
    Can_MainFunction_Mode();
    Can_GetControllerMode(0, &mode);
    Serial.printf("[TEST 3] Sau Start -> Can Mode: %d | CanIf Mode: %d (Ky vong: 1 - STARTED)\n", 
                  mode, CanIf_GetCachedControllerMode(0));
}

void loop() {
    Can_MainFunction_Mode();
    delay(100);
}