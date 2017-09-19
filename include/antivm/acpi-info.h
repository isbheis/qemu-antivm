/*
 * 2017-09-15
 * some const string for acpi
*/

// acpi oem id, max length is 6
#define ACPI_OEM_ID "LENOVO"

// acpi oem table name, max length is 8
#define ACPI_OEM_TABLE_ID "TC-FW___"

// acpi oem revision, DWORD
#define ACPI_OEM_REVISION 0x12d0

// acpi asl compiler id, max length is 4
#define ACPI_ASL_COMPILER_ID "LENO"

// [optinal] acpi asl compiler revision, default is 1
//#define ACPI_ASL_COMPILER_REVISION 1

// add 2017-09-18, acpi fw_cfg device aml id
#define ACPI_FW_CFG_AML_ID "LENO0002"

// add 2017-09-18, pvpanic device(PEVT) acpi id,
// this should be consistent with smbios PEVT aml
#define ACPI_PVPANIC_ID "LENO0001"
