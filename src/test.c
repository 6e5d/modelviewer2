#include <assert.h>
#include <cglm/cglm.h>
#include <unistd.h>

#include "../../modelobj/include/transform.h"
#include "../../shapematch/include/model.h"
#include "../include/modelviewer2.h"

int main(int argc, char** argv) {
	assert(argc >= 2);

	Modelobj model;
	modelobj_load_file(&model, argv[1]);
	modelobj_scale(&model, 0.2f);
	vec3 t = {0.0f, 0.4f, 0.0f};
	modelobj_translate(&model, t);
	Shapematch sm;
	shapematch_init(&sm, &model);

	Modelviewer2 mv = {0};
	modelviewer2_init(&mv, &model);
	while (!mv.quit) {
		if (modelviewer2_playing(&mv)) {
			shapematch_step(&sm);
		}
		modelviewer2_go(&mv);
		usleep(10000);
	}

	shapematch_deinit(&sm);
	modelobj_deinit(&model);
	modelviewer2_deinit(&mv);
	return 0;
}
