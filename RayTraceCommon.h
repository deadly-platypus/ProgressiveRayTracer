#pragma once
#include "App.h"
#include "QuadTree.h"

struct Collector {
	App *app;
	QuadTree *qt;
	int render_index;

	Collector(App *app, QuadTree *qt, int render_index) {
		this->app = app;
		this->qt = qt;
		this->render_index = render_index;
	}

	Collector(){};

};

//void cudaRayTrace(App *app, float *input, float *result);