#include "../../camcon/build/camcon.h"
#include "../../modelobj/build/modelobj.h"
#include "../../vkbasic/include/vkbasic.h"
#include "../../vkbasic3d/include/vkbasic3d.h"
#include "../../vkstatic/include/vkstatic.h"
#include "../../wlezwrap/include/wlezwrap.h"

typedef struct {
	Vkstatic() vks;
	Vkbasic() vb;
	Vkbasic3d() vb3;
	Camcon() cc;
	Wlezwrap() wew;
	Modelobj()* model;

	// controls
	uint32_t index;
	uint8_t playing; // 0: pause 1: play 2: step
	uint32_t width;
	uint32_t height;
	bool quit;
	bool resize;
	bool present;
	float px;
	float py;
} Modelviewer2();

void modelviewer2(init)(Modelviewer2()* mv, Modelobj()* model);
bool modelviewer2(playing)(Modelviewer2()* mv);
void modelviewer2(go)(Modelviewer2()* mv);
void modelviewer2(deinit)(Modelviewer2()* mv);
