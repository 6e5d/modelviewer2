#ifndef INCLUDEGUARD_MODELVIEWER2H
#define INCLUDEGUARD_MODELVIEWER2H

#include "../../camcon/include/camcon.h"
#include "../../modelobj/include/modelobj.h"
#include "../../vkbasic3d/include/vkbasic3d.h"
#include "../../wlezwrap/include/wlezwrap.h"

typedef struct {
	Vkstatic vks;
	Vkbasic vb;
	Vkbasic3d vb3;
	Camcon cc;
	Wlezwrap wew;
	Modelobj* model;

	// controls
	uint32_t index;
	double px;
	double py;
	uint8_t mouse_state;
	uint8_t playing; // 0: pause 1: play 2: step
	uint32_t width;
	uint32_t height;
	bool quit;
	bool resize;
	bool drag;
} Modelviewer2;

void modelviewer2_init(Modelviewer2* mv, Modelobj* model);
bool modelviewer2_playing(Modelviewer2* mv);
void modelviewer2_go(Modelviewer2* mv);
void modelviewer2_deinit(Modelviewer2* mv);

#endif
