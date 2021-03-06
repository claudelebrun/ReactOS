#ifndef __VMWINST_H
#define __VMWINST_H

#ifndef PSCB_BUTTONPRESSED
#define PSCB_BUTTONPRESSED	(3)
#endif

/* metrics */
#define PROPSHEETWIDTH  250
#define PROPSHEETHEIGHT 120
#define PROPSHEETPADDING        6
#define SYSTEM_COLUMN   (18 * PROPSHEETPADDING)
#define LABELLINE(x)    (((PROPSHEETPADDING + 2) * x) + (x + 2))
#define ICONSIZE        16

/* Resource IDs */

#define IDS_WIZARD_NAME	100
#define IDS_FAILEDTOLOCATEDRIVERS	101
#define IDS_FAILEDTOCOPYFILES	102
#define IDS_FAILEDTOACTIVATEDRIVER	103
#define IDS_FAILEDTOSELVGADRIVER	104
#define IDS_FAILEDTOSELVBEDRIVER	105
#define IDS_UNINSTNOTICE	106

#define IDD_WELCOMEPAGE	100
#define IDD_INSERT_VMWARE_TOOLS	101
#define IDD_CONFIG	102
#define IDD_CHOOSEACTION	103
#define IDD_SELECTDRIVER	104
#define IDD_INSTALLATION_FAILED	105
#define IDD_DOUNINSTALL	106

#define IDC_COLORQUALITY	200
#define IDC_CONFIGSETTINGS	201
#define IDC_USEOTHERDRIVER	202
#define IDC_UNINSTALL	203
#define IDC_VGA	204
#define IDC_VBE	205

#endif /* __VMWINST_H */
