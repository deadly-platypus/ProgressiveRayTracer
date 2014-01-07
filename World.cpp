#include <G3D/Vector3.h>
#include <G3D/Color3.h>
#include <G3D/Any.h>
#include <G3D/CoordinateFrame.h>
#include <G3D/Stopwatch.h>
#include <G3D/Array.h>

#include <GLG3D/Light.h>
#include <GLG3D/ArticulatedModel.h>

#include "World.h"

World::World() : m_mode(TRACE) {
    begin();

    lightArray.append(G3D::Light::point("Light1", G3D::Vector3(0, 10, 0), G3D::Color3::white() * 1200));
    lightArray.append(G3D::Light::point("Light2", G3D::Vector3(22.6f, 2.9f,  6.6f), G3D::Color3::fromARGB(0xffe5bd) * 1000));

    ambient = G3D::Radiance3::fromARGB(0x304855) * 0.3f;
	
    G3D::Any teapot = G3D::PARSE_ANY
        ( G3D::ArticulatedModel::Specification {
            filename = "teapot/teapot.obj";
            scale = 0.01;
            stripMaterials = true;
            preprocess = 
                ( setMaterial(all(), 
                             UniversalMaterial::Specification {
                                 specular = Color3(0.2f);
                                 shininess = mirror();
                                 lambertian = Color3(0.7f, 0.5f, 0.1f);
                             });
                 );
         } );

    insert(G3D::ArticulatedModel::create(teapot), G3D::CFrame::fromXYZYPRDegrees(19.4f, -0.2f, 0.94f, 70));
    G3D::Any sphereOutside = G3D::PARSE_ANY 
        ( ArticulatedModel::Specification {
            filename = "sphere.ifs";
            scale = 0.3;
            preprocess =
                ( setTwoSided(all(),  true);
                    setMaterial(all(), 
                              UniversalMaterial::Specification {
                                  specular = Color3(0.1f);
                                  shininess = mirror();
                                  lambertian = Color3(0.0f);
                                  etaTransmit = 1.3f;
                                  etaReflect = 1.0f;
                                  transmissive = Color3(0.2f, 0.5f, 0.7f);
                              });
                  );
          });

    G3D::Any sphereInside = G3D::PARSE_ANY 
        ( G3D::ArticulatedModel::Specification {
            filename = "sphere.ifs";
            scale = -0.3;
            preprocess =
                ( setTwoSided(all(), true);
                    setMaterial(all(),
                              UniversalMaterial::Specification {
                                  specular = Color3(0.1f);
                                  shininess = mirror();
                                  lambertian = Color3(0.0f);
                                  etaReflect = 1.3f;
                                  etaTransmit = 1.0f;
                                  transmissive = Color3(1.0f);
                              });
                  );
          });
	
    insert(G3D::ArticulatedModel::create(sphereOutside), G3D::CFrame::fromXYZYPRDegrees(19.7f, 0.2f, -1.1f, 70));
    insert(G3D::ArticulatedModel::create(sphereInside),  G3D::CFrame::fromXYZYPRDegrees(19.7f, 0.2f, -1.1f, 70));
	
    G3D::Any sponza = G3D::PARSE_ANY
        ( ArticulatedModel::Specification {
            filename = "dabrovic_sponza/sponza.zip/sponza.obj";
          } );
    G3D::Stopwatch timer;
    insert(G3D::ArticulatedModel::create(sponza), G3D::Vector3(8.2f, -6, 0));
    timer.after("Sponza load");
    end();
}


void World::begin() {
    debugAssert(m_mode == TRACE);
    m_surfaceArray.clear();
    m_triArray.clear();
    m_mode = INSERT;
}


void World::insert(const shared_ptr<G3D::ArticulatedModel>& model, const G3D::CFrame& frame) {
    G3D::Array<shared_ptr<G3D::Surface> > posed;
    model->pose(posed, frame);
    for (int i = 0; i < posed.size(); ++i) {
        insert(posed[i]);
    }
}

void World::insert(const shared_ptr<G3D::Surface>& m) {
    debugAssert(m_mode == INSERT);
    m_surfaceArray.append(m);
}


void World::end() {
    G3D::Surface::getTris(m_surfaceArray, m_cpuVertexArray, m_triArray);
    for (int i = 0; i < m_triArray.size(); ++i) {
        m_triArray[i].material()->setStorage(G3D::MOVE_TO_CPU);
    }

    debugAssert(m_mode == INSERT);
    m_mode = TRACE;

    G3D::TriTree::Settings s;
    s.algorithm = G3D::TriTree::MEAN_EXTENT;
    G3D::Stopwatch timer;
    m_triTree.setContents(m_triArray, m_cpuVertexArray, s); 
    timer.after("TriTree creation");
    m_triArray.clear();
}


bool World::lineOfSight(const G3D::Vector3& v0, const G3D::Vector3& v1) const {
    debugAssert(m_mode == TRACE);
    
    G3D::Vector3 d = v1 - v0;
    float len = d.length();
    G3D::Ray ray = G3D::Ray::fromOriginAndDirection(v0, d / len);
    float distance = len;
    G3D::Tri::Intersector intersector;

    // For shadow rays, try to find intersections as quickly as possible, rather
    // than solving for the first intersection
    static const bool exitOnAnyHit = true, twoSidedTest = true;
    return ! m_triTree.intersectRay(ray, intersector, distance, exitOnAnyHit, twoSidedTest);

}

shared_ptr<G3D::Surfel> World::intersect(const G3D::Ray& ray, float& distance) const {
    debugAssert(m_mode == TRACE);

    return m_triTree.intersectRay(ray, distance);
}
