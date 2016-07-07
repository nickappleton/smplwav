add_library(smplwav STATIC ../smplwav.h ../smplwav_mount.h ../smplwav_serialise.h smplwav.c smplwav_serialise.c smplwav_mount.c)
target_include_directories(smplwav PUBLIC "../..")
