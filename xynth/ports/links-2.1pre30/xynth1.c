/***************************************************************************
    begin                : Thu Jan 22 2004
    copyright            : (C) 2004 - 2006 by Alper Akcan
    email                : distchx@yahoo.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License as        *
 *   published by the Free Software Foundation; either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 ***************************************************************************/

#include "cfg.h"

#ifdef NCE_WINDOWS

#include "links.h"
#include "bits.h"

#include <ctype.h>
#include <xynth.h>

//#define DEBUGF printf
#define DEBUGF

#define MIN(a, b)	((a) < (b) ? (a) : (b))
#define MAX(a, b)	((a) > (b) ? (a) : (b))

extern struct graphics_driver xynth_driver;

typedef struct lxynth_device_s {
	char *title;
	s_rect_t update;
	s_surface_t *surface;
} lxynth_device_t;

typedef struct lxynth_eventq_s {
	s_list_t *list;
	s_thread_mutex_t *mut;
} lxynth_eventq_t;

typedef struct lxynth_gd_s {
	s_list_t *list;
	s_thread_mutex_t *mut;
	struct graphics_device *active;
} lxynth_gd_t;

typedef struct lxynth_root_s {
	int timerid;
	int running;
	s_thread_t *tid;
	s_window_t *window;
	lxynth_eventq_t *eventq;
	lxynth_gd_t *gd;
} lxynth_root_t;

static lxynth_root_t *lxynth_root = NULL;
static void lxynth_set_title (struct graphics_device *dev, unsigned char *title);

static void lxynth_surface_register_update (void *dev)
{
	lxynth_device_t *wd;
        if (lxynth_root->running == 0) return;
        s_thread_mutex_lock(lxynth_root->gd->mut);
        if (lxynth_root->gd->active != dev) {
		s_thread_mutex_unlock(lxynth_root->gd->mut);
		return;
	}
	wd = (lxynth_device_t *) ((struct graphics_device *) dev)->driver_data;
	s_putboxpart(lxynth_root->window->surface, wd->update.x, wd->update.y, wd->update.w, wd->update.h, wd->surface->width, wd->surface->height, wd->surface->vbuf, wd->update.x, wd->update.y);
	wd->update = (s_rect_t) {-1, -1, -1, -1};
	s_thread_mutex_unlock(lxynth_root->gd->mut);
}

static void lxynth_surface_update (struct graphics_device *dev, int x, int y, int w, int h)
{
	s_rect_t rect;
	lxynth_device_t *wd;
	wd = (lxynth_device_t *) dev->driver_data;
	rect = wd->update;
	if (rect.x >= 0) { wd->update.x = MIN(rect.x, x); } else { wd->update.x = x; }
	if (rect.y >= 0) { wd->update.y = MIN(rect.y, y); } else { wd->update.y = y; }
	if (rect.w >= 0) { wd->update.w = MAX(rect.x + rect.w, x + w) - wd->update.x; } else { wd->update.w = w; }
	if (rect.h >= 0) { wd->update.h = MAX(rect.y + rect.h, y + h) - wd->update.y; } else { wd->update.h = h; }
	register_bottom_half(lxynth_surface_register_update, dev);
}

static void lxynth_event_parse_mouse (s_event_t *event)
{
	int x;
	int y;
	int flag = 0;

	x = event->mouse->x - lxynth_root->window->surface->buf->x;
	y = event->mouse->y - lxynth_root->window->surface->buf->y;
	if ((x < 0) || (x >  lxynth_root->window->surface->buf->w) ||
	    (y < 0) || (y >  lxynth_root->window->surface->buf->h)) {
		return;
	}
	switch (event->mouse->b) {
		case 1:  flag = B_RIGHT;  break;
		case 2:  flag = B_MIDDLE; break;
		case 4:  flag = B_LEFT;   break;
	}
	if (event->type & MOUSE_PRESSED) {
		flag |= B_DOWN;
		if (event->mouse->b == MOUSE_WHEELXBUTTON_U) {
			flag = B_MOVE | B_WHEELUP;
		} else if (event->mouse->b == MOUSE_WHEELXBUTTON_D) {
			flag = B_MOVE | B_WHEELDOWN;
		} else if (event->mouse->b == MOUSE_WHEELYBUTTON_U) {
			flag = B_MOVE | B_WHEELLEFT;
		} else if (event->mouse->b == MOUSE_WHEELYBUTTON_D) {
			flag = B_MOVE | B_WHEELRIGHT;
		}
	} else if (event->type & (MOUSE_RELEASED | MOUSE_CLICKED)) {
		flag |= B_UP;
	} else {
		if (event->mouse->buttons) {
			flag |= B_DRAG;
		} else {
			flag |= B_MOVE;
		}
	}
	lxynth_root->gd->active->mouse_handler(lxynth_root->gd->active, x, y, flag);
}

static void lxynth_event_parse_keybd (s_event_t *event)
{
	int k = 0;
	int flag = 0;

	if (!(event->type & KEYBD_PRESSED)) {
		return;
	}
	switch (event->keybd->keycode) {
		case S_KEYCODE_RETURN:	k = KBD_ENTER;	break;
		case S_KEYCODE_DELETE:	k = KBD_BS;	break;
		case S_KEYCODE_TAB:	k = KBD_TAB;	break;
		case S_KEYCODE_ESCAPE:	k = KBD_ESC;	break;
		case S_KEYCODE_UP:	k = KBD_UP;	break;
		case S_KEYCODE_DOWN:	k = KBD_DOWN;	break;
		case S_KEYCODE_LEFT:	k = KBD_LEFT;	break;
		case S_KEYCODE_RIGHT:	k = KBD_RIGHT;	break;
		case S_KEYCODE_INSERT:	k = KBD_INS;	break;
		case S_KEYCODE_REMOVE:	k = KBD_DEL;	break;
		case S_KEYCODE_HOME:	k = KBD_HOME;	break;
		case S_KEYCODE_END:	k = KBD_END;	break;
		case S_KEYCODE_PAGEUP:	k = KBD_PAGE_UP;break;
		case S_KEYCODE_PAGEDOWN:k = KBD_PAGE_DOWN;break;
		case S_KEYCODE_F1:	k = KBD_F1;	break;
		case S_KEYCODE_F2:	k = KBD_F2;	break;
		case S_KEYCODE_F3:	k = KBD_F3;	break;
		case S_KEYCODE_F4:	k = KBD_F4;	break;
		case S_KEYCODE_F5:	k = KBD_F5;	break;
		case S_KEYCODE_F6:	k = KBD_F6;	break;
		case S_KEYCODE_F7:	k = KBD_F7;	break;
		case S_KEYCODE_F8:	k = KBD_F8;	break;
		case S_KEYCODE_F9:	k = KBD_F9;	break;
		case S_KEYCODE_F10:	k = KBD_F10;	break;
		case S_KEYCODE_F11:	k = KBD_F11;	break;
		case S_KEYCODE_F12:	k = KBD_F12;	break;
		default:
			k = event->keybd->ascii;
			if (!isprint(k)) {
				return;
			}
			break;
	}
	if (event->keybd->flag & (KEYCODE_CTRLF)) {
		flag |= KBD_CTRL;
	}
	if (event->keybd->flag & (KEYCODE_SHIFTF)) {
	}
	if (event->keybd->flag & (KEYCODE_ALTF | KEYCODE_ALTGRF)) {
		flag |= KBD_ALT;
	}

#define KEYBD_HOOK0 \
	int acc = 0;\
	int pos = 0;\
	struct graphics_device *gd;\
	s_thread_mutex_lock(lxynth_root->gd->mut);\
	while (!s_list_eol(lxynth_root->gd->list, pos)) {\
		gd = (struct graphics_device *) s_list_get(lxynth_root->gd->list, pos);\
		if (gd == lxynth_root->gd->active) {\

#define KEYBD_HOOK1 \
		}\
		pos++;\
	}\
	s_thread_mutex_unlock(lxynth_root->gd->mut);\
	if (acc) {\
		char *title;\
		struct rect r;\
		r.x1 = 0;\
		r.y1 = 0;\
		r.x2 = lxynth_root->window->surface->buf->w;\
		r.y2 = lxynth_root->window->surface->buf->h;\
		title = strdup(((lxynth_device_t *) lxynth_root->gd->active->driver_data)->title);\
		lxynth_set_title(lxynth_root->gd->active, title);\
		lxynth_root->gd->active->resize_handler(lxynth_root->gd->active);\
		lxynth_root->gd->active->redraw_handler(lxynth_root->gd->active, &r);\
		s_free(title);\
	}

	if ((flag == KBD_CTRL) && (event->keybd->ascii == 'w')) {
		KEYBD_HOOK0;
		if (lxynth_root->gd->list->nb_elt > 1) {
			s_list_remove(lxynth_root->gd->list, pos);
			lxynth_root->gd->active = (struct graphics_device *) s_list_get(lxynth_root->gd->list, 0);
			acc = 1;
			break;
		}
		KEYBD_HOOK1;
		if (acc) {
			gd->keyboard_handler(gd, KBD_CLOSE, 0);
		}
	} else if ((flag == KBD_CTRL) && (k == KBD_LEFT)) {
		KEYBD_HOOK0;
		if (pos >= 1) {
			lxynth_root->gd->active = (struct graphics_device *) s_list_get(lxynth_root->gd->list, pos - 1);
			acc = 1;
			break;
		}
		KEYBD_HOOK1;
	} else if ((flag == KBD_CTRL) && (k == KBD_RIGHT)) {
		KEYBD_HOOK0;
		if ((pos + 1) < lxynth_root->gd->list->nb_elt) {
			lxynth_root->gd->active = (struct graphics_device *) s_list_get(lxynth_root->gd->list, pos + 1);
			acc = 1;
			break;
		}
		KEYBD_HOOK1;
	} else {
		lxynth_root->gd->active->keyboard_handler(lxynth_root->gd->active, k, flag);
	}
}

static void lxynth_event_parse_config (s_event_t *event)
{
	if (event->type & (CONFIG_CHNGW | CONFIG_CHNGH)) {
		int pos = 0;
		s_thread_mutex_lock(lxynth_root->gd->mut);
		while (!s_list_eol(lxynth_root->gd->list, pos)) {
			struct graphics_device *gd = (struct graphics_device *) s_list_get(lxynth_root->gd->list, pos);
			lxynth_device_t *wd = gd->driver_data;

			gd->size.x2 = lxynth_root->window->surface->buf->w;
			gd->size.y2 = lxynth_root->window->surface->buf->h;
			wd->surface->width = lxynth_root->window->surface->buf->w;
			wd->surface->height = lxynth_root->window->surface->buf->h;
			s_free(wd->surface->vbuf);
			wd->surface->vbuf = (char *) s_malloc(wd->surface->width *
			                                      wd->surface->height *
			                                      wd->surface->bytesperpixel);
			pos++;
		}
		s_thread_mutex_unlock(lxynth_root->gd->mut);
		lxynth_root->gd->active->resize_handler(lxynth_root->gd->active);
	}
}

static void lxynth_timer (void *arg)
{
        int x = 0;
        int y = 0;
        int k = 0;
        int flag = 0;
        s_event_t *event;

	s_thread_mutex_lock(lxynth_root->eventq->mut);
        while (!s_list_eol(lxynth_root->eventq->list, 0)) {
		event = (s_event_t *) s_list_get(lxynth_root->eventq->list, 0);
		s_list_remove(lxynth_root->eventq->list, 0);
                switch (event->type & EVENT_MASK) {
			case QUIT_EVENT:	break;
			case MOUSE_EVENT:	lxynth_event_parse_mouse(event);	break;
			case KEYBD_EVENT:	lxynth_event_parse_keybd(event);	break;
			case CONFIG_EVENT:	lxynth_event_parse_config(event);	break;
		}
		s_event_uninit(event);
	}
	s_thread_mutex_unlock(lxynth_root->eventq->mut);

        if (lxynth_root->running) {
		lxynth_root->timerid = install_timer(20, lxynth_timer, NULL);
	}
}

static void lxynth_atevent (s_window_t *window, s_event_t *event)
{
        s_event_t *e;

        if (lxynth_root == NULL) {
		return;
	}
	switch (event->type & EVENT_MASK) {
		case QUIT_EVENT:
		case MOUSE_EVENT:
		case KEYBD_EVENT:
		case EXPOSE_EVENT:
		case CONFIG_EVENT:
		case FOCUS_EVENT:
			if (!s_event_init(&e)) {
				e->type = event->type;
				memcpy(e->mouse, event->mouse, sizeof(s_mouse_t));
				memcpy(e->keybd, event->keybd, sizeof(s_keybd_t));
				memcpy(e->expose->rect, event->expose->rect, sizeof(s_rect_t));
				s_thread_mutex_lock(lxynth_root->eventq->mut);
				s_list_add(lxynth_root->eventq->list, e, -1);
				s_thread_mutex_unlock(lxynth_root->eventq->mut);
			}
			break;
		default:
			break;
	}
}

static void lxynth_atexit (s_window_t *window)
{
        if (lxynth_root == NULL) {
		return;
	}
	if (lxynth_root->running) {
		struct graphics_device *gd = lxynth_root->gd->active;

		s_thread_mutex_lock(lxynth_root->gd->mut);
		lxynth_root->running = 0;
		lxynth_root->gd->active = NULL;
		s_thread_mutex_unlock(lxynth_root->gd->mut);
		gd->keyboard_handler(gd, KBD_CLOSE, 0);
	}
}

static unsigned char * lxynth_init_driver (unsigned char *param, unsigned char *display)
{
        int bpp;
        int Bpp;

	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);

	lxynth_root = (lxynth_root_t *) s_malloc(sizeof(lxynth_root_t));
	lxynth_root->eventq = (lxynth_eventq_t *) s_malloc(sizeof(lxynth_eventq_t));
	s_list_init(&(lxynth_root->eventq->list));
	s_thread_mutex_init(&(lxynth_root->eventq->mut));
	lxynth_root->gd = (lxynth_gd_t *) s_malloc(sizeof(lxynth_gd_t));
	s_list_init(&(lxynth_root->gd->list));
	s_thread_mutex_init(&(lxynth_root->gd->mut));
	lxynth_root->gd->active = NULL;

	s_client_init(&(lxynth_root->window));
	s_window_new(lxynth_root->window, WINDOW_MAIN, NULL);
	s_window_set_coor(lxynth_root->window, WINDOW_NOFORM, 50, 50,
	                                                      (lxynth_root->window->surface->width * 2) / 3,
	                                                      (lxynth_root->window->surface->height * 2) / 3);
        bpp = lxynth_root->window->surface->bitsperpixel;
        Bpp = lxynth_root->window->surface->bytesperpixel;

        if (bpp ==  32) {
		bpp = 24;
	}
	xynth_driver.depth = (bpp << 3) | Bpp;

	s_window_show(lxynth_root->window);
	lxynth_root->tid = s_thread_create(s_client_main, lxynth_root->window);
	s_client_atevent(lxynth_root->window, lxynth_atevent);
	s_client_atexit(lxynth_root->window, lxynth_atexit);
	lxynth_root->running = 1;

	install_timer(20, lxynth_timer, NULL);

	return NULL;
}

static struct graphics_device * lxynth_init_device (void)
{
        char *vbuf;
	lxynth_device_t *wd;
	struct graphics_device *gd;

	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);

	wd = (lxynth_device_t *) s_malloc(sizeof(lxynth_device_t));
	wd->update = (s_rect_t) {-1, -1, -1, -1};
	gd = (struct graphics_device *) s_malloc(sizeof(struct graphics_device));

	wd->title = NULL;
        wd->surface = (s_surface_t *) s_malloc(sizeof(s_surface_t));
        vbuf = (char *) s_malloc(lxynth_root->window->surface->buf->w * lxynth_root->window->surface->buf->h * lxynth_root->window->surface->bytesperpixel);
        s_getsurfacevirtual(wd->surface, lxynth_root->window->surface->buf->w , lxynth_root->window->surface->buf->h, lxynth_root->window->surface->bitsperpixel, vbuf);

	gd->size.x1 = 0;
	gd->size.x2 = wd->surface->width;
	gd->size.y1 = 0;
	gd->size.y2 = wd->surface->height;
	gd->clip.x1 = 0;
	gd->clip.x2 = gd->size.x2;
	gd->clip.y1 = 0;
	gd->clip.y2 = gd->size.y2;

	gd->drv = &xynth_driver;
	gd->driver_data = wd;
	gd->user_data = NULL;

	s_thread_mutex_lock(lxynth_root->gd->mut);
	s_list_add(lxynth_root->gd->list, gd, -1);
	lxynth_root->gd->active = gd;
	s_thread_mutex_unlock(lxynth_root->gd->mut);

	return gd;
}

static void lxynth_shutdown_device (struct graphics_device *dev)
{
        int acc = 0;
        int pos = 0;
	lxynth_device_t *wd;
	struct graphics_device *gd;

	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);

	s_thread_mutex_lock(lxynth_root->gd->mut);
	while (!s_list_eol(lxynth_root->gd->list, pos)) {
		gd = (struct graphics_device *) s_list_get(lxynth_root->gd->list, pos);
		if (gd == dev) {
			s_list_remove(lxynth_root->gd->list, pos);
			if (lxynth_root->gd->list->nb_elt > 0) {
				lxynth_root->gd->active = (struct graphics_device *) s_list_get(lxynth_root->gd->list, 0);
				acc = 1;
			} else {
				lxynth_root->gd->active = NULL;
			}
			break;
		}
		pos++;
	}
	unregister_bottom_half(lxynth_surface_register_update, dev);
	wd = (lxynth_device_t *) dev->driver_data;
	s_free(wd->title);
	s_free(wd->surface->vbuf);
	s_free(wd->surface);
	s_free(wd);
	s_free(dev);
	s_thread_mutex_unlock(lxynth_root->gd->mut);

	if (acc && lxynth_root->running) {
		char *title;
		struct rect r;
		r.x1 = 0;
		r.y1 = 0;
		r.x2 = lxynth_root->window->surface->buf->w;
		r.y2 = lxynth_root->window->surface->buf->h;
		title = strdup(((lxynth_device_t *) lxynth_root->gd->active->driver_data)->title);
		lxynth_set_title(lxynth_root->gd->active, title);
		lxynth_root->gd->active->resize_handler(lxynth_root->gd->active);
		lxynth_root->gd->active->redraw_handler(lxynth_root->gd->active, &r);
		s_free(title);
	}
}

static void lxynth_shutdown_driver (void)
{
	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);

        if (lxynth_root->running) {
		lxynth_root->running = 0;
		s_client_quit(lxynth_root->window);
		s_thread_join(lxynth_root->tid, NULL);
		kill_timer(lxynth_root->timerid);
	}

	while (!s_list_eol(lxynth_root->eventq->list, 0)) {
		s_event_t *event = s_list_get(lxynth_root->eventq->list, 0);
		s_list_remove(lxynth_root->eventq->list, 0);
		s_event_uninit(event);
	}
	s_free(lxynth_root->eventq->list);
	s_thread_mutex_destroy(lxynth_root->eventq->mut);
	s_free(lxynth_root->eventq);

	while (!s_list_eol(lxynth_root->gd->list, 0)) {
		struct graphics_device *gd = s_list_get(lxynth_root->gd->list, 0);
		lxynth_device_t *wd = (lxynth_device_t *) gd->driver_data;
		s_list_remove(lxynth_root->gd->list, 0);
		s_free(wd->title);
		s_free(wd->surface->vbuf);
		s_free(wd->surface);
		s_free(wd);
		s_free(gd);
	}
	s_free(lxynth_root->gd->list);
	s_thread_mutex_destroy(lxynth_root->gd->mut);
	s_free(lxynth_root->gd);

	s_free(lxynth_root);
}

static unsigned char * lxynth_driver_param (void)
{
	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);
	exit(0);
}

static int lxynth_get_empty_bitmap (struct bitmap *dest)
{
	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);

	dest->data = (char *) s_malloc(dest->x * dest->y * lxynth_root->window->surface->bytesperpixel);
	dest->skip = dest->x * lxynth_root->window->surface->bytesperpixel;
	dest->flags = 0;

	return 0;
}

static void lxynth_register_bitmap (struct bitmap *bmp)
{
	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);
}

static void * lxynth_prepare_strip (struct bitmap *bmp, int top, int lines)
{
	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);
	exit(0);
}

static void lxynth_commit_strip (struct bitmap *bmp, int top, int lines)
{
	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);
	exit(0);
}

static void lxynth_unregister_bitmap (struct bitmap *bmp)
{
	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);
	s_free(bmp->data);
}

#define	CLIP_PREFACE \
	if ((x >= dev->clip.x2) || ((x + xs) <= dev->clip.x1)) return;\
	if ((y >= dev->clip.y2) || ((y + ys) <= dev->clip.y1)) return;\
	if ((x + xs) > dev->clip.x2) xs = dev->clip.x2 - x;\
	if ((y + ys) > dev->clip.y2) ys = dev->clip.y2 - y;\
	if ((dev->clip.x1 - x) > 0){\
		xs -= (dev->clip.x1 - x);\
		data += lxynth_root->window->surface->bytesperpixel * (dev->clip.x1 - x);\
		x = dev->clip.x1;\
	}\
	if ((dev->clip.y1 - y) > 0) {\
		ys -= (dev->clip.y1 - y);\
		data += hndl->skip * (dev->clip.y1 - y);\
		y = dev->clip.y1;\
	}

static void lxynth_draw_bitmap (struct graphics_device *dev, struct bitmap *hndl, int x, int y)
{
	lxynth_device_t *wd;
	int xs = hndl->x;
	int ys = hndl->y;
	char *data = hndl->data;

	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);

	CLIP_PREFACE;

        if (lxynth_root->running == 0) return;
        s_thread_mutex_lock(lxynth_root->gd->mut);
        if (lxynth_root->gd->active != dev) {
		s_thread_mutex_unlock(lxynth_root->gd->mut);
		return;
	}

	wd = (lxynth_device_t *) dev->driver_data;
        s_putboxpart(wd->surface, x, y, xs, ys, hndl->x, hndl->y, data, 0, 0);
	lxynth_surface_update(dev, x, y, xs, ys);
	s_thread_mutex_unlock(lxynth_root->gd->mut);
}

static void lxynth_draw_bitmaps (struct graphics_device *dev, struct bitmap **hndls, int n, int x, int y)
{
	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);
	exit(0);
}

static long lxynth_get_color (int rgb)
{
	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);

	return s_rgbcolor(lxynth_root->window->surface, (rgb >> 0x10) & 0xFF,
	                                               (rgb >> 0x08) & 0xFF,
	                                               (rgb << 0x00) & 0xFF);
}

#define FILL_CLIP_PREFACE \
	if ((left >= right) || (top >= bottom)) return;\
	if ((left >= dev->clip.x2) || (right <= dev->clip.x1) || (top >= dev->clip.y2) || (bottom <= dev->clip.y1)) return;\
	if (left < dev->clip.x1) left = dev->clip.x1;\
	if (right > dev->clip.x2) right = dev->clip.x2;\
	if (top < dev->clip.y1) top = dev->clip.y1;\
	if (bottom > dev->clip.y2) bottom = dev->clip.y2;

static void lxynth_fill_area (struct graphics_device *dev, int left, int top, int right, int bottom, long color)
{
	lxynth_device_t *wd;

	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);

	FILL_CLIP_PREFACE;

        if (lxynth_root->running == 0) return;
        s_thread_mutex_lock(lxynth_root->gd->mut);
        if (lxynth_root->gd->active != dev) {
		s_thread_mutex_unlock(lxynth_root->gd->mut);
		return;
	}

	wd = (lxynth_device_t *) dev->driver_data;
	s_fillbox(wd->surface, left, top, right - left, bottom - top, color);
	lxynth_surface_update(dev, left, top, right - left, bottom - top);
	s_thread_mutex_unlock(lxynth_root->gd->mut);
}

#define HLINE_CLIP_PREFACE \
	if (left >= right) return;\
	if ((y < dev->clip.y1) || (y >= dev->clip.y2) || (right <= dev->clip.x1) || (left >= dev->clip.x2)) return;\
	if (left < dev->clip.x1) left = dev->clip.x1;\
	if (right > dev->clip.x2) right = dev->clip.x2;

static void lxynth_draw_hline (struct graphics_device *dev, int left, int y, int right, long color)
{
	lxynth_device_t *wd;

	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);

	HLINE_CLIP_PREFACE;

        if (lxynth_root->running == 0) return;
        s_thread_mutex_lock(lxynth_root->gd->mut);
        if (lxynth_root->gd->active != dev) {
		s_thread_mutex_unlock(lxynth_root->gd->mut);
		return;
	}

	wd = (lxynth_device_t *) dev->driver_data;
	s_hline(wd->surface, left, y, right, color);
	lxynth_surface_update(dev, left, y, right - left, 1);
	s_thread_mutex_unlock(lxynth_root->gd->mut);
}

#define VLINE_CLIP_PREFACE \
	if (top >= bottom) return;\
	if ((x < dev->clip.x1) || (x >= dev->clip.x2) || (top >= dev->clip.y2) || (bottom <= dev->clip.y1)) return;\
	if (top < dev->clip.y1) top = dev->clip.y1;\
	if (bottom > dev->clip.y2) bottom = dev->clip.y2;

static void lxynth_draw_vline (struct graphics_device *dev, int x, int top, int bottom, long color)
{
	lxynth_device_t *wd;

	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);

	VLINE_CLIP_PREFACE;

        if (lxynth_root->running == 0) return;
        s_thread_mutex_lock(lxynth_root->gd->mut);
        if (lxynth_root->gd->active != dev) {
		s_thread_mutex_unlock(lxynth_root->gd->mut);
		return;
	}

	wd = (lxynth_device_t *) dev->driver_data;
	s_vline(wd->surface, x, top, bottom, color);
	lxynth_surface_update(dev, x, top, 1, bottom - top);
	s_thread_mutex_unlock(lxynth_root->gd->mut);
}

#define HSCROLL_CLIP_PREFACE \
	if (!sc) return 0;\
	if (sc > (dev->clip.x2 - dev->clip.x1) || -sc > (dev->clip.x2 - dev->clip.x1))\
		return 1;

static int lxynth_hscroll (struct graphics_device *dev, struct rect_set **ignore, int sc)
{
	s_rect_t rect;
	lxynth_device_t *wd;

	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);

	HSCROLL_CLIP_PREFACE;

        if (lxynth_root->running == 0) return;
        s_thread_mutex_lock(lxynth_root->gd->mut);
        if (lxynth_root->gd->active != dev) {
		s_thread_mutex_unlock(lxynth_root->gd->mut);
		return;
	}

	wd = (lxynth_device_t *) dev->driver_data;

	ignore = NULL;

	rect.x = dev->clip.x1;
	rect.y = dev->clip.y1;
	rect.w = dev->clip.x2 - dev->clip.x1;
	rect.h = dev->clip.y2 - dev->clip.y1;

        s_copybox(wd->surface, rect.x, rect.y, rect.w, rect.h, rect.x + sc, rect.y);
	lxynth_surface_update(dev, rect.x, rect.y, rect.w, rect.h);
	s_thread_mutex_unlock(lxynth_root->gd->mut);

        return 1;
}

#define VSCROLL_CLIP_PREFACE \
	if (!sc) return 0;\
	if (sc > dev->clip.y2 - dev->clip.y1 || -sc > dev->clip.y2 - dev->clip.y1) return 1;

static int lxynth_vscroll (struct graphics_device *dev, struct rect_set **ignore, int sc)
{
	s_rect_t rect;
	lxynth_device_t *wd;

	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);

	VSCROLL_CLIP_PREFACE;

        if (lxynth_root->running == 0) return;
        s_thread_mutex_lock(lxynth_root->gd->mut);
        if (lxynth_root->gd->active != dev) {
		s_thread_mutex_unlock(lxynth_root->gd->mut);
		return;
	}

	wd = (lxynth_device_t *) dev->driver_data;

	ignore = NULL;

	rect.x = dev->clip.x1;
	rect.y = dev->clip.y1;
	rect.w = dev->clip.x2 - dev->clip.x1;
	rect.h = dev->clip.y2 - dev->clip.y1;

        s_copybox(wd->surface, rect.x, rect.y, rect.w, rect.h, rect.x, rect.y + sc);
	lxynth_surface_update(dev, rect.x, rect.y, rect.w, rect.h);
	s_thread_mutex_unlock(lxynth_root->gd->mut);

	return 1;
}

static void lxynth_set_clip_area (struct graphics_device *dev, struct rect *r)
{
	lxynth_device_t *wd;

	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);

        s_thread_mutex_lock(lxynth_root->gd->mut);
        if (lxynth_root->gd->active != dev) {
		s_thread_mutex_unlock(lxynth_root->gd->mut);
		return;
	}

	wd = (lxynth_device_t *) dev->driver_data;

	memcpy(&dev->clip, r, sizeof(struct rect));
	if ((dev->clip.x1 >= dev->clip.x2) ||
	    (dev->clip.y2 <= dev->clip.y1) ||
	    (dev->clip.y2 <= 0) ||
	    (dev->clip.x2 <= 0) ||
	    (dev->clip.x1 >= wd->surface->width) ||
	    (dev->clip.y1 >= wd->surface->height)) {
		dev->clip.x1 = dev->clip.x2 = dev->clip.y1 = dev->clip.y2 = 0;
	} else {
		if (dev->clip.x1 < 0) dev->clip.x1 = 0;
		if (dev->clip.x2 > wd->surface->width) dev->clip.x2 = wd->surface->width;
		if (dev->clip.y1 < 0) dev->clip.y1 = 0;
		if (dev->clip.y2 > wd->surface->height) dev->clip.y2 = wd->surface->height;
	}
	s_thread_mutex_unlock(lxynth_root->gd->mut);
}

static int lxynth_block (struct graphics_device *dev)
{
	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);
	exit(0);
}

static void lxynth_unblock (struct graphics_device *dev)
{
	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);
	exit(0);
}

static void lxynth_set_title (struct graphics_device *dev, unsigned char *title)
{
	lxynth_device_t *wd;

	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);

	wd = (lxynth_device_t *) dev->driver_data;
        s_free(wd->title);
        wd->title = strdup(title);

        s_thread_mutex_lock(lxynth_root->gd->mut);
        if (lxynth_root->gd->active != dev) {
		s_thread_mutex_unlock(lxynth_root->gd->mut);
		return;
	}

	s_window_set_title(lxynth_root->window, title);

	s_thread_mutex_unlock(lxynth_root->gd->mut);
}

static int lxynth_exec (unsigned char *command, int flag)
{
	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);
	exit(0);
}

struct graphics_driver xynth_driver = {
	"xynth",
	lxynth_init_driver,
	lxynth_init_device,
	lxynth_shutdown_device,
	lxynth_shutdown_driver,
	lxynth_driver_param,
	lxynth_get_empty_bitmap,
	lxynth_register_bitmap,
	lxynth_prepare_strip,
	lxynth_commit_strip,
	lxynth_unregister_bitmap,
	lxynth_draw_bitmap,
	lxynth_draw_bitmaps,
	lxynth_get_color,
	lxynth_fill_area,
	lxynth_draw_hline,
	lxynth_draw_vline,
	lxynth_hscroll,
	lxynth_vscroll,
	lxynth_set_clip_area,
	lxynth_block,
	lxynth_unblock,
	lxynth_set_title,
	lxynth_exec,
	0,
	0,
	0,
	0,
};

#endif
