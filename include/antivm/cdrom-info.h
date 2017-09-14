/*
 * add 2017-09-14
 * some const string for cd-rom
**/

// cdrom vendor, max length is 8
#define CDROM_VENDOR "DELL"

// cdrom version, max length is 8
#define CDROM_VERSION "MA01"

// cdrom model str, for ide driver, max length is 16(in hw/ide/atapi.c)
#define CDROM_MODEL_STR "DELL DVD-ROM"

// cdrom product str, for scsi driver
#define CDROM_PRODUCT_STR "DELL CD-ROM"
