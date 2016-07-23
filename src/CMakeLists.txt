add_library(smplwav STATIC ../smplwav.h ../smplwav_convert.h ../smplwav_mount.h ../smplwav_serialise.h smplwav.c smplwav_convert.c smplwav_mount.c smplwav_serialise.c)
target_include_directories(smplwav PUBLIC "../..")
