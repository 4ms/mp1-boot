#
# rom_trace_analyzer_py : script to analyze STM32MP13 ROM code traces
# 
import os
import sys
import struct
import argparse


levels = (
    'INFO',
    'WARN',
    'ERR ',
    'DEBUG',
)


def level2str(level):
    try:
       return levels[level]
    except IndexError:
       return "UNKNOWN"


tags = {
   0x90000001 : "NAND_DetectionFailed",
   0x90000002 : "NAND_InitFailed",
   0x90000003 : "NAND_NotInitialized",
   0x90000004 : "NAND_NotOnfiOrNotDetected",
   0x90000005 : "NAND_TransferFailed",
   0x90000006 : "NAND_ImageTooBig",
   0x90000007 : "NAND_OnlyBadBlocks",
   0x90000008 : "NAND_UncorrectableEccErr",
   0x90000009 : "NAND_NotGPTFound",
   0x9000000A : "NAND_TryingSECBL1",
   0x9000000B : "NAND_TryingSECBL2",
   0x9000000C : "NAND_SecblFirstValidBlock",
   0x9000000E : "NAND_EccErrorsDetected",
   0x9000000F : "NAND_EccErrorCorrected",
   0x90000010 : "NAND_EccErrorNotInDataArea",
   0x90000011 : "NAND_SecblsFoundInGPT",
   0x90000012 : "NAND_ReadColAddr",
   0x90000013 : "NAND_ResetFailed",
   0x90000014 : "NAND_NoOnfiParameterPage",
   0x90000015 : "NAND_GetParamFromOtp",
   0x90000016 : "NAND_EccBitsNbForcedByOtp",
   0x90000017 : "NAND_NoValidBlockFound",
   0x90000018 : "NAND_GptFound",
   0x90000019 : "NAND_DirectFsblFound",
   0x9000001A : "NAND_TryingDirectFSBL",
   0x9000001B : "NAND_WrongFsblOffsetsInGPT",
   0x9000001C : "NAND_EndOfNandReached",
   0x90000101 : "NANDCORE_DetectionFailed",
   0x90000102 : "NANDCORE_InitFailed",
   0x90000103 : "NANDCORE_NotInitialized",
   0x90000104 : "NANDCORE_NotOnfiOrNotDetected",
   0x90000105 : "NANDCORE_TransferFailed",
   0x90000106 : "NANDCORE_ImageTooBig",
   0x90000107 : "NANDCORE_OnlyBadBlocks",
   0x90000108 : "NANDCORE_UncorrectableEccErr",
   0x90000109 : "NANDCORE_NotGPTFound",
   0x9000010A : "NANDCORE_TryingSECBL1",
   0x9000010B : "NANDCORE_TryingSECBL2",
   0x9000010E : "NANDCORE_EccErrorsDetected",
   0x9000010F : "NANDCORE_EccErrorCorrected",
   0x90000110 : "NANDCORE_EccErrorNotInDataArea",
   0x9000010C : "NANDCORE_SecblFirstValidBlock",
   0x90000111 : "NANDCORE_SecblsFoundInGPT",
   0x90000112 : "NANDCORE_ReadColAddr",
   0x90000113 : "NANDCORE_ResetFailed",
   0x90000114 : "NANDCORE_NoOnfiParameterPage",
   0x90000115 : "NANDCORE_GetParamFromOtp",
   0x90000116 : "NANDCORE_EccBitsNbForcedByOtp",
   0x90000117 : "NANDCORE_NoValidBlockFound",
   0x90000118 : "NANDCORE_GptFound",
   0x90000119 : "NANDCORE_DirectFsblFound",
   0x9000011A : "NANDCORE_TryingDirectFSBL",
   0x9000011B : "NANDCORE_WrongFsblOffsetsInGPT",
   0x9000011C : "NANDCORE_EndOfNandReached",
   0x9000020E : "FMC_NAND_LL_EccErrorsDetected",
   0x9000020F : "FMC_NAND_LL_EccErrorCorrected",
   0x90000210 : "FMC_NAND_LL_EccErrorNotInDataArea",
   0x90000251 : "QSPI_NAND_LL_NoEccErrorOrCorrected",
   0x90000252 : "QSPI_NAND_LL_UncorrectableEccErr",
   0x40000001 : "MMC_AcknowledgeErr",
   0x40000002 : "MMC_AcknowledgeTimeout",
   0x40000003 : "MMC_CardNotDetected",
   0x40000004 : "MMC_DataCrcErr",
   0x40000005 : "MMC_DataTimeout",
   0x40000007 : "MMC_DmaTransferErr",
   0x40000008 : "MMC_CmdTimeoutErr",
   0x40000009 : "MMC_CmdCrcErr",
   0x4000000A : "MMC_AddAlignErr",
   0x4000000B : "MMC_NotEnoughBootDataRxErr",
   0x4000000C : "MMC_ImageCopiedIsNotComplete",
   0x4000000D : "MMC_ImageCopiedHeaderSizeZero",
   0x4000000E : "MMC_AttemptBootDespiteErrors",
   0x4000000F : "MMC_GotDataEndFlagWithNoErrors",
   0x40000010 : "MMC_NbBytesCopiedInRam",
   0x40000011 : "MMC_ImageCopiedHeaderNotFound",
   0x40000012 : "MMC_VDDSD1RDYNotAsserted",
   0x40000013 : "MMC_VDDSD2RDYNotAsserted",
   0xCC000001 : "HASHDRV_Timeout",
   0xCC000002 : "HASHDRV_BadState",
   0xCC000003 : "HASHDRV_AccumulateSizeNotMultOf4",
   0xCC000040 : "HASHDRV_TestPass",
   0xCC000041 : "HASHDRV_TestAccumulateError",
   0xCC000042 : "HASHDRV_TestComputeError",
   0xCC000043 : "HASHDRV_TestFinishError",
   0xCC000044 : "HASHDRV_TestCompareError",
   0x80000001 : "QSPI_TryingSECBL1",
   0x80000002 : "QSPI_TryingSECBL2",
   0x80000003 : "QSPI_TryingSingleMode",
   0x80000004 : "QSPI_TryingDualMode",
   0x80000005 : "QSPI_FoundGpt",
   0x80000006 : "QSPI_FoundMagicAtDefaultLocation",
   0x80000007 : "QSPI_UseOffsets",
   0x80000008 : "QSPI_WrongImageLen",
   0x80000009 : "QSPI_Timeout",
   0x0000000A : "BOOTCORE_HwResetPOR",
   0x0000000B : "BOOTCORE_HwResetBOR",
   0x0000000C : "BOOTCORE_HwResetPadNrst",
   0x0000000D : "BOOTCORE_HwResetHcssRst",
   0x0000000E : "BOOTCORE_HwResetVCore",
   0x0000000F : "BOOTCORE_HwResetMpSysRst",
   0x00000010 : "BOOTCORE_HwResetIwdg1",
   0x00000011 : "BOOTCORE_HwResetIwdg2",
   0x00000012 : "BOOTCORE_HwResetStdby",
   0x00000013 : "BOOTCORE_HwResetMpup0Rst",
   0x0000001B : "BOOTCORE_LogicalResetRmaReq",
   0x0000001C : "BOOTCORE_LogicalResetLocalMpuCore0Reset",
   0x0000001D : "BOOTCORE_LogicalResetStdbyExit",
   0x0000001E : "BOOTCORE_LogicalResetSystem",
   0x00000025 : "BOOTCORE_ChipModeSecOpen",
   0x00000026 : "BOOTCORE_ChipModeSecOpenEngiBoot",
   0x00000027 : "BOOTCORE_ChipModeSecClosed",
   0x00000028 : "BOOTCORE_ChipModeSecClosedRmaOngoing",
   0x0000002F : "BOOTCORE_OtpModeOtpSecured",
   0x00000035 : "BOOTCORE_BootActionLocalMpuCore0ResetProcess",
   0x00000038 : "BOOTCORE_BootActionRmaReqProcess",
   0x00000039 : "BOOTCORE_BootActionRmaProcess",
   0x0000003A : "BOOTCORE_BootActionSecureBootProcess",
   0x0000003B : "BOOTCORE_BootActionWakeupStdby",
   0x0000003D : "BOOTCORE_BootActionEngiBoot",
   0x00000043 : "BOOTCORE_DebugCfgExitValue",
   0x00000044 : "BOOTCORE_BsecJtagInValue",
   0x00000045 : "BOOTCORE_BootCfgOtpWordValue",
   0x00000046 : "BOOTCORE_BootCfgAfmuxOtpWord1Value",
   0x00000047 : "BOOTCORE_BootCfgAfmuxOtpWord2Value",
   0x00000048 : "BOOTCORE_BootCfgAfmuxOtpWord3Value",
   0x00000049 : "BOOTCORE_BootCfgHseValue",
   0x00000051 : "BOOTCORE_NoUsbDpPullupOtpBitValue",
   0x00000052 : "BOOTCORE_NoCpuPllOtpBitValue",
   0x00000057 : "BOOTCORE_BootCfgFsblDecryptPrio",
   0x00000061 : "BOOTCORE_ReOpenDebugValue",
   0x00000062 : "BOOTCORE_ReOpenSecNSecDebugValue",
   0x00000063 : "BOOTCORE_ReOpenSecOnlyDebugValue",
   0x00000064 : "BOOTCORE_ReOpenDbgSwEnableAndHdp",
   0x00000065 : "BOOTCORE_DebugCloseAllButDbgenAndDeviceen",
   0x0000006E : "BOOTCORE_RmaRequestValue",
   0x0000006F : "BOOTCORE_RmaStatusValue",
   0x00000074 : "BOOTCORE_NoDataCacheBitValue",
   0x00000075 : "BOOTCORE_WakeupFromCSTANDBY",
   0x00000076 : "BOOTCORE_WakeupFromSTANDBY",
   0x00000079 : "BOOTCORE_WkUpActiveFlags",
   0x00000091 : "BOOTCORE_MpuCpuPowerStateHasBeenInCStandby",
   0x00000092 : "BOOTCORE_SystemPowerStateHasBeenInStandby",
   0x00000093 : "BOOTCORE_BootModeNOBOOTENGIBOOT",
   0x00000094 : "BOOTCORE_BootModeNOBOOTNOTENGIBOOTREQBYBOOTPINS",
   0x00000095 : "BOOTCORE_BootModeNOBOOT",
   0x00000096 : "BOOTCORE_BootModeSECUREBOOT",
   0x00000097 : "BOOTCORE_EndBootROMGPIONotification",
   0x00000098 : "BOOTCORE_NOBOOTLoop",
   0x000000BF : "BOOTCORE_BootTimerSrcSoftCnt",
   0x000000C0 : "BOOTCORE_BootTimerSrcGenTimer",
   0x000000CF : "BOOTCORE_OtpBootSrcDisableMaskVal",
   0x000000D0 : "BOOTCORE_OtpBootUartInstanceDisableMaskVal",
   0x000000D1 : "BOOTCORE_EnabledSrcMaskVal",
   0x000000E8 : "BOOTCORE_SafMemPwrUp",
   0x000000E9 : "BOOTCORE_SafMemPwrDown",
   0x000000EC : "BOOTCORE_EndBootGPIOEngiTesterStarted",
   0x000000ED : "BOOTCORE_BootFAILED",
   0x00000100 : "BOOTCORE_ARM_EXCEPTION_UNDEF",
   0x00000101 : "BOOTCORE_ARM_EXCEPTION_PREFETCH_ABORT",
   0x00000102 : "BOOTCORE_ARM_EXCEPTION_DATA_ABORT",
   0x00000103 : "BOOTCORE_ARM_EXCEPTION_HYP",
   0x00000104 : "BOOTCORE_ARM_EXCEPTION_IRQ",
   0x00000105 : "BOOTCORE_ARM_EXCEPTION_FIQ",
   0x00000106 : "BOOTCORE_ARM_EXCEPTION_SVC",
   0x00000107 : "BOOTCORE_ARM_EXCEPTION_SMC_FROM_SECURE_WORLD_ERROR",
   0x00000120 : "BOOTCORE_UartTraceEntryOnHwInstance",
   0x00000130 : "BOOTCORE_Pll12StartBegin",
   0x00000131 : "BOOTCORE_Pll12StartEnd",
   0x00000132 : "BOOTCORE_Pll12StartDisabledByOtpBit",
   0x00000133 : "BOOTCORE_Pll12StartNotDisabledByOtpBit",
   0x00000134 : "BOOTCORE_Pll1Started",
   0x00000135 : "BOOTCORE_Pll1Locked",
   0x00000136 : "BOOTCORE_Pll1LockTimeout",
   0x00000137 : "BOOTCORE_CkMpuSsSwitchedOnPll1",
   0x00000138 : "BOOTCORE_Pll2Started",
   0x00000139 : "BOOTCORE_Pll2Locked",
   0x0000013A : "BOOTCORE_Pll2LockTimeout",
   0x0000013B : "BOOTCORE_CkAxiSsSwitchedOnPll2",
   0x0000013C : "BOOTCORE_Pll12StartReqStatusPllNotStarted",
   0x0000013D : "BOOTCORE_Pll12StartReqStatusPllStarted",
   0x0000013E : "BOOTCORE_Pll12StartDisabledByCompilationDirective",
   0x00000140 : "BOOTCORE_BootSrcPeriphLoadLoopDisabled",
   0x0000014E : "BOOTCORE_Pll4Started",
   0x0000014F : "BOOTCORE_Pll4Locked",
   0x00000150 : "BOOTCORE_Pll4LockTimeout",
   0x00000151 : "BOOTCORE_HseNotReady",
   0x00000152 : "BOOTCORE_StartupWaitTime",
   0x00000153 : "BOOTCORE_PllUsbLocked",
   0x00000154 : "BOOTCORE_PllUsbLockTimeout",
   0x00000155 : "BOOTCORE_BootRomVer",
   0x00000156 : "BOOTCORE_BootRomChipVer",
   0x00000157 : "BOOTCORE_BootRomCutVer",
   0x00000158 : "BOOTCORE_BootRomForRtlVer",
   0x00000159 : "BOOTCORE_BootRomTargetPlatform",
   0x0000015A : "BOOTCORE_BootRomMaskVer",
   0x0000015D : "BOOTCORE_McuSecPerimeterRccTzenValue",
   0x00000160 : "BOOTCORE_BootModeNOBOOTWithSignalingFromNsec",
   0x00000161 : "BOOTCORE_NoBootTogglePa13Nsec",
   0x000001C0 : "BOOTCORE_OtpEcdsaKeyCounterMonotonicVal",
   0x000001C1 : "BOOTCORE_EcdsaKeyCounterVal",
   0x000001C2 : "BOOTCORE_EcdsaKeyCounterNewProgVal",
   0x000001D0 : "BOOTCORE_ImageVersionCounterNewProgVal",
   0x000001D1 : "BOOTCORE_ImageVersionTooBig",
   0x00000200 : "BOOTCORE_ConfigureSRAMsForBlockingFailure",
   0x00000201 : "BOOTCORE_ConfigureSRAMsForDownload",
   0x00000202 : "BOOTCORE_ConfigureSRAMsSecure",
   0x00000203 : "BOOTCORE_ClearDownloadAreas",
   0x00001000 : "BOOTCORE_BootPinsFirstSelNone",
   0x00001001 : "BOOTCORE_BootPinsFirstSelSerial",
   0x00001002 : "BOOTCORE_BootPinsFirstSelQspiNor",
   0x00001003 : "BOOTCORE_BootPinsFirstSelSd",
   0x00001004 : "BOOTCORE_BootPinsFirstSelEmmc",
   0x00001005 : "BOOTCORE_BootPinsFirstSelFmc",
   0x00001006 : "BOOTCORE_BootPinsFirstSelEngiBoot",
   0x00001007 : "BOOTCORE_BootPinsFirstSelUnknown",
   0x00001008 : "BOOTCORE_BootPinsOverridesOtp",
   0x00001017 : "BOOTCORE_BootPinsFirstSelQspiNand",
   0x00001009 : "BOOTCORE_OtpPrimarySrcForceFmc",
   0x0000100A : "BOOTCORE_OtpPrimarySrcForceQspiNor",
   0x0000100B : "BOOTCORE_OtpPrimarySrcForceEmmc",
   0x0000100C : "BOOTCORE_OtpPrimarySrcForceSd",
   0x0000100D : "BOOTCORE_OtpPrimarySrcForceQspiNand",
   0x0000100E : "BOOTCORE_OtpPrimarySrcForceNothing",
   0x0000100F : "BOOTCORE_OtpPrimarySrcForceUnknown",
   0x00001010 : "BOOTCORE_OtpSecondarySrcForceFmc",
   0x00001011 : "BOOTCORE_OtpSecondarySrcForceQspiNand",
   0x00001012 : "BOOTCORE_OtpSecondarySrcForceQspiNor",
   0x00001013 : "BOOTCORE_OtpSecondarySrcForceEmmc",
   0x00001014 : "BOOTCORE_OtpSecondarySrcForceSd",
   0x00001015 : "BOOTCORE_OtpSecondarySrcForceNothing",
   0x00001016 : "BOOTCORE_OtpSecondarySrcForceUnknown",
   0x00001020 : "BOOTCORE_TampForceSerialBoot",
   0x00001030 : "BOOTCORE_BootPinsValueRead",
   0x00002001 : "BOOTCORE_AuthWrongImageVersion",
   0x00002002 : "BOOTCORE_AuthHashAreaStartAdd",
   0x00002003 : "BOOTCORE_AuthImageLength",
   0x00002004 : "BOOTCORE_AuthImageSizeToHash",
   0x00002005 : "BOOTCORE_AuthImageEntryPoint",
   0x00002006 : "BOOTCORE_AuthEccAlgoP256NIST",
   0x00002007 : "BOOTCORE_AuthEccAlgoBrainPool256",
   0x00002008 : "BOOTCORE_AuthEccAlgoUnknown",
   0x00002009 : "BOOTCORE_AuthSignCheckValue",
   0x0000200A : "BOOTCORE_AuthEntryPointOutOfRange",
   0x0000200B : "BOOTCORE_AuthPubKHashFailStart",
   0x0000200C : "BOOTCORE_AuthPubKHashFailFinish",
   0x0000200D : "BOOTCORE_AuthPubKHashCompareError",
   0x0000200E : "BOOTCORE_AuthPubKHashCompareOk",
   0x0000200F : "BOOTCORE_AuthImageHashFailStart",
   0x00002010 : "BOOTCORE_AuthImageHashFailFinish",
   0x00002011 : "BOOTCORE_AuthImageSignatureFromHeaderECCDecryptStarted",
   0x00002012 : "BOOTCORE_AuthImageSignatureKo",
   0x00002013 : "BOOTCORE_AuthImageSignatureOk",
   0x00002014 : "BOOTCORE_AuthDecisionIsJumpToImage",
   0x00002015 : "BOOTCORE_AuthWrongImageHeaderVersion",
   0x00002016 : "BOOTCORE_AuthWrongMagicNumber",
   0x00002017 : "BOOTCORE_AuthImageLengthTooSmall",
   0x00002018 : "BOOTCORE_AuthImageLengthTooBig",
   0x00002019 : "BOOTCORE_AuthWrongImageChecksum",
   0x0000201A : "BOOTCORE_AuthIsAuthenticationMandatory",
   0x0000201B : "BOOTCORE_AuthIsAuthenticationSkipRequestedAndAllowed",
   0x00002020 : "BOOTCORE_AuthImageEncrypted",
   0x00002021 : "BOOTCORE_AuthImageNotEncrypted",
   0x00002022 : "BOOTCORE_AuthImageDecryptionSucceed",
   0x00002023 : "BOOTCORE_AuthImageDecryptionFailed",
   0x00002024 : "BOOTCORE_AuthCryptoDisabledError",
   0x00002025 : "BOOTCORE_AuthenticationExtensionHeaderMissing",
   0x00002026 : "BOOTCORE_AuthMemoryVerifError",
   0x00002040 : "BOOTCORE_AuthAuthenticationHeaderNotFound",
   0x00002041 : "BOOTCORE_AuthPubKHashTableNotEqualToRef",
   0x00002042 : "BOOTCORE_AuthPubKHashTableCheckSpecialSspStage3",
   0x00002043 : "BOOTCORE_AuthInvalidKeyIndex",
   0x00002044 : "BOOTCORE_AuthPublicKeyIsRevoked",
   0x00002045 : "BOOTCORE_AuthPublicKeyNotRevoked",
   0x00002046 : "BOOTCORE_AuthPublicKeyHashVerificationError",
   0x00002047 : "BOOTCORE_AuthPubKeyVerifFailed",
   0x00002048 : "BOOTCORE_AuthPubKeyVerifOk",
   0x00002060 : "BOOTCORE_AuthFuseEcdsaKeyCounterRequired",
   0x00002061 : "BOOTCORE_AuthFuseEcdsaKeyCounterNotRequired",
   0x00002062 : "BOOTCORE_VerifyErrorWhileProgrammingEcdsaKeyCounter",
   0x00002063 : "BOOTCORE_AuthNewEcdsaKeyCounterProgSuccess",
   0x00002064 : "BOOTCORE_AuthMaxEcdsaKeyCounterReachedRevokNotPossible",
   0x00002080 : "BOOTCORE_DecryptOptionHeaderNotFound",
   0x00002081 : "BOOTCORE_DecryptWrongKeySize",
   0x00002082 : "BOOTCORE_DecryptDerivKeyCalcError",
   0x00002083 : "BOOTCORE_DecryptImageDecryptError",
   0x00002084 : "BOOTCORE_DecryptHashIntegrityError",
   0x00002085 : "BOOTCORE_DecryptOtpSecretKeyReadError",
   0x00002086 : "BOOTCORE_DecryptUsingCrypDriver",
   0x00002087 : "BOOTCORE_DecryptUsingSaesDriver",
   0x00002088 : "BOOTCORE_DecryptOtpAesDriverError",
   0x00002089 : "BOOTCORE_DerivKey128UsingSaesDriver",
   0x0000208A : "BOOTCORE_DerivKey256UsingSaesDriver",
   0x0000208B : "BOOTCORE_DerivKey128UsingCrypDriver",
   0x0000208C : "BOOTCORE_DerivKey256UsingCrypDriver",
   0x00002400 : "BOOTCORE_HseFrequencyDetected",
   0x00002401 : "BOOTCORE_HseDigBypassDetected",
   0x00002402 : "BOOTCORE_HseAnalogBypassDetected",
   0x00002403 : "BOOTCORE_HseNoBypass",
   0x00002404 : "BOOTCORE_HseFreqDetectDisabled",
   0x00002405 : "BOOTCORE_HseBypassDetectDisabled",
   0x00002406 : "BOOTCORE_eMmcUseSingle128KBytesBootPartition",
   0x00002410 : "BOOTCORE_WkAroundEXTIResetValues",
   0x00002503 : "BOOTCORE_FingerPrintOverflow",
   0x00002504 : "BOOTCORE_FingerPrintValue",
   0x00002505 : "BOOTCORE_FingerPrintCoef",
   0x50000000 : "USB_Init",
   0x50000001 : "USB_Loop",
   0x50000002 : "USB_Selection",
   0x50000003 : "USB_CallingAuthAndGo",
   0x50000004 : "USB_DFU_DownloadComplete",
   0x50000005 : "USB_DFU_GetStatus",
   0x50000006 : "USB_DFU_GetStatus_GoTo_Idle",
   0x50000007 : "USB_DFU_END_OF_DNLOAD",
   0x50000008 : "USB_DFU_DNLOAD",
   0x50000009 : "USB_DFU_UPLOAD",
   0x5000000A : "USB_DFU_GETSTATUS",
   0x5000000B : "USB_DFU_CLRSTATUS",
   0x5000000C : "USB_DFU_GETSTATE",
   0x5000000D : "USB_DFU_ABORT",
   0x5000000E : "USB_DFU_DETACH",
   0x5000000F : "USB_DFU_LEAVE",
   0x50000010 : "USB_DFU_DataOut",
   0x50000011 : "USB_DFU_GetStatusMedia",
   0x50000012 : "USB_DFU_SRAMWrite",
   0x50000013 : "USB_DFU_SRAMWrite_CRC",
   0x50000014 : "USB_DFU_SRAMWrite_imgL",
   0x50000015 : "USB_DFU_Init",
   0x50000016 : "USB_DFU_SET_INTERFACE",
   0x50000017 : "USB_DFU_SET_INTERFACE_ERR",
   0x50000018 : "USB_DFU_Get_INTERFACE",
   0x50000019 : "USB_DFU_LEAVE_EP0_TxReady",
   0x50000020 : "USB_ClockTree_Failed",
   0x50000021 : "USB_WrongDataTransfer",
   0x50000022 : "USB_DataTransfered",
   0x50000030 : "USB_DFU_DetachComplete",
   0x60000001 : "UART_Selection",
   0x60000002 : "UART_PeripheralSerialBootLoopStart",
   0x60000003 : "UART_PacketNumber",
   0x60000004 : "UART_PacketSize",
   0x60000005 : "UART_PacketXor",
   0x60000006 : "UART_ImageLength",
   0x60000007 : "UART_Cursor",
   0x60000008 : "UART_NACK",
   0x60000009 : "UART_Abort",
   0x6000000A : "UART_ActivityDetected",
   0x6000000B : "UART_ActivityDetecting",
   0x6000000C : "UART_ReceivedByte",
   0x6000000D : "UART_DownloadFinished",
   0x6000000E : "UART_Start",
   0x30000001 : "SD_InitFailed",
   0x30000002 : "SD_TransferFailed",
   0x30000003 : "SD_CardDetected",
   0x30000004 : "SD_NoCardResponse",
   0x30000005 : "SD_TrySecBL1",
   0x30000006 : "SD_FailedSecBL1TrySecBL2",
   0x30000007 : "SD_InitDetailedErrorDesc",
   0x30000008 : "SD_ImageTooBig",
   0x30000009 : "SD_ImageLengthZero",
   0x3000000A : "SD_StdCapacityCardV11",
   0x3000000B : "SD_StdCapacityCardV20",
   0x3000000C : "SD_HighCapacityCardSDHCOrSDXC",
   0x3000000D : "SD_UnknownCapacityCard",
   0x3000000E : "SD_OverallRetryCnt",
   0x3000000F : "SD_ImageHeaderNotFound",
   0x30000010 : "SD_NotGPTFound",
   0x30000011 : "SD_FsblsFound",
   0x30000012 : "SD_GPTFound",
   0x30000013 : "SD_NotFSBLFound",
   0x30000014 : "SD_RamCopyAddr",
   0x30000015 : "SD_BufLen",
   0x30000016 : "SD_ReadAddrInCard",
   0x30000017 : "SD_NbFullBlocks",
   0x30000018 : "SD_BytesRemainingInLastBlock",
   0x30000019 : "SD_InFullBlocksRead",
   0x3000001A : "SD_InLastIncompleteBlockRead",
   0x30000020 : "SD_VDDSD1RDYNotAsserted",
   0x30000021 : "SD_VDDSD2RDYNotAsserted",
   0xC4000002 : "DWNLDMGR_WrongMagicNumber",
   0xC4000003 : "DWNLDMGR_ImageLengthTooSmall",
   0xC4000004 : "DWNLDMGR_ImageLengthTooBig",
   0xC4000005 : "DWNLDMGR_EntryPointOutOfRange",
   0xC4000006 : "DWNLDMGR_WrongImageChecksum",
   0xC4000007 : "DWNLDMGR_WrongHeaderVersion",
   0xC4000008 : "DWNLDMGR_UnknownExtensionHeader",
   0xC4000009 : "DWNLDMGR_FoundFsblDecryptionHeader",
   0xC400000A : "DWNLDMGR_FoundPkhAuthenticationHeader",
   0xC400000B : "DWNLDMGR_WrongExtensionHeaderLength",
   0xC400000C : "DWNLDMGR_WrongCipherKeySize",
   0xC400000D : "DWNLDMGR_ExtensionFlagsMismatch",
   0xC400000E : "DWNLDMGR_WrongNumberOfPublicKeyHash",
   0xC400000F : "DWNLDMGR_WrongPublicKeyIndex",
   0xC4000010 : "DWNLDMGR_FoundPaddingHeader",

}


def tag2str(tag):
    try:
       return tags[tag]
    except KeyError:
       return None


class Trace(object):

    def __init__(self, timestamp, level, tag, data ):
        self.timestamp = timestamp
        self.level = level
        self.tag = tag
        self.data = data

    def dump(self):
        level_str = level2str(self.level)
        tag_str = tag2str(self.tag)

        if tag_str: 
           print( "< @ %08d | [%s] - %s" % (self.timestamp, level_str, tag_str), end='' )

           if len(self.data) > 0:
              print (" (", end='')
              for d in self.data[:-1]:
                 print (" 0x%08X," % (d), end='')
              print (" 0x%08X )" % (self.data[-1]), end='')

           print (" >")


class TraceAnalyzer(object):
  
    START_WORD = 0xFFDDBB00

    def __init__(self):
       pass

    def _next_word_(self):
        word = self._file.read(4)
        if len(word):
           return struct.unpack('<L',word)[0]
        else:
           raise StopIteration('END OF FILE')

    def _look_for_start_word_(self):
       while True:
          if self._next_word_() == self.START_WORD:
             return True

    def parse(self,file_):
        self._file = file_
        return iter(self)

    def __iter__(self):
        return self

    def __next__(self):
        self._look_for_start_word_()

        size = self._next_word_()
        timestamp = self._next_word_()
        size -= 4
        level = self._next_word_()
        size -= 4
        tag = self._next_word_()
        size -= 4

        data = []
        while size > 0:
            data.append(self._next_word_())
            size -= 4

        return Trace(timestamp, level, tag, data)

           



if __name__ == '__main__':
    
    parser = argparse.ArgumentParser(description='STM32MPU15 Bootrom trace decoder')
    parser.add_argument("trace_file",  
          action="store",
          type=argparse.FileType('rb'),
          help="Name of binary trace file",
          metavar="FILE.BIN")
    
    args = parser.parse_args()

    traces_analyzer = TraceAnalyzer()
    traces = traces_analyzer.parse(args.trace_file)
    for trace in traces:
      trace.dump()
