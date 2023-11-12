#include <assert.h>
#include <math.h>
#include <cglm/cglm.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <vulkan/vulkan.h>
#include <wayland-client.h>

#include "../../camcon/include/camcon.h"
#include "../../vkbasic/include/vkbasic.h"
#include "../../vkstatic/include/vkstatic.h"
#include "../../vkwayland/include/vkwayland.h"
#include "../../vkbasic3d/include/vkbasic3d.h"
#include "../../vkbasic3d/include/model.h"
#include "../../wlezwrap/include/wlezwrap.h"
#include "../../modelobj/include/modelobj.h"
#include "../../modelobj/include/normal.h"
#include "../../modelobj/include/transform.h"
#include "../../shapematch/include/model.h"

static Modelobj model;
static Vkstatic vks;
static Vkbasic vb;
static Vkbasic3d vb3;
static Camcon cc;
static Shapematch sm;
static bool playing;
static uint32_t index = 0;

static Wlezwrap wew;
uint32_t width = 640, height = 480;
bool quit, resize = true;
static uint8_t mouse_state;
static float px, py;
static uint32_t pp;

static void f_resize(void* data, uint32_t w, uint32_t h) {
	width = w;
	height = h;
	resize = true;
}
static void f_quit(void* data) {
	quit = true;
}
static void f_motion(void* data, float x, float y) {
	if (mouse_state == 0) { pp = 0; return; }
	if (pp) {
		camcon_rotate(&cc, -0.0001f * (x - px), 0.0001f * (y - py));
	} else { pp = 1; }
	px = (float)x;
	py = (float)y;
}
static void f_button(void* data, uint8_t button, bool state) {
	if (!state) {
		mouse_state = 0;
	} else if (state) {
		mouse_state = 1;
	}
	if (state == 0) {
		pp = 0;
	}
}
static void f_key(void* data, char ch, bool pressed) {
	if (!pressed) { return; }
	vec3 dp = {0};
	float k = cc.r * 0.1f;
	switch (ch) {
	case 'i':
		cc.r *= 0.9f;
		return;
	case 'o':
		cc.r *= 1.1f;
		return;
	case 'h':
		dp[0] -= k;
		break;
	case 'l':
		dp[0] += k;
		break;
	case 'j':
		dp[1] -= k;
		break;
	case 'k':
		dp[1] += k;
		break;
	case ' ':
		playing = !playing;
		return;
	default:
		return;
	}
	camcon_transpose(&cc, dp);
}

static void handle_resize(void) {
	if (!resize) { return; }
	printf("Resize: %u %u\n", width, height);
	assert(0 == vkDeviceWaitIdle(vks.device));
	vkbasic_swapchain_update(&vb, &vks, vb3.renderpass, width, height);
	index = 0;
	resize = false;
	wl_surface_commit(wew.wl.surface);
	vb3.recreate_pipeline = true;
}

int main(int argc, char** argv) {
	assert(argc >= 2);
	wlezwrap_confgen(&wew);
	wew.f_resize = f_resize;
	wew.f_quit = f_quit;
	wew.f_motion = f_motion;
	wew.f_button = f_button;
	wew.f_key = f_key;
	wlezwrap_init(&wew);
	camcon_init(&cc);
	vkwayland_new(&vks, wew.wl.display, wew.wl.surface);
	vkbasic_init(&vb, vks.device);
	vkbasic3d_init(&vb3, &vks);
	modelobj_load_file(&model, argv[1]);
	modelobj_normal_build(&model);
	// modelobj_normal_smooth(&model);
	modelobj_scale(&model, 0.1f);
	vec3 t = {0.0f, 1.0f, 0.0f};
	modelobj_transpose(&model, t);
	shapematch_init(&sm, &model);
	vkbasic3d_model_upload(&vks, &vb3, &model);
	while (!quit) {
		if (playing) {
			shapematch_step(&sm);
		}
		vkbasic3d_model_upload(&vks, &vb3, &model);
		handle_resize();
		vkbasic_next_index(&vb, vks.device, &index);
		float ratio = (float)width / (float)height;
		glm_perspective(1.0f, ratio, 0.01f, 100.0f, vb3.camera->proj);
		camcon_compute(&cc, vb3.camera->view);
		camcon_lookn(&cc, vb3.camera->direction);
		vkbasic3d_build_command(&vb3, vks.device, vks.cbuf,
			vb.vs.elements[index].framebuffer, width, height);
		assert(0 == vkEndCommandBuffer(vks.cbuf));
		vkbasic_present(&vb, vks.queue, vks.cbuf, &index);
		wl_display_roundtrip(wew.wl.display);
		usleep(10000);
	}
	assert(0 == vkDeviceWaitIdle(vks.device));

	shapematch_deinit(&sm);
	modelobj_deinit(&model);
	vkbasic3d_deinit(&vb3, vks.device);
	vkbasic_deinit(&vb, vks.device, vks.cpool);
	vkstatic_deinit(&vks);
	wlezwrap_deinit(&wew);
	return 0;
}
