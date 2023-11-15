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

static void f_resize(void* data, uint32_t w, uint32_t h) {
	Modelviewer2* mv = data;
	mv->width = w;
	mv->height = h;
	mv->resize = true;
}

static void f_quit(void* data) {
	Modelviewer2* mv = data;
	mv->quit = true;
}

static void f_motion(void* data, double x, double y, double pressure) {
	Modelviewer2* mv = data;
	if (mv->mouse_state == 0) {
		mv->drag = 0;
		return;
	}
	if (!mv->drag) {
		mv->px = x;
		mv->py = y;
		mv->drag = 1;
		return;
	}
	if (mv->mouse_state == 10) {
		camcon_rotate(&mv->cc,
			-3e-3f * (float)(x - mv->px),
			3e-3f * (float)(y - mv->py)
		);
	} else if (mv->mouse_state == 11) {
		vec3 dp = {
			1e-3f * (float)(x - mv->px) * mv->cc.r,
			1e-3f * (float)(y - mv->py) * mv->cc.r,
			0.0f,
		};
		camcon_translate(&mv->cc, dp);
	} else if (mv->mouse_state == 12) {
		vec3 dp = {
			0.0f,
			0.0f,
			1e-3f * (float)(y - mv->py) * mv->cc.r,
		};
		camcon_translate(&mv->cc, dp);
	}
	mv->px = x;
	mv->py = y;
}

static void f_button(void* data, uint8_t button, bool state) {
	Modelviewer2* mv = data;
	if (!state) {
		mv->mouse_state -= 10;
	} else if (button == 1) {
		if (mv->mouse_state == 1) {
			mv->mouse_state = 11;
		} else if (mv->mouse_state == 2) {
			mv->mouse_state = 12;
		} else {
			mv->mouse_state = 10;
		}
	}
}

static void f_key(void* data, char ch, bool pressed) {
	Modelviewer2* mv = data;
	if (!pressed) {
		if (ch < 0) {
			mv->mouse_state = 0;
		}
		return;
	}
	vec3 dp = {0};
	switch (ch) {
	case -1:
		mv->mouse_state = 1;
		return;
	case -2:
		mv->mouse_state = 2;
		return;
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

void modelviewer2_init(Modelviewer2* mv, Modelobj* model) {
	mv->wew.data = (void*)mv;
	wlezwrap_confgen(&mv->wew);
	mv->wew.f_resize = f_resize;
	mv->wew.f_quit = f_quit;
	mv->wew.f_motion = f_motion;
	mv->wew.f_button = f_button;
	mv->wew.f_key = f_key;
	mv->resize = true; // swapchain has not been created
	mv->width = 640;
	mv->height = 480;
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
