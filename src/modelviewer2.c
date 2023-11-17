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

static void mview_event(WlezwrapMview* wewmv, double x, double y) {
	Modelviewer2* mv = (Modelviewer2*)wewmv->data;
	if (wewmv->button == 0) {
		camcon_rotate(&mv->cc,
			-3e-3f * (float)(x - wewmv->px),
			3e-3f * (float)(y - wewmv->py)
		);
	} else if (wewmv->button == 1) {
		vec3 dp = {
			1e-3f * (float)(x - wewmv->px) * mv->cc.r,
			1e-3f * (float)(y - wewmv->py) * mv->cc.r,
			0.0f,
		};
		camcon_translate(&mv->cc, dp);
	} else if (wewmv->button == 2) {
		vec3 dp = {
			0.0f,
			0.0f,
			1e-3f * (float)(y - wewmv->py) * mv->cc.r,
		};
		camcon_translate(&mv->cc, dp);
	}
}

static void f_key(Modelviewer2* mv, int8_t key, bool pressed) {
	if (!pressed) {
		return;
	}
	vec3 dp = {0};
	switch (key) {
	case 'i':
		mv->cc.r *= 0.9f;
		break;
	case 'o':
		mv->cc.r *= 1.1f;
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
	Modelviewer2* mv = data;
	wlezwrap_mview_update(&mv->mview, type, e);
	switch(type) {
	case 0:
		f_quit(mv);
		break;
	case 1:
		f_resize(mv, e->resize[0], e->resize[1]);
		break;
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
	wlezwrap_init(&mv->wew);
	mv->mview.event = mview_event;
	mv->mview.data = mv;
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

void modelviewer2_go(Modelviewer2* mv) {
	vkbasic3d_model_upload(&mv->vks, &mv->vb3, mv->model);
	uint32_t width = mv->width;
	uint32_t height = mv->height;
	if (width == 0) { width = 1; }
	if (height == 0) { height = 1; }
	if (mv->resize) {
		printf("Resize: %u %u\n", width, height);
		assert(0 == vkDeviceWaitIdle(mv->vks.device));
		vkbasic_swapchain_update(&mv->vb, &mv->vks,
			mv->vb3.renderpass, width, height);
		mv->index = 0;
		mv->resize = false;
		wl_surface_commit(mv->wew.wl.surface);
		mv->vb3.recreate_pipeline = true;
	}
	vkbasic_next_index(&mv->vb, mv->vks.device, &mv->index);
	float ratio = (float)width / (float)height;
	glm_perspective(0.7f, ratio, 0.05f, 100.0f, mv->vb3.camera->proj);
	camcon_compute(&mv->cc, mv->vb3.camera->view);
	camcon_lookn(&mv->cc, mv->vb3.camera->direction);
	vkbasic3d_build_command(&mv->vb3, &mv->vks, mv->vks.cbuf,
		mv->vb.vs.elements[mv->index].framebuffer, width, height);
	vkbasic_present(&mv->vb, mv->vks.queue, mv->vks.cbuf, &mv->index);
	wl_display_roundtrip(mv->wew.wl.display);
}

void modelviewer2_deinit(Modelviewer2* mv) {
	assert(0 == vkDeviceWaitIdle(mv->vks.device));
	vkbasic3d_deinit(&mv->vb3, mv->vks.device);
	vkbasic_deinit(&mv->vb, mv->vks.device, mv->vks.cpool);
	vkstatic_deinit(&mv->vks);
	wlezwrap_deinit(&mv->wew);
}
