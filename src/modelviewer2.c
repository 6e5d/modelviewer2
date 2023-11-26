#include <assert.h>
#include <cglm/cglm.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <vulkan/vulkan.h>
#include <wayland-client.h>

#include "../../camcon/include/camcon.h"
#include "../../modelobj/include/modelobj.h"
#include "../../modelobj/include/normal.h"
#include "../../vkbasic/include/vkbasic.h"
#include "../../vkbasic3d/include/model.h"
#include "../../vkbasic3d/include/vkbasic3d.h"
#include "../../vkstatic/include/oneshot.h"
#include "../../vkstatic/include/vkstatic.h"
#include "../../vkwayland/include/vkwayland.h"
#include "../../wlezwrap/include/wlezwrap.h"
#include "../include/modelviewer2.h"

static void f_resize(Modelviewer2* mv, uint32_t w, uint32_t h) {
	mv->width = w;
	mv->height = h;
	mv->resize = true;
}

static void f_quit(Modelviewer2* mv) {
	mv->quit = true;
}

static void motion(Modelviewer2 *mv, bool *keystate,
	float x, float y, float px, float py
) {
	if (keystate[WLEZWRAP_LSHIFT]) {
		vec3 dp = {
			-2e-3f * (float)(x - px) * mv->cc.r,
			-2e-3f * (float)(y - py) * mv->cc.r,
			0.0f,
		};
		camcon_translate(&mv->cc, dp);
	} else if (keystate[WLEZWRAP_LCTRL]) {
		vec3 dp = {
			0.0f,
			0.0f,
			2e-3f * (float)(y - py) * mv->cc.r,
		};
		camcon_translate(&mv->cc, dp);
	} else {
		camcon_rotate(&mv->cc,
			-5e-3f * (float)(x - px),
			5e-3f * (float)(y - py)
		);
	}
}

static void f_key(Modelviewer2* mv, uint8_t key, bool pressed) {
	if (!pressed) {
		return;
	}
	vec3 dp = {0};
	switch (key) {
	case 'i':
		mv->cc.r *= 0.9f;
		printf("zoomin: %f\n", (double)mv->cc.r);
		break;
	case 'o':
		mv->cc.r *= 1.1f;
		printf("zoomout: %f\n", (double)mv->cc.r);
		break;
	case 'r':
		camcon_init(&mv->cc);
		return;
	case 's':
		mv->playing = 2;
		return;
	case ' ':
		if (mv->playing == 1) {
			mv->playing = 0;
		} else {
			mv->playing = 1;
		}
		return;
	default:
		return;
	}
	camcon_translate(&mv->cc, dp);
}

static void f_event(void* data, uint8_t type, WlezwrapEvent *e) {
	static float px, py;
	Modelviewer2* mv = data;
	switch(type) {
	case 0:
		f_quit(mv);
		break;
	case 1:
		f_resize(mv, e->resize[0], e->resize[1]);
		break;
	case 2: {
		float x = e->motion[0];
		float y = e->motion[1];
		if (mv->wew.keystate[WLEZWRAP_MCLICK]) {
			motion(mv, mv->wew.keystate, x, y, px, py);
		}
		px = x; py = y;
		break;
	}
	case 3:
		f_key(mv, e->key[0], (bool)e->key[1]);
		break;
	}
}

void modelviewer2_init(Modelviewer2* mv, Modelobj* model) {
	mv->wew.data = (void*)mv;
	wlezwrap_confgen(&mv->wew);
	mv->wew.event = f_event;
	mv->resize = true; // swapchain has not been created
	mv->width = 640;
	mv->height = 480;
	mv->present = false;
	wlezwrap_init(&mv->wew);
	camcon_init(&mv->cc);
	vkwayland_new(&mv->vks, mv->wew.wl.display, mv->wew.wl.surface);
	vkbasic_init(&mv->vb, mv->vks.device);
	vkbasic3d_init(&mv->vb3, &mv->vks);
	mv->model = model;
	if (mv->model->n_len == 0) {
		modelobj_normal_build(mv->model);
	}
}

bool modelviewer2_playing(Modelviewer2* mv) {
	if (mv->playing == 0) {
		return false;
	}
	if (mv->playing == 2) {
		mv->playing = 0;
	}
	return true;
}

static void modelviewer2_present(Modelviewer2* mv) {
	if (!mv->present) { return; }
	vkbasic_present(&mv->vb, mv->vks.queue, &mv->index);
	mv->present = false;
}

void modelviewer2_go(Modelviewer2* mv) {
	vkbasic3d_model_upload(&mv->vks, &mv->vb3, mv->model);
	uint32_t width = mv->width;
	uint32_t height = mv->height;
	if (width == 0) { width = 1; }
	if (height == 0) { height = 1; }
	wl_display_dispatch_pending(mv->wew.wl.display);
	if (mv->resize) {
		printf("Resize: %u %u\n", width, height);
		assert(0 == vkDeviceWaitIdle(mv->vks.device));
		vkbasic_swapchain_update(&mv->vb, &mv->vks,
			mv->vb3.renderpass, width, height);
		mv->index = 0;
		mv->resize = false;
		wl_surface_commit(mv->wew.wl.surface);
	}
	float ratio = (float)width / (float)height;
	glm_perspective(0.7f, ratio, 0.05f, 100.0f, mv->vb3.camera->proj);
	camcon_compute(&mv->cc, mv->vb3.camera->view);
	camcon_lookn(&mv->cc, mv->vb3.camera->direction);
	vkbasic_next_index(&mv->vb, mv->vks.device, &mv->index);
	VkCommandBuffer cbuf = vkstatic_begin(&mv->vks);
	vkbasic3d_build_command(&mv->vb3, &mv->vks, cbuf,
		mv->vb.vs.elements[mv->index].framebuffer, width, height);
	vkbasic_submit(&mv->vb, mv->vks.queue, cbuf, &mv->index);
	mv->present = true;
	modelviewer2_present(mv);
	wl_display_roundtrip(mv->wew.wl.display);
}

void modelviewer2_deinit(Modelviewer2* mv) {
	assert(0 == vkDeviceWaitIdle(mv->vks.device));
	vkbasic3d_deinit(&mv->vb3, mv->vks.device);
	vkbasic_deinit(&mv->vb, mv->vks.device, mv->vks.cpool);
	vkstatic_deinit(&mv->vks);
	wlezwrap_deinit(&mv->wew);
}
