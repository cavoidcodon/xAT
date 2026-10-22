# AUTOSAR compliance matrix (R21-11)

One section per module. Every SWS requirement that applies is listed with its status.
Project-wide deviations are listed in `.claude/rules/coding_rules.md` §10.

Status values: `Implemented`, `Partial`, `Not applicable`, `Deviation`, `Not implemented`.

<!--
## <Ma> (AUTOSAR_SWS_<...>)

| Requirement | Summary | Status | Code | Test |
|---|---|---|---|---|
| SWS_<Ma>_xxxxx | ... | Implemented | `src/.../<Ma>.c:<Function>` | `test_<Ma>.c:test_...` |
-->

## Platform (AUTOSAR_SWS_PlatformTypes, document 48)

Code: `src/platform/include/Platform_Types.h`. Tests: `tests/unit/platform/test_Platform_Types.c`
(host, LP64) and the `_Static_assert` block in `src/ecu/main.c` (target, ILP32, checked at
every firmware build). SWS_Platform_00063 was removed from the SWS and is not listed.

| Requirement | Summary | Status | Code | Test |
|---|---|---|---|---|
| SWS_Platform_00002 | all platform types and symbols in `Platform_Types.h`, no extension | Implemented | `Platform_Types.h` | all tests of `test_Platform_Types.c` |
| SWS_Platform_00013 | `uint8` 8-bit unsigned | Implemented | `Platform_Types.h:uint8` | `test_Uint8_WidthAndRange_Is8BitUnsigned`, `main.c` |
| SWS_Platform_00014 | `uint16` 16-bit unsigned | Implemented | `Platform_Types.h:uint16` | `test_Uint16_WidthAndRange_Is16BitUnsigned`, `main.c` |
| SWS_Platform_00015 | `uint32` 32-bit unsigned | Implemented | `Platform_Types.h:uint32` | `test_Uint32_WidthAndRange_Is32BitUnsigned`, `main.c` |
| SWS_Platform_00066 | `uint64` 64-bit unsigned | Implemented | `Platform_Types.h:uint64` | `test_Uint64_WidthAndRange_Is64BitUnsigned`, `main.c` |
| SWS_Platform_00016 | `sint8` 8-bit signed | Implemented | `Platform_Types.h:sint8` | `test_Sint8_WidthAndRange_Is8BitSigned`, `main.c` |
| SWS_Platform_00017 | `sint16` 16-bit signed | Implemented | `Platform_Types.h:sint16` | `test_Sint16_WidthAndRange_Is16BitSigned`, `main.c` |
| SWS_Platform_00018 | `sint32` 32-bit signed | Implemented | `Platform_Types.h:sint32` | `test_Sint32_WidthAndRange_Is32BitSigned`, `main.c` |
| SWS_Platform_00067 | `sint64` 64-bit signed | Implemented | `Platform_Types.h:sint64` | `test_Sint64_WidthAndRange_Is64BitSigned`, `main.c` |
| SWS_Platform_00061 | signed types are two's complement | Implemented | `Platform_Types.h` (C11 `intN_t`) | `test_SignedTypes_Representation_IsTwosComplement` |
| SWS_Platform_00020 | `uint8_least` at least 8-bit unsigned | Implemented | `Platform_Types.h:uint8_least` (32 bit) | `test_UnsignedLeastTypes_Width_AtLeastRequiredAnd32Bit` |
| SWS_Platform_00021 | `uint16_least` at least 16-bit unsigned | Implemented | `Platform_Types.h:uint16_least` (32 bit) | `test_UnsignedLeastTypes_Width_AtLeastRequiredAnd32Bit` |
| SWS_Platform_00022 | `uint32_least` at least 32-bit unsigned | Implemented | `Platform_Types.h:uint32_least` | `test_UnsignedLeastTypes_Width_AtLeastRequiredAnd32Bit` |
| SWS_Platform_00023 | `sint8_least` at least 8-bit signed | Implemented | `Platform_Types.h:sint8_least` (32 bit) | `test_SignedLeastTypes_Width_AtLeastRequiredAnd32Bit` |
| SWS_Platform_00024 | `sint16_least` at least 16-bit signed | Implemented | `Platform_Types.h:sint16_least` (32 bit) | `test_SignedLeastTypes_Width_AtLeastRequiredAnd32Bit` |
| SWS_Platform_00025 | `sint32_least` at least 32-bit signed | Implemented | `Platform_Types.h:sint32_least` | `test_SignedLeastTypes_Width_AtLeastRequiredAnd32Bit` |
| SWS_Platform_00026 | `boolean`, values `TRUE` / `FALSE` | Implemented | `Platform_Types.h:boolean` | `test_Boolean_SizeAndValues_8BitUnsignedTrueOneFalseZero` |
| SWS_Platform_00027 | `boolean` is the shortest native unsigned type (8 bit) | Implemented | `Platform_Types.h:boolean` | `test_Boolean_SizeAndValues_8BitUnsignedTrueOneFalseZero`, `main.c` |
| SWS_Platform_00060 | `boolean` can be pointed to | Implemented | `Platform_Types.h:boolean` (`uint8_t`) | `test_Boolean_PassedByPointer_KeepsTrueAndFalse` |
| SWS_Platform_00034 | `boolean` used only with `TRUE` / `FALSE` and the listed operators | Not applicable | usage rule, `coding_rules.md` 3.3 | `test_Boolean_PassedByPointer_KeepsTrueAndFalse` (allowed forms) |
| SWS_Platform_00055 | `TRUE` / `FALSE` used only with `boolean` | Not applicable | usage rule, `coding_rules.md` 3.3 | — |
| SWS_Platform_00056 | `TRUE` = 1, `FALSE` = 0 with `#ifndef` | Implemented | `Platform_Types.h:TRUE`, `FALSE` | `test_Boolean_SizeAndValues_8BitUnsignedTrueOneFalseZero` |
| SWS_Platform_00054 | no redefinition of in-built `TRUE` / `FALSE` | Implemented | `Platform_Types.h` (`#ifndef TRUE`) | `compile_checks/check_TrueFalsePredefined.c` |
| SWS_Platform_00041 | `float32` IEEE 754 binary32 | Implemented | `Platform_Types.h:float32` | `test_Float32_Format_IsIeee754Binary32` |
| SWS_Platform_00042 | `float64` IEEE 754 binary64 | Implemented | `Platform_Types.h:float64` | `test_Float64_Format_IsIeee754Binary64` |
| SWS_Platform_91001 | `VoidPtr` = `void *` | Implemented | `Platform_Types.h:VoidPtr` | `test_VoidPtrTypes_Definitions_PointToVoidAndConstVoid` |
| SWS_Platform_91002 | `ConstVoidPtr` = `const void *` | Implemented | `Platform_Types.h:ConstVoidPtr` | `test_VoidPtrTypes_Definitions_PointToVoidAndConstVoid` |
| SWS_Platform_00064 | `CPU_TYPE` symbol and its values | Implemented | `Platform_Types.h:CPU_TYPE_8..64` | `test_CpuSymbolValues_Definitions_MatchSwsAnnex` |
| SWS_Platform_00044 | `CPU_TYPE` defined for the platform | Implemented | `Platform_Types.h:CPU_TYPE` | `test_CpuSymbols_Mcxn947_32BitLsbFirstLowByteFirst` |
| SWS_Platform_00045 | `CPU_TYPE` = register width (`CPU_TYPE_32`) | Implemented | `Platform_Types.h:CPU_TYPE` | `test_CpuSymbols_Mcxn947_32BitLsbFirstLowByteFirst`, `main.c` |
| SWS_Platform_00038 | `CPU_BIT_ORDER` symbol and its values | Implemented | `Platform_Types.h:MSB_FIRST`, `LSB_FIRST` | `test_CpuSymbolValues_Definitions_MatchSwsAnnex` |
| SWS_Platform_00043 | `CPU_BIT_ORDER` defined for the platform | Implemented | `Platform_Types.h:CPU_BIT_ORDER` | `test_CpuSymbols_Mcxn947_32BitLsbFirstLowByteFirst` |
| SWS_Platform_00049 | little endian bit order -> `LSB_FIRST` | Implemented | `Platform_Types.h:CPU_BIT_ORDER` | `test_CpuSymbols_Mcxn947_32BitLsbFirstLowByteFirst` |
| SWS_Platform_00048 | big endian bit order -> `MSB_FIRST` | Not applicable | MCXN947 is little endian | — |
| SWS_Platform_00039 | `CPU_BYTE_ORDER` symbol and its values | Implemented | `Platform_Types.h:HIGH_BYTE_FIRST`, `LOW_BYTE_FIRST` | `test_CpuSymbolValues_Definitions_MatchSwsAnnex` |
| SWS_Platform_00046 | `CPU_BYTE_ORDER` defined for the platform | Implemented | `Platform_Types.h:CPU_BYTE_ORDER` | `test_CpuSymbols_Mcxn947_32BitLsbFirstLowByteFirst`, `test_CpuByteOrder_HostMemory_LowByteFirst` |
| SWS_Platform_00051 | little endian byte order -> `LOW_BYTE_FIRST` | Implemented | `Platform_Types.h:CPU_BYTE_ORDER` | `test_CpuSymbols_Mcxn947_32BitLsbFirstLowByteFirst`, `main.c` |
| SWS_Platform_00050 | big endian byte order -> `HIGH_BYTE_FIRST` | Not applicable | MCXN947 is little endian | — |

## Std (AUTOSAR_SWS_StandardTypes, document 49)

Code: `src/platform/include/Std_Types.h` (and `Compiler.h` for `NULL_PTR`). Tests:
`tests/unit/platform/test_Std_Types.c`, compile checks in `tests/unit/platform/`.

| Requirement | Summary | Status | Code | Test |
|---|---|---|---|---|
| SWS_Std_00004 | no project or supplier extension in `Std_Types.h` | Implemented | `Std_Types.h` | review |
| SWS_Std_00014 | include guard `STD_TYPES_H` | Implemented | `Std_Types.h` | `compile_check_platform` (`check_Std_Types.c`) |
| SWS_Std_00005 | `Std_ReturnType` = `uint8` | Implemented | `Std_Types.h:Std_ReturnType` | `test_StdReturnType_Definition_IsUint8` |
| SWS_Std_00006 | `E_OK` 0, `E_NOT_OK` 1, `STATUSTYPEDEFINED` guard | Implemented | `Std_Types.h:E_OK`, `E_NOT_OK` | `test_StdReturnValues_Definitions_EOkZeroENotOkOne`, `compile_checks/check_StatusTypePredefined.c` |
| SWS_Std_00011 | user specific return values in the 6 LSB | Not applicable | usage rule, documented at `Std_ReturnType` | — |
| SWS_Std_00007 | `STD_HIGH` 1, `STD_LOW` 0 | Implemented | `Std_Types.h:STD_HIGH`, `STD_LOW` | `test_StdHighLow_Definitions_HighOneLowZero` |
| SWS_Std_00013 | `STD_ACTIVE` 1, `STD_IDLE` 0 | Implemented | `Std_Types.h:STD_ACTIVE`, `STD_IDLE` | `test_StdActiveIdle_Definitions_ActiveOneIdleZero` |
| SWS_Std_00010 | `STD_ON` 1, `STD_OFF` 0 | Implemented | `Std_Types.h:STD_ON`, `STD_OFF` | `test_StdOnOff_Definitions_OnOneOffZero` |
| SWS_Std_00015 | `Std_VersionInfoType` members | Implemented | `Std_Types.h:Std_VersionInfoType` | `test_StdVersionInfoType_Members_NamesAndTypesMatchSws` |
| SWS_Std_00021 | `Std_TransformerError` | Implemented | `Std_Types.h:Std_TransformerError` | `test_TransformerTypes_Definitions_Uint8BasedWithSwsClasses` |
| SWS_Std_00022 | `Std_TransformerErrorCode` = `uint8` | Implemented | `Std_Types.h:Std_TransformerErrorCode` | `test_TransformerTypes_Definitions_Uint8BasedWithSwsClasses` |
| SWS_Std_00023 | `Std_TransformerClass` based on `uint8` | Implemented | `Std_Types.h:Std_TransformerClass` | `test_TransformerTypes_Definitions_Uint8BasedWithSwsClasses` |
| SWS_Std_00024 | transformer class values | Implemented | `Std_Types.h:STD_TRANSFORMER_*` | `test_TransformerTypes_Definitions_Uint8BasedWithSwsClasses` |
| SWS_Std_00025 | `STD_TRANSFORMER_UNSPECIFIED` when no error | Implemented | `Std_Types.h:STD_TRANSFORMER_UNSPECIFIED` | `test_TransformerTypes_Definitions_Uint8BasedWithSwsClasses` |
| SWS_Std_00026 | mapping of transformer classes | Implemented | `Std_Types.h:STD_TRANSFORMER_*` | `test_TransformerTypes_Definitions_Uint8BasedWithSwsClasses` |
| SWS_Std_00027 | `Std_TransformerForward` (draft) | Not implemented | draft in R21-11 | — |
| SWS_Std_00028 | safety transformer forward codes (draft) | Not implemented | draft in R21-11 | — |
| SWS_Std_00029 | `Std_TransformerForwardCode` (draft) | Not implemented | draft in R21-11 | — |
| SWS_Std_91001 | `Std_MessageTypeType` and values | Implemented | `Std_Types.h:Std_MessageTypeType`, `STD_MESSAGETYPE_*` | `test_MessageTypes_Definitions_Uint8WithSwsValues` |
| SWS_Std_00017 | use of `Std_MessageTypeType` | Implemented | `Std_Types.h:Std_MessageTypeType` | `test_MessageTypes_Definitions_Uint8WithSwsValues` |
| SWS_Std_91002 | `Std_MessageResultType` and values | Implemented | `Std_Types.h:Std_MessageResultType`, `STD_MESSAGERESULT_*` | `test_MessageTypes_Definitions_Uint8WithSwsValues` |
| SWS_Std_00019 | use of `Std_MessageResultType` | Implemented | `Std_Types.h:Std_MessageResultType` | `test_MessageTypes_Definitions_Uint8WithSwsValues` |
| SWS_Std_91003 | `Std_ExtractProtocolHeaderFieldsType` | Implemented | `Std_Types.h:Std_ExtractProtocolHeaderFieldsType` | `test_ExtractProtocolHeaderFields_FunctionPointer_CallsParser` |
| SWS_Std_00031 | `NULL_PTR` = `((void *)0)` | Implemented | `Compiler.h:NULL_PTR` | `test_NullPtr_Definition_IsVoidPointerToZero` |
| SWS_Std_00030 | `ComStack_Types.h` includes `Std_Types.h` | Not implemented | communication stack task (CanIf) | — |

## Compiler (AUTOSAR_SWS_CompilerAbstraction, document 51)

Code: `src/platform/include/Compiler.h`, `Compiler_Cfg.h`. Tests:
`tests/unit/platform/test_Compiler.c` (GCC / aarch64 pair); the GCC / Cortex-M33 pair is
compiled by every firmware build (`src/ecu/main.c`).

| Requirement | Summary | Status | Code | Test |
|---|---|---|---|---|
| SWS_COMPILER_00003 | one compiler abstraction per compiler and platform | Implemented | `Compiler.h` (GCC / Cortex-M33, GCC / aarch64, else `#error`) | `test_CompilerSymbol_HostBuild_GccAarch64Only`, firmware build |
| SWS_COMPILER_00004 | file name `Compiler.h` | Implemented | `Compiler.h` | `compile_check_platform` (`check_Compiler.c`) |
| SWS_COMPILER_00010 | compiler symbol `_<COMPILER>_C_<PLATFORM>_` without value | Implemented | `Compiler.h:_GCC_C_CORTEXM33_`, `_GCC_C_AARCH64_` (MISRA 21.1 deviation) | `test_CompilerSymbol_HostBuild_GccAarch64Only` |
| SWS_COMPILER_00030 | supported compiler vendor and version | Implemented | `Compiler.h` (GCC >= 13 check) | `compile_check_platform` (host), firmware build (target) |
| SWS_COMPILER_00047 | include guard, only comments outside | Implemented | `Compiler.h` | `compile_check_platform` (`check_Compiler.c`) |
| SWS_COMPILER_00005 | macros without effect are empty | Implemented | `Compiler.h:AUTOMATIC`, `TYPEDEF` | `test_MemoryClasses_AutomaticAndTypedef_ExpandToEmpty` |
| SWS_COMPILER_00046 | `AUTOMATIC` empty | Implemented | `Compiler.h:AUTOMATIC` | `test_MemoryClasses_AutomaticAndTypedef_ExpandToEmpty` |
| SWS_COMPILER_00059 | `TYPEDEF` empty | Implemented | `Compiler.h:TYPEDEF` | `test_MemoryClasses_AutomaticAndTypedef_ExpandToEmpty` |
| SWS_COMPILER_00057 | `INLINE` | Implemented | `Compiler.h:INLINE` (`inline`) | `test_Inline_ExternFunction_ExpandsToInlineAndRuns` |
| SWS_COMPILER_00060 | `LOCAL_INLINE` | Implemented | `Compiler.h:LOCAL_INLINE` (`static inline`) | `test_LocalInline_HelperFunction_ExpandsToStaticInlineAndRuns` |
| SWS_COMPILER_00055 | `Compiler_Cfg.h` holds module memclass / ptrclass | Partial | `Compiler_Cfg.h` (exists, no classes: DEV-01) | `compile_check_platform` (`check_Compiler_Cfg.c`) |
| SWS_COMPILER_00053 | `Compiler.h` contains the macros of chapter 7.1.5 | Deviation | DEV-01 | — |
| SWS_COMPILER_00001 | `FUNC` | Deviation | DEV-01 | — |
| SWS_COMPILER_00061 | `FUNC_P2CONST` | Deviation | DEV-01 | — |
| SWS_COMPILER_00062 | no compiler keyword in `FUNC_P2CONST` parameters | Deviation | DEV-01 | — |
| SWS_COMPILER_00063 | `FUNC_P2VAR` | Deviation | DEV-01 | — |
| SWS_COMPILER_00064 | no compiler keyword in `FUNC_P2VAR` parameters | Deviation | DEV-01 | — |
| SWS_COMPILER_00058 | no compiler keyword in `FUNC` parameters | Deviation | DEV-01 | — |
| SWS_COMPILER_00006 | `P2VAR` | Deviation | DEV-01 | — |
| SWS_COMPILER_00013 | `P2CONST` | Deviation | DEV-01 | — |
| SWS_COMPILER_00031 | `CONSTP2VAR` | Deviation | DEV-01 | — |
| SWS_COMPILER_00032 | `CONSTP2CONST` | Deviation | DEV-01 | — |
| SWS_COMPILER_00039 | `P2FUNC` | Deviation | DEV-01 | — |
| SWS_COMPILER_00065 | `CONSTP2FUNC` | Deviation | DEV-01 | — |
| SWS_COMPILER_00023 | `CONST` | Deviation | DEV-01 | — |
| SWS_COMPILER_00026 | `VAR` | Deviation | DEV-01 | — |
| SWS_COMPILER_00035 | memclass / ptrclass take configured values only | Deviation | DEV-01 (no memclass / ptrclass) | — |
| SWS_COMPILER_00036 | `AUTOMATIC` as memclass of stack pointers | Deviation | DEV-01 | — |
| SWS_COMPILER_00040 | modules distinguish the memory / pointer classes | Deviation | DEV-01 | — |
| SWS_COMPILER_00041 | modules wrap declarations in the keyword macros | Deviation | DEV-01 | — |
| SWS_COMPILER_00048 | memclass / ptrclass fulfil each module's needs | Deviation | DEV-01 | — |
| SWS_COMPILER_00050 | vendor extensions carry the vendor ID | Not applicable | no extension | — |
| SWS_COMPILER_00054 | module extensions carry the module abbreviation | Not applicable | no extension | — |
| SWS_COMPILER_00042 | no standardized configuration interface | Not applicable | informative | — |
