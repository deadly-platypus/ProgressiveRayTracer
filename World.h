/**
  @file World.h
  @author Morgan McGuire, http://graphics.cs.williams.edu
 */
#ifndef World_h
#define World_h

//#include <G3D\G3DAll.h>

#include <G3D/Array.h>
#include <G3D/Color3.h>
#include <G3D/Vector3.h>
#include <G3D/CoordinateFrame.h>
#include <G3D/Ray.h>

#include <GLG3D/Tri.h>
#include <GLG3D/Surface.h>
#include <GLG3D/TriTree.h>
#include <GLG3D/CPUVertexArray.h>
#include <GLG3D/Light.h>
#include <GLG3D/ArticulatedModel.h>

/** \brief The scene.*/
class World {
private:

    G3D::Array<G3D::Tri>					m_triArray;
    G3D::Array<shared_ptr<G3D::Surface> >   m_surfaceArray;
    G3D::TriTree							m_triTree;
    G3D::CPUVertexArray						m_cpuVertexArray;
    enum Mode {TRACE, INSERT}				m_mode;

public:

    G3D::Array<shared_ptr<G3D::Light> >		lightArray;
    G3D::Color3								ambient;

    World();

    /** Returns true if there is an unoccluded line of sight from v0
        to v1.  This is sometimes called the visibilty function in the
        literature.*/
    bool lineOfSight(const G3D::Vector3& v0, const G3D::Vector3& v1) const;

    void begin();
    void insert(const shared_ptr<G3D::ArticulatedModel>& model, const G3D::CFrame& frame = G3D::CFrame());
    void insert(const shared_ptr<G3D::Surface>& m);
    void end();

    /**\brief Trace the ray into the scene and return the first
       surface hit.

       \param ray In world space 

       \param distance On input, the maximum distance to trace to.  On
       output, the distance to the closest surface.

       \return The surfel hit, or NULL if none.
     */
    shared_ptr<G3D::Surfel> intersect(const G3D::Ray& ray, float& distance) const;
};

#endif
