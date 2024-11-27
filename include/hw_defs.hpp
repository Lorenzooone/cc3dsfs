#ifndef __HW_DEFS_HPP
#define __HW_DEFS_HPP

#define AUDIO_CHANNELS 2
#define SAMPLE_RATE_BASE 67027964
#define SAMPLE_RATE_DIVISOR 2048

#define TOP_WIDTH_3DS 400
#define BOT_WIDTH_3DS 320
#define HEIGHT_3DS 240

#define TOP_SPECIAL_DS_WIDTH_3DS 384
#define BOT_SPECIAL_DS_WIDTH_3DS 320

#define TOP_SCALED_DS_WIDTH_3DS 320
#define BOT_SCALED_DS_WIDTH_3DS 320

#define WIDTH_DS 256
#define HEIGHT_DS 192

#define WIDTH_GBA 240
#define HEIGHT_GBA 160

#define WIDTH_SCALED_GBA 360
#define HEIGHT_SCALED_GBA 240

#define WIDTH_GB 160
#define HEIGHT_GB 144

#define WIDTH_SCALED_GB 266
#define HEIGHT_SCALED_GB 240

#define WIDTH_SCALED_SNES 284
#define HEIGHT_SCALED_SNES 224

#define WIDTH_SNES 256
#define HEIGHT_SNES 224

#define WIDTH_NES 256
#define HEIGHT_NES 240

#define TOP_SIZE_3DS (TOP_WIDTH_3DS * HEIGHT_3DS)
#define BOT_SIZE_3DS (BOT_WIDTH_3DS * HEIGHT_3DS)
#define TOP_SIZE_DS (WIDTH_DS * HEIGHT_DS)

#define IN_VIDEO_WIDTH_3DS HEIGHT_3DS
#define IN_VIDEO_HEIGHT_3DS (TOP_WIDTH_3DS + BOT_WIDTH_3DS)
#define IN_VIDEO_NO_BOTTOM_SIZE_3DS ((TOP_WIDTH_3DS - BOT_WIDTH_3DS) * HEIGHT_3DS)
#define IN_VIDEO_SIZE_3DS (IN_VIDEO_WIDTH_3DS * IN_VIDEO_HEIGHT_3DS)
#define IN_VIDEO_BPP_SIZE_3DS 24

#define IN_VIDEO_WIDTH_3DS_3D HEIGHT_3DS
#define IN_VIDEO_HEIGHT_3DS_3D (TOP_WIDTH_3DS + BOT_WIDTH_3DS + TOP_WIDTH_3DS)
#define IN_VIDEO_NO_BOTTOM_SIZE_3DS_3D (((TOP_WIDTH_3DS - BOT_WIDTH_3DS) * HEIGHT_3DS) * 2)
#define IN_VIDEO_SIZE_3DS_3D (IN_VIDEO_WIDTH_3DS_3D * IN_VIDEO_HEIGHT_3DS_3D)
#define IN_VIDEO_BPP_SIZE_3DS_3D 24

#define IN_VIDEO_WIDTH_DS WIDTH_DS
#define IN_VIDEO_HEIGHT_DS (HEIGHT_DS + HEIGHT_DS)
#define IN_VIDEO_SIZE_DS (IN_VIDEO_WIDTH_DS * IN_VIDEO_HEIGHT_DS)
#define IN_VIDEO_BPP_SIZE_DS_BASIC 15
#define IN_VIDEO_BPP_SIZE_DS_BLENDING 18

#define MAX_IN_VIDEO_WIDTH IN_VIDEO_WIDTH_DS
#define MAX_IN_VIDEO_HEIGHT IN_VIDEO_HEIGHT_3DS_3D
#define MAX_IN_VIDEO_SIZE (MAX_IN_VIDEO_WIDTH * MAX_IN_VIDEO_HEIGHT)
#define MAX_IN_VIDEO_BPP_SIZE IN_VIDEO_BPP_SIZE_3DS

// 1096 is the value when things are ideal. However, it can actually happen that the transfer isn't 100% on time.
// When that happens, a bit more audio data may get transfered. It's a ton on O3DS when Windows underprioritizes USB...
// In general, it should be less than * 2, but you never know. For now, go for safety at x16...
#define O3DS_SAMPLES_IN (1096 * 16)
#define N3DSXL_SAMPLES_IN (1096 * 16)
#define DS_SAMPLES_IN (1096)
#define MAX_SAMPLES_IN O3DS_SAMPLES_IN

#endif
