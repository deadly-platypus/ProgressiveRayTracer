/**
  @file App.cpp
 */
#include "App.h"
#include "World.h"
#include "RayTraceCommon.h"

#include <G3D/Image3.h>
#include <G3D/Color4.h>
#include <G3D/Color3.h>
#include <G3D/debugPrintf.h>

#include <GLG3D/GApp.h>
#include <GLG3D/GUI.h>
#include <GLG3D/GuiPane.h>
#include <GLG3D/Surface.h>
#include <GLG3D/Surfel.h>
#include <GLG3D/Light.h>
#include <GLG3D/Texture.h>
#include <GLG3D/Draw.h>
#include <GLG3D/DeveloperWindow.h>
#include <GLG3D/CameraControlWindow.h>

#include <math.h>
#include <random>
#include <cstdlib>
#include <set>

G3D_START_AT_MAIN();

int main(int argc, char** argv) {
    G3D::GApp::Settings settings;
    settings.window.caption     = "Progressive Ray Tracer Demo";
    settings.window.width       = 960; 
    settings.window.height      = 640;

    return App(settings).run();
}

static G3D::Random rnd(0xF018B4D3, false);

App::App(const G3D::GApp::Settings& settings) : 
    GApp(settings),
    m_raysPerPixel(1),
	m_maxBounces(3),
    m_world(NULL),
	threshold(0.05f){
    catchCommonExceptions = false;
	
	this->message("Building the QuadTree...");
	this->tree = new QuadTree(0.0f, 0.0f, settings.window.width, settings.window.height);

	for(int j = 0; j < settings.window.height; j++){
		for(int i = 0; i < settings.window.width; i++){
			this->tree->insert(G3D::Point2(i + 0.5, j + 0.5));
		}
	}

	// The minus one is to remove the very top level node, which we aren't really interested
	this->render_order = std::vector<QuadTree*>(this->tree->size());
	this->tmp_render_order = new std::vector<QuadTree*>();
	this->m_currentImage = G3D::Image3::createEmpty(settings.window.width, settings.window.height);
}

void App::onCleanup() {
    delete m_world;
    m_world = NULL;
}

void App::start_threads(){
	bool started = false;
	if(this->ne_thread) {
		started = this->ne_thread->start();
	}
	if(started) {
		G3D::debugPrintf("%s started\n", this->ne_thread->name());
	} else {
		G3D::debugPrintf("ne_thread NOT started\n");
	}
	started = false;
	if(this->nw_thread) {
		started = this->nw_thread->start();
	}
	if(started) {
		G3D::debugPrintf("%s started\n", this->nw_thread->name());
	} else {
		G3D::debugPrintf("nw_thread NOT started\n");
	}
	started = false;
	if(this->sw_thread) {
		started = this->sw_thread->start();
	}
	if(started) {
		G3D::debugPrintf("%s started\n", this->sw_thread->name());
	} else {
		G3D::debugPrintf("sw_thread NOT started\n");
	}
	started = false;
	if(this->se_thread) {
		started = this->se_thread->start();
	}
	if(started) {
		G3D::debugPrintf("%s started\n", this->se_thread->name());
	} else {
		G3D::debugPrintf("se_thread NOT started\n");
	}
}

void App::check_threads(){
	if(this->ne_thread && this->ne_thread->completed()){
		this->threads.remove(this->ne_thread);
		this->ne_thread = NULL;
	}
	if(this->nw_thread && this->nw_thread->completed()){
		this->threads.remove(this->nw_thread);
		this->nw_thread = NULL;
	}
	if(this->se_thread && this->se_thread->completed()){
		this->threads.remove(this->se_thread);
		this->se_thread = NULL;
	}
	if(this->sw_thread && this->sw_thread->completed()){
		this->threads.remove(this->sw_thread);
		this->sw_thread = NULL;
	}
}

void App::onInit() {
    message("Loading...");
	
    m_world = new World();
	
    showRenderingStats = false;
    createDeveloperHUD();
    developerWindow->setVisible(false);
    developerWindow->cameraControlWindow->setVisible(false);
	m_debugCamera->filmSettings().setAntialiasingEnabled(true);
    m_debugCamera->filmSettings().setContrastToneCurve();

    // Starting position
    m_debugCamera->setFrame(G3D::CFrame::fromXYZYPRDegrees(24.3f, 0.4f, 2.5f, 68.7f, 1.2f, 0.0f));
    m_debugCamera->frame();

    //makeGUI();

	this->renderFirstFrame();
	this->start_threads();
	this->threads.waitForCompletion();
	this->check_threads();

	this->calculateNeighborDiff();
	this->start_threads();
	this->threads.waitForCompletion();
	this->check_threads();

	message("Sorting World...");
	this->sort_render_order();

	this->smallDiffStart = this->render_order.size();

	this->current_mode = App::render_mode::SORT;
}

void App::message(const std::string& msg) const {
    renderDevice->clear();
    renderDevice->push2D();
        debugFont->draw2D(renderDevice, msg, renderDevice->viewport().center(), 12, 
            G3D::Color3::white(), G3D::Color4::clear(), G3D::GFont::XALIGN_CENTER, G3D::GFont::YALIGN_CENTER);
    renderDevice->pop2D();

    // Force update so that we can see the message
    renderDevice->swapBuffers();
}


void App::makeGUI() {
    shared_ptr<G3D::GuiWindow> window = G3D::GuiWindow::create("Controls", debugWindow->theme(), G3D::Rect2D::xywh(0,0,0,0), G3D::GuiTheme::TOOL_WINDOW_STYLE);
    G3D::GuiPane* pane = window->pane();
	pane->addSlider("threshold", &threshold, 2.0f, 0.000005f);
    window->pack();

    window->setVisible(true);
    addWidget(window);
}

void App::onGraphics(G3D::RenderDevice* rd, G3D::Array<shared_ptr<G3D::Surface> >& surface3D, G3D::Array<shared_ptr<G3D::Surface2D> >& surface2D) {
    // Update the preview image only while moving
	if (this->current_mode == App::render_mode::FINISH){
		// Post-process
		shared_ptr<G3D::Texture> src = G3D::Texture::fromImage("Source", m_currentImage);
		m_film->exposeAndRender(renderDevice, m_debugCamera->filmSettings(), src, m_result);
		m_prevCFrame = m_debugCamera->frame();
		this->current_mode = App::render_mode::NONE;
	} else if (this->current_mode != App::render_mode::START && !this->m_prevCFrame.fuzzyEq(this->m_debugCamera->frame())) {
		this->current_mode = App::render_mode::INITIAL;

		if(this->ne_thread != NULL && this->ne_thread->started()){
			this->ne_thread->terminate();
		}
		if(this->nw_thread != NULL && this->nw_thread->started()){
			this->nw_thread->terminate();
		}
		if(this->sw_thread != NULL && this->sw_thread->started()){
			this->sw_thread->terminate();
		}
		if(this->se_thread != NULL && this->se_thread->started()){
			this->se_thread->terminate();
		}
		this->threads.terminate();
		this->threads.waitForCompletion();
		this->threads.clear();

		this->ne_thread = NULL;
		this->nw_thread = NULL;
		this->sw_thread = NULL;
		this->se_thread = NULL;

		this->tmp_render_order->clear();
		this->smallDiffStart = this->render_order.size();
		this->rayTraceImage(1);
		this->m_prevCFrame = this->m_debugCamera->frame();
		
		// There is some bug that is keeping the threads from exiting if they are changed quickly.
		// When this happens, the program stops working, and must be restarted.
		// This is a hack to prevent this
		G3D::System::sleep(.3);
		this->timer.reset();
		this->start_threads();
	} else if(this->threads.size() > 0){
		this->check_threads();
		this->m_result = G3D::Texture::fromImage("Source", m_currentImage);
	} else if (this->threads.size() == 0 && this->current_mode == App::render_mode::INITIAL) {
		this->timer.after("color_quad");
		this->current_mode = App::render_mode::FAST_COLOR;

		this->fastColor();
		this->timer.reset();
		this->start_threads();
	} else if (this->threads.size() == 0 && this->current_mode == App::render_mode::FAST_COLOR) {
		this->timer.after("fast color");
		this->current_mode = App::render_mode::SLOW_COLOR;

		this->slowColor();
		this->start_threads();
	} else if (this->threads.size() == 0 && this->current_mode == App::render_mode::SLOW_COLOR) {
		this->current_mode = App::render_mode::SORT;
	} else if (this->current_mode == App::render_mode::START) {
		this->current_mode = App::render_mode::FINISH;
	} else if (this->threads.size() == 0 && this->current_mode == App::render_mode::SORT) {
		this->current_mode = App::render_mode::SORT_WAITING;

		this->calculateNeighborDiff();
		this->start_threads();
	} else if (this->threads.size() == 0 && this->current_mode == App::render_mode::SORT_WAITING) {
		this->current_mode = App::render_mode::FINISH;
		this->sort_render_order();
	}

    if (m_result) {
        rd->push2D();
        rd->setTexture(0, m_result);
        G3D::Draw::rect2D(rd->viewport(), rd);
        rd->pop2D();
    }

    G3D::Surface2D::sortAndRender(rd, surface2D);
}

void App::sort_render_order() {
	std::priority_queue<QuadTree*, std::vector<QuadTree*>, QuadTreeDiffComparator> tmp(this->tmp_render_order->begin(), this->tmp_render_order->end());
	int counter = 0;
	while(!tmp.empty()){
		this->render_order[counter++] = tmp.top();
		tmp.pop();
	}

}

struct Collector *ne_collector, *nw_collector, *sw_collector, *se_collector;

void fstColor(void *arg) {
	Collector *collector = (Collector*)arg;  
	App *app = collector->app;
	QuadTree *qt = collector->qt;

	if(qt == NULL){
		return;
	} 

	// No need to check for null because that really shouldn't happen
	G3D::Rect2D boundary = app->m_currentImage->rect2DBounds();
	G3D::Color3 average = G3D::Color3::black();
	int index = collector->render_index;
	while(index < app->render_order.size() && qt->neighborColorDiff > app->threshold) {
		qt = app->render_order[index];
		app->order_lock.lock();
		app->tmp_render_order->push_back(qt);
		app->order_lock.unlock();
		
		for(int i = 0; i < qt->points.size(); i++){
			float x = qt->points[i].x;
			float y = qt->points[i].y;
			if(app->m_currentImage->fastGet(x, y) == G3D::Color3::black()){
				G3D::Color3 sum = app->rayTrace(app->getDebugCamera()->worldRay(x, y, boundary), app->m_world);
				qt->points[i].color = sum;
				app->m_currentImage->fastSet(x, y, qt->points[i].color);
				average += app->m_currentImage->fastGet(x, y);
			}
		}

		qt->color = average / qt->points.size();
		index += 4;
	}

	app->diff_lock.lock();
	if(app->smallDiffStart > index){
		app->smallDiffStart = index;
	}
	app->diff_lock.unlock();
}

void App::fastColor(){
	if(this->ne_thread == NULL) {
		if(ne_collector){
			delete ne_collector;
			delete nw_collector;
			delete sw_collector;
			delete se_collector;
		}
		ne_collector = new Collector(this, this->render_order[0], 0);
		nw_collector = new Collector(this, this->render_order[1], 1);
		sw_collector = new Collector(this, this->render_order[2], 2);
		se_collector = new Collector(this, this->render_order[3], 3);

		this->ne_thread = G3D::GThread::create("fastColor_ne_thread", &fstColor, (void*)ne_collector);
		this->nw_thread = G3D::GThread::create("fastColor_nw_thread", &fstColor, (void*)nw_collector);
		this->sw_thread = G3D::GThread::create("fastColor_sw_thread", &fstColor, (void*)sw_collector);
		this->se_thread = G3D::GThread::create("fastColor_se_thread", &fstColor, (void*)se_collector);

		this->threads.insert(this->ne_thread);
		this->threads.insert(this->nw_thread);
		this->threads.insert(this->sw_thread);
		this->threads.insert(this->se_thread);
	}
}

void slwColor(void *arg){
	Collector *collector = (Collector*)arg;  
	App *app = collector->app;

	G3D::Rect2D boundary = app->m_currentImage->rect2DBounds();

	int index = collector->render_index;
	while(index < app->render_order.size()){
		QuadTree *qt = app->render_order[index];
		if(qt != NULL) {
			G3D::Color3 average = G3D::Color3::black();
			app->order_lock.lock();
			app->tmp_render_order->push_back(qt);
			app->order_lock.unlock();

			for(int i = 0; i < qt->points.size(); i++){
				float x = qt->points[i].x;
				float y = qt->points[i].y;
				if(app->m_currentImage->fastGet(x, y) == G3D::Color3::black()){
					G3D::Color3 sum = app->rayTrace(app->getDebugCamera()->worldRay(x, y, boundary), app->m_world);
					qt->points[i].color = sum;
					app->m_currentImage->fastSet(x, y, qt->points[i].color);
				}

				average += app->m_currentImage->fastGet(x, y) / qt->points.size();
			}
			qt->color = average;			
		}
		index += 4;
	}
}

void App::slowColor(){
	if(this->ne_thread == NULL) {
		if(ne_collector){
			delete ne_collector;
			delete nw_collector;
			delete sw_collector;
			delete se_collector;
		}
		ne_collector = new Collector(this, this->render_order[this->smallDiffStart], this->smallDiffStart);
		nw_collector = new Collector(this, this->render_order[this->smallDiffStart + 1], this->smallDiffStart + 1);
		sw_collector = new Collector(this, this->render_order[this->smallDiffStart + 2], this->smallDiffStart + 2);
		se_collector = new Collector(this, this->render_order[this->smallDiffStart + 3], this->smallDiffStart + 3);

		this->ne_thread = G3D::GThread::create("slowColor_ne_thread", &slwColor, (void*)ne_collector);
		this->nw_thread = G3D::GThread::create("slowColor_nw_thread", &slwColor, (void*)nw_collector);
		this->sw_thread = G3D::GThread::create("slowColor_sw_thread", &slwColor, (void*)sw_collector);
		this->se_thread = G3D::GThread::create("slowColor_se_thread", &slwColor, (void*)se_collector);

		this->threads.insert(this->ne_thread);
		this->threads.insert(this->nw_thread);
		this->threads.insert(this->sw_thread);
		this->threads.insert(this->se_thread);
	}
}

void firstFrame(void *arg){
	Collector *collector = (Collector*)arg;  
	App *app = collector->app;
	QuadTree *qt = collector->qt;

	if(qt == NULL){
		return;
	}
	app->order_lock.lock();
	app->tmp_render_order->push_back(qt);
	app->order_lock.unlock();

	G3D::Rect2D boundary = app->window()->clientRect();
	G3D::Color3 average = G3D::Color3::black();

	for(int i = 0; i < qt->points.size(); i++){
		G3D::Color3 sum = app->rayTrace(app->getDebugCamera()->worldRay(qt->points[i].x, qt->points[i].y, boundary), app->m_world);
		qt->points[i].color = sum;
		app->m_currentImage->fastSet(qt->points[i].x, qt->points[i].y, sum);
		average += sum;
	}

	qt->color = average / qt->points.size();

	struct Collector tmp;
	tmp.app = app;

	tmp.qt = qt->ne;
	firstFrame((void*)&tmp);

	tmp.qt = qt->nw;
	firstFrame((void*)&tmp);

	tmp.qt = qt->sw;
	firstFrame((void*)&tmp);

	tmp.qt = qt->se;
	firstFrame((void*)&tmp);
}

void App::renderFirstFrame() {
	if(this->ne_thread == NULL) {
		if(ne_collector){
			delete ne_collector;
			delete nw_collector;
			delete sw_collector;
			delete se_collector;
		}
		ne_collector = new Collector(this, this->tree->ne, 0);
		nw_collector = new Collector(this, this->tree->nw, 0);
		sw_collector = new Collector(this, this->tree->sw, 0);
		se_collector = new Collector(this, this->tree->se, 0);

		this->ne_thread = G3D::GThread::create("firstFrame_ne_thread", &firstFrame, (void*)ne_collector);
		this->nw_thread = G3D::GThread::create("firstFrame_nw_thread", &firstFrame, (void*)nw_collector);
		this->sw_thread = G3D::GThread::create("firstFrame_sw_thread", &firstFrame, (void*)sw_collector);
		this->se_thread = G3D::GThread::create("firstFrame_se_thread", &firstFrame, (void*)se_collector);

		this->threads.insert(this->ne_thread);
		this->threads.insert(this->nw_thread);
		this->threads.insert(this->sw_thread);
		this->threads.insert(this->se_thread);
	}
}

void color_quad(void *arg) {
	Collector *collector = (Collector*)arg;  
	App *app = collector->app;
	QuadTree *qt = collector->qt;

	if(qt == NULL){
		return;
	} 
	
	int index = collector->render_index;
	while(index < app->render_order.size() && qt->neighborColorDiff > app->threshold) {
		qt = app->render_order[index];
		G3D::Color3 whole = app->rayTrace(app->getDebugCamera()->worldRay(qt->boundary->center().x, qt->boundary->center().y, app->m_currentImage->rect2DBounds()), app->m_world);

		app->m_currentImage->fastSet(qt->boundary->center().x, qt->boundary->center().y, whole);
		index += 4;
	}

	app->diff_lock.lock();
	if(app->smallDiffStart > index){
		app->smallDiffStart = index;
	}
	app->diff_lock.unlock();
}

void App::rayTraceImage(int numRays) {
	int width = int(window()->width());
	int height = int(window()->height());

	if(this->ne_thread == NULL){
		if(ne_collector){
			delete ne_collector;
			delete nw_collector;
			delete sw_collector;
			delete se_collector;
		}

		ne_collector = new Collector(this, this->render_order[0], 0);
		nw_collector = new Collector(this, this->render_order[1], 1);
		sw_collector = new Collector(this, this->render_order[2], 2);
		se_collector = new Collector(this, this->render_order[3], 3);

		this->ne_thread = G3D::GThread::create("rayTraceImage_ne_thread", &color_quad, (void*)ne_collector);
		this->nw_thread = G3D::GThread::create("rayTraceImage_nw_thread", &color_quad, (void*)nw_collector);
		this->sw_thread = G3D::GThread::create("rayTraceImage_sw_thread", &color_quad, (void*)sw_collector);
		this->se_thread = G3D::GThread::create("rayTraceImage_se_thread", &color_quad, (void*)se_collector);

		this->threads.insert(this->ne_thread);
		this->threads.insert(this->nw_thread);
		this->threads.insert(this->sw_thread);
		this->threads.insert(this->se_thread);
	}

	m_currentImage = G3D::Image3::createEmpty(width, height);
    m_currentRays = numRays;
}

void calc_neighbor_diff(void *arg){
	Collector *collector = (Collector*)arg;  
	App *app = collector->app;
	QuadTree *qt = collector->qt;

	if(qt == NULL){
		return;
	} else if(qt->ne == NULL){
		qt->neighborColorDiff = 0.0;
		return;
	}

	G3D::Color3 sum = qt->ne->color - qt->color;
	sum += qt->nw->color - qt->color;
	sum += qt->sw->color - qt->color;
	sum += qt->sw->color - qt->color;

	qt->neighborColorDiff = sqrt((sum / 4).squaredLength());
	if(qt->neighborColorDiff > 10.0f){
		qt->neighborColorDiff /= 100.0f;
	}

	struct Collector tmp;
	tmp.app = app;

	tmp.qt = qt->ne;
	calc_neighbor_diff((void*)&tmp);

	tmp.qt = qt->nw;
	calc_neighbor_diff((void*)&tmp);

	tmp.qt = qt->sw;
	calc_neighbor_diff((void*)&tmp);

	tmp.qt = qt->se;
	calc_neighbor_diff((void*)&tmp);
}

void App::calculateNeighborDiff(){
	if(this->ne_thread == NULL) {
		if(ne_collector){
			delete ne_collector;
			delete nw_collector;
			delete sw_collector;
			delete se_collector;
		}
		ne_collector = new Collector(this, this->tree->ne, 0);
		nw_collector = new Collector(this, this->tree->nw, 0);
		sw_collector = new Collector(this, this->tree->sw, 0);
		se_collector = new Collector(this, this->tree->se, 0);

		this->ne_thread = G3D::GThread::create("neighborDiff_ne_thread", &calc_neighbor_diff, (void*)ne_collector);
		this->nw_thread = G3D::GThread::create("neighborDiff_nw_thread", &calc_neighbor_diff, (void*)nw_collector);
		this->sw_thread = G3D::GThread::create("neighborDiff_sw_thread", &calc_neighbor_diff, (void*)sw_collector);
		this->se_thread = G3D::GThread::create("neighborDiff_se_thread", &calc_neighbor_diff, (void*)se_collector);

		this->threads.insert(this->ne_thread);
		this->threads.insert(this->nw_thread);
		this->threads.insert(this->sw_thread);
		this->threads.insert(this->se_thread);
	}
}

G3D::Radiance3 App::rayTrace(const G3D::Ray& ray, World* world, int bounce) {
    G3D::Radiance3 radiance = G3D::Radiance3::zero();
	const G3D::Radiance3 white = G3D::Radiance3::white();
    const float BUMP_DISTANCE = 0.0001f;

    float dist = (float)G3D::inf();
    const shared_ptr<G3D::Surfel>& surfel = world->intersect(ray, dist);

    if (G3D::notNull(surfel)) {
		// Shade this point (direct illumination)
		for (int L = 0; L < world->lightArray.size(); ++L) {
			const shared_ptr<G3D::Light>& light = world->lightArray[L];

			// Shadow rays
			if (world->lineOfSight(surfel->location + surfel->geometricNormal * BUMP_DISTANCE, light->position().xyz())) {
				G3D::Vector3 w_i = light->position().xyz() - surfel->location;
				const float distance2 = w_i.squaredLength();
				w_i /= sqrt(distance2);

				// Biradiance
				const G3D::Biradiance3& B_i = light->color / (4.0f * G3D::pif() * distance2);

				radiance += 
					surfel->finiteScatteringDensity(w_i, -ray.direction()) * 
					B_i *
					G3D::max(0.0f, w_i.dot(surfel->shadingNormal));

				debugAssert(radiance.isFinite());
			}
		}

        // Specular
        if (bounce < m_maxBounces) {
            // Perfect reflection and refraction
            G3D::Surfel::ImpulseArray impulseArray;
            surfel->getImpulses(G3D::PathDirection::EYE_TO_SOURCE, -ray.direction(), impulseArray);
                
            for (int i = 0; i < impulseArray.size(); ++i) {
                const G3D::Surfel::Impulse& impulse = impulseArray[i];
                // Bump along normal *in the outgoing ray direction*. 
                const G3D::Vector3& offset = surfel->geometricNormal * G3D::sign(impulse.direction.dot(surfel->geometricNormal)) * BUMP_DISTANCE;
                const G3D::Ray& secondaryRay = G3D::Ray::fromOriginAndDirection(surfel->location + offset, impulse.direction);
                debugAssert(secondaryRay.direction().isFinite());
                radiance += rayTrace(secondaryRay, world, bounce + 1) * impulse.magnitude;
                debugAssert(radiance.isFinite());
            }
        }
    } else {
        // Hit the sky
        radiance = world->ambient;
    }

    return radiance;
}
