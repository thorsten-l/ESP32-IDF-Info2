#pragma once

#ifdef HAS_SDCARD
extern void list_sdcard_root();

#define LIST_SDCARD_ROOT() list_sdcard_root()

#else

#define LIST_SDCARD_ROOT()

#endif
