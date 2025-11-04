#ifndef __LIBGPIOD_COMPAT_H
#define __LIBGPIOD_COMPAT_H

#ifdef RASPI
#include <gpiod.h>
// This seems to work... The fact that there is no compile
// time version is baffling... :/
// It's the basis of making a library...
// Do they expect you to use dlopen? Is that what is going on?
#ifndef GPIOD_CTXLESS_FLAG_OPEN_DRAIN
#define LIBGPIOD3
#endif

#ifdef LIBGPIOD3

#define GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP GPIOD_LINE_BIAS_PULL_UP
#define GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN GPIOD_LINE_BIAS_PULL_DOWN

// Extra struct needed due to how libgpiod3 is written... :/
struct gpiod_line {
	struct gpiod_line_request* request;
	struct gpiod_chip* chip;
	int offset;
};

struct gpiod_line* gpiod_line_find(const char *line_name);
void gpiod_line_close_chip(struct gpiod_line* in);
void gpiod_line_release(struct gpiod_line* in);
void gpiod_line_request_input_flags(struct gpiod_line* in, const char *consumer, gpiod_line_bias);
int gpiod_line_get_value(struct gpiod_line* in);

#endif
#endif

#endif
