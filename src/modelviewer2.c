#include <cglm/cglm.h>
#include <math.h>
#include <vulkan/vulkan.h>
#include <wayland-client.h>

#include "../../vkwayland/include/vkwayland.h"
#include "../include/modelviewer2.h"

static void f_resize(Modelviewer2()* mv, uint32_t w, uint32_t h) {
	mv->width = w;
	mv->height = h;
	mv->resize = true;
}

static void f_quit(Modelviewer2()* mv) {
	mv->quit = true;
}

static void motion(Modelviewer2() *mv, bool *keystate,
	float x, float y, float px, float py
) {
	if (keystate[wlezwrap(lshift)]) {
		vec3 dp = {
			-2e-3f * (float)(x - px) * mv->cc.r,
			-2e-3f * (float)(y - py) * mv->cc.r,
			0.0f,
		};
		camcon(translate)(&mv->cc, dp);
	} else if (keystate[wlezwrap(lctrl)]) {
		vec3 dp = {
			0.0f,
			0.0f,
			2e-3f * (float)(y - py) * mv->cc.r,
		};
		camcon(translate)(&mv->cc, dp);
	} else {
		camcon(rotate)(&mv->cc,
			-5e-3f * (float)(x - px),
			5e-3f * (float)(y - py)
		);
	}
}

static void f_key(Modelviewer2()* mv, uint8_t key, bool pressed) {
	if (!pressed) {
		return;
	}
	vec3 dp = {0};
	if (key == 'i') {
		mv->cc.r *= 0.9f;
		printf("zoomin: %f\n", (double)mv->cc.r);
	} else if (key == 'o') {
		mv->cc.r *= 1.1f;
		printf("zoomout: %f\n", (double)mv->cc.r);
	} else if (key == 'r') {
		camcon(init)(&mv->cc);
		return;
	} else if (key == 's') {
		mv->playing = 2;
		return;
	} else if (key == ' ') {
		if (mv->playing == 1) {
			mv->playing = 0;
		} else {
			mv->playing = 1;
		}
		return;
	} else {
		return;
	}
	camcon(translate)(&mv->cc, dp);
}

static void f_event(void* data, uint8_t type, Wlezwrap(Event) *e) {
	Modelviewer2()* mv = data;
	if (type == 0) {
		f_quit(mv);
	} else if (type == 1) {
		f_resize(mv, e->resize[0], e->resize[1]);
	} else if (type == 2) {
		float x = e->motion[0];
		float y = e->motion[1];
		if (mv->wew.keystate[wlezwrap(mclick)]) {
			motion(mv, mv->wew.keystate, x, y, mv->px, mv->py);
		}
		mv->px = x; mv->py = y;
	} else if (type == 3) {
		f_key(mv, e->key[0], (bool)e->key[1]);
	}
}

void modelviewer2(init)(Modelviewer2()* mv, Com_6e5dModelobj* model) {
	mv->wew.data = (void*)mv;
	wlezwrap(confgen)(&mv->wew);
	mv->wew.event = f_event;
	mv->resize = true; // swapchain has not been created
	mv->width = 640;
	mv->height = 480;
	mv->present = false;
	wlezwrap(init)(&mv->wew);
	camcon(init)(&mv->cc);
	vkwayland(new)(&mv->vks, mv->wew.wl.display, mv->wew.wl.surface);
	vkbasic(init)(&mv->vb, mv->vks.device);
	vkbasic3d(init)(&mv->vb3, &mv->vks);
	mv->model = model;
	if (mv->model->n_len == 0) {
		com_6e5d_modelobj_normal_build(mv->model);
	}
}

bool modelviewer2(playing)(Modelviewer2()* mv) {
	if (mv->playing == 0) {
		return false;
	}
	if (mv->playing == 2) {
		mv->playing = 0;
	}
	return true;
}

static void modelviewer2(present)(Modelviewer2()* mv) {
	if (!mv->present) { return; }
	vkbasic(present)(&mv->vb, mv->vks.queue, &mv->index);
	mv->present = false;
}

void modelviewer2(go)(Modelviewer2()* mv) {
	vkbasic3d(model_upload)(&mv->vks, &mv->vb3, mv->model);
	uint32_t width = mv->width;
	uint32_t height = mv->height;
	if (width == 0) { width = 1; }
	if (height == 0) { height = 1; }
	wl_display_dispatch_pending(mv->wew.wl.display);
	if (mv->resize) {
		printf("Resize: %u %u\n", width, height);
		assert(0 == vkDeviceWaitIdle(mv->vks.device));
		vkbasic(swapchain)_update(&mv->vb, &mv->vks,
			mv->vb3.renderpass, width, height);
		mv->index = 0;
		mv->resize = false;
		wl_surface_commit(mv->wew.wl.surface);
	}
	float ratio = (float)width / (float)height;
	glm_perspective(0.7f, ratio, 0.05f, 100.0f, mv->vb3.camera->proj);
	camcon(compute)(&mv->cc, mv->vb3.camera->view);
	camcon(lookn)(&mv->cc, mv->vb3.camera->direction);
	vkbasic(next)_index(&mv->vb, mv->vks.device, &mv->index);
	VkCommandBuffer cbuf = vkstatic(begin)(&mv->vks);
	vkbasic3d(build)_command(&mv->vb3, &mv->vks, cbuf,
		mv->vb.vs.elements[mv->index].framebuffer, width, height);
	vkbasic(submit)(&mv->vb, mv->vks.queue, cbuf);
	mv->present = true;
	modelviewer2(present)(mv);
	wl_display_roundtrip(mv->wew.wl.display);
}

void modelviewer2(deinit)(Modelviewer2()* mv) {
	assert(0 == vkDeviceWaitIdle(mv->vks.device));
	vkbasic3d(deinit)(&mv->vb3, mv->vks.device);
	vkbasic(deinit)(&mv->vb, mv->vks.device);
	vkstatic(deinit)(&mv->vks);
	wlezwrap(deinit)(&mv->wew);
}
