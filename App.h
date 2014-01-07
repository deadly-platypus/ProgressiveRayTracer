/**
  @file App.h

  This is a simple ray tracing demo showing how to use the G3D ray tracing 
  primitives.  It runs fast enough for real-time flythrough of 
  a 100k scene at low resolution. At a loss of simplicity, it could be made
  substantially faster using adaptive refinement and multithreading.
 */
#ifndef App_h
#define App_h

#include <G3D/Random.h>
#include <G3D/CoordinateFrame.h>
#include <G3D/Color3.h>
#include <G3D/ThreadSet.h>
#include <G3D/Image3.h>
#include <G3D/Ray.h>
#include <G3D/Stopwatch.h>

#include <GLG3D/GApp.h>
#include <GLG3D/Texture.h>
#include <GLG3D/RenderDevice.h>
#include <GLG3D/Surface.h>
#include <GLG3D/Camera.h>
#include <GLG3D/Film.h>

#include <queue>
#include <set>

#include "World.h"
#include "QuadTree.h"

class World;

class App : public G3D::GApp {
private:
    int                 m_raysPerPixel;
	int					m_maxBounces;

	QuadTree			*tree;

    G3D::Random         m_rng;
	G3D::Stopwatch		timer;

    /** Position during the previous frame */
    G3D::CFrame         m_prevCFrame;

	G3D::GThreadRef		ne_thread;
	G3D::GThreadRef		nw_thread;
	G3D::GThreadRef		sw_thread;
	G3D::GThreadRef		se_thread;

    /** Called from onInit() */
    void makeGUI();

    /** Trace a whole image. */
    void rayTraceImage(int numRays);
	void fastColor();
	void slowColor();
	void renderFirstFrame();
	void calculateNeighborDiff();

    /** Show a full-screen message */
    void message(const std::string& msg) const;

	G3D::Radiance3 performDof(const G3D::Ray& ray, World* world);

	void start_threads();
	void check_threads();

public:
	static enum render_mode { START, INITIAL, FAST_COLOR, SLOW_COLOR, FINISH, SORT, SORT_WAITING, NONE };

	render_mode current_mode;

	/** Used to pass information from rayTraceImage() to trace() */
    shared_ptr<G3D::Image3>		m_currentImage;
	World*						m_world;
	/** Used to pass information from rayTraceImage() to trace() */
    int							m_currentRays;
	shared_ptr<G3D::Texture>	m_result;
	int							smallDiffStart;
	G3D::ThreadSet				threads;
	G3D::GMutex					order_lock;
	G3D::GMutex					diff_lock;
	float						threshold;

	//std::priority_queue<QuadTree*, std::vector<QuadTree*>, QuadTreeComparator> *render_queue;
	std::vector<QuadTree*> render_order;
	std::vector<QuadTree*> *tmp_render_order;

    App(const GApp::Settings& settings = GApp::Settings());

    virtual void onInit();

    virtual void onGraphics(G3D::RenderDevice* rd, G3D::Array<shared_ptr<G3D::Surface> >& posed3D, G3D::Array<shared_ptr<G3D::Surface2D> >& posed2D);
    virtual void onCleanup();

	/** Trace a single ray backwards. */
    G3D::Radiance3 rayTrace(const G3D::Ray& ray, World* world, int bounces = 1);

	shared_ptr<G3D::Camera> getDebugCamera() { return m_debugCamera; }
	shared_ptr<G3D::Film> getFilm() { return m_film; }

	void sort_render_order();
};

#endif
