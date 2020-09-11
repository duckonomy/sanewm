#ifndef SANEWM_MONITOR_H
#define SANEWM_MONITOR_H

#include <xcb/xcb.h>
#include <xcb/randr.h>
#include "types.h"

xcb_screen_t *xcb_screen_of_display(xcb_connection_t *con, int screen);
void get_monitor_size(int8_t with_offsets, int16_t *monitor_x, int16_t *monitor_y, uint16_t *monitor_width, uint16_t *monitor_height, const struct sane_window *window);
int setup_randr(void);
void get_randr(void);
void get_randr_outputs(xcb_randr_output_t *outputs, const int len, xcb_timestamp_t timestamp);
void arrange_by_monitor(struct monitor *monitor);
struct monitor *find_monitor(xcb_randr_output_t id);
struct monitor *find_clone_monitors(xcb_randr_output_t id, const int16_t x, const int16_t y);
struct monitor *find_monitor_by_coordinate(const int16_t x, const int16_t y);
struct monitor *add_monitor(xcb_randr_output_t id, const int16_t x, const int16_t y, const uint16_t width, const uint16_t height);
void delete_monitor(struct monitor *mon);
void change_monitor(const Arg *arg);
void send_to_monitor(const Arg *arg);

#endif
