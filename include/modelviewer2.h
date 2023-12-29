#ifndef INCLUDEGUARD_MODELVIEWER2
#define INCLUDEGUARD_MODELVIEWER2

#include "../../camcon/build/camcon.h"
#include "../../modelobj/build/modelobj.h"
#include "../../vkbasic/include/vkbasic.h"
#include "../../vkbasic3d/include/vkbasic3d.h"
#include "../../wlezwrap/include/wlezwrap.h"

typedef struct {
	Vkstatic vks;
	Vkbasic vb;
	Vkbasic3d vb3;
	Camcon() cc;
	Wlezwrap wew;
	Com_6e5dModelobj* model;

	// controls
	uint32_t index;
	uint8_t playing; // 0: pause 1: play 2: step
	uint32_t width;
	uint32_t height;
	bool quit;
	bool resize;
	bool present;
} Modelviewer2;

void modelviewer2_init(Modelviewer2* mv, Com_6e5dModelobj* model);
bool modelviewer2_playing(Modelviewer2* mv);
void modelviewer2_go(Modelviewer2* mv);
void modelviewer2_deinit(Modelviewer2* mv);

#endif
