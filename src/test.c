#include <cglm/cglm.h>
#include <unistd.h>

#include "../../chrono/include/chrono.h"
#include "../../modelobj/build/modelobj.h"
#include "../../shapematch/build/shapematch.h"
#include "../include/modelviewer2.h"

int main(int argc, char** argv) {
	assert(argc >= 2);

	Com_6e5dModelobj model;
	com_6e5d_modelobj_load_file(&model, argv[1]);
	com_6e5d_modelobj_scale(&model, 0.2f);
	vec3 t = {0.0f, 0.4f, 0.0f};
	com_6e5d_modelobj_translate(&model, t);
	Com_6e5dShapematch sm;
	com_6e5d_shapematch_init(&sm, &model);

	Modelviewer2 mv = {0};
	modelviewer2_init(&mv, &model);
	while (!mv.quit) {
		if (modelviewer2_playing(&mv)) {
			com_6e5d_shapematch_step(&sm);
		}
		modelviewer2_go(&mv);
		com_6e5d_chrono_sleep(10000000);
	}

	com_6e5d_shapematch_deinit(&sm);
	com_6e5d_modelobj_deinit(&model);
	modelviewer2_deinit(&mv);
	return 0;
}
