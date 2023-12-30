#include <cglm/cglm.h>
#include <unistd.h>

#include "../../chrono/include/chrono.h"
#include "../../modelobj/build/modelobj.h"
#include "../../shapematch/build/shapematch.h"
#include "../include/modelviewer2.h"

int main(int argc, char** argv) {
	assert(argc >= 2);

	Modelobj() model;
	modelobj(load_file)(&model, argv[1]);
	modelobj(scale)(&model, 0.2f);
	vec3 t = {0.0f, 0.4f, 0.0f};
	modelobj(translate)(&model, t);
	Shapematch() sm;
	shapematch(init)(&sm, &model);

	Modelviewer2() mv = {0};
	modelviewer2(init)(&mv, &model);
	while (!mv.quit) {
		if (modelviewer2(playing)(&mv)) {
			shapematch(step)(&sm);
		}
		modelviewer2(go)(&mv);
		chrono(sleep)(10000000);
	}

	shapematch(deinit)(&sm);
	modelobj(deinit)(&model);
	modelviewer2(deinit)(&mv);
	return 0;
}
