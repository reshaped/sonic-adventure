/* Maple bus driver: its state, its device tables and its version */
#include <kaede.h>
#include <mpdrv.h>
#include <sg_xpt.h>

const Uint8 _mp_Per2APTable[KDD_DEV_MAX] = {
#include "shinobi/mpgbl/_mp_Per2APTable.inc"
};
const Sint32 _mp_RootDeviceNo[MPDRV_PORTS] = {
#include "shinobi/mpgbl/_mp_RootDeviceNo.inc"
};
const Sint32 _mp_MajorVersion = {
#include "shinobi/mpgbl/_mp_MajorVersion.inc"
};
const Sint32 _mp_MinorVersion = {
#include "shinobi/mpgbl/_mp_MinorVersion.inc"
};

struct mp_context _mp_Context;
struct mp_context *_mp;
KDS_GUNMODEINFO _mp_GunModeInfo;
