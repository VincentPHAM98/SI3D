//! \file tuto9.cpp utilisation d'un shader 'utilisateur' pour afficher un objet Mesh

#include <algorithm>

#include "app.h"
#include "app_time.h"
#include "draw.h"
#include "mat.h"
#include "mesh.h"
#include "orbiter.h"
#include "program.h"
#include "uniforms.h"
#include "wavefront.h"

struct BBox {
    Mesh m;
    Point pmin, pmax;
    std::array<Point, 8> cornerPoints;
    uint idxStart = 0u;
    uint triangleCount = 0u;
    std::vector<TriangleData> trianglesInside;
    // mis a jour a chaque frame. Dessine le mesh si les triangles sont ds le frustum
    bool drawNextTime = false;

    BBox() : pmin(), pmax(), m(GL_LINES) {
        init();
    }

    BBox(const Point &p) : pmin(p), pmax(p) {
        init();
        trace();
    }
    BBox(const Point &_pmin, const Point &_pmax) : pmin(_pmin), pmax(_pmax) {
        init();
        trace();
    }
    BBox(const BBox &box) : pmin(box.pmin), pmax(box.pmax), cornerPoints(box.cornerPoints) {
        *this = BBox(pmin, pmax);
        trace();
    }
    BBox &operator=(const BBox &b) {
        pmin = b.pmin;
        pmax = b.pmax;
        cornerPoints = b.cornerPoints;
        trace();
        return *this;
    }

    void init() {
        cornerPoints.at(0) = pmin;
        cornerPoints.at(1) = Point(pmax.x, pmin.y, pmin.z);
        cornerPoints.at(2) = Point(pmax.x, pmax.y, pmin.z);
        cornerPoints.at(3) = Point(pmin.x, pmax.y, pmin.z);
        cornerPoints.at(4) = Point(pmin.x, pmin.y, pmax.z);
        cornerPoints.at(5) = Point(pmax.x, pmin.y, pmax.z);
        cornerPoints.at(6) = pmax;
        cornerPoints.at(7) = Point(pmin.x, pmax.y, pmax.z);
    }

    void trace() {
        Point &f11 = cornerPoints.at(0);
        Point &f12 = cornerPoints.at(1);
        Point &f13 = cornerPoints.at(2);
        Point &f14 = cornerPoints.at(3);

        Point &f21 = cornerPoints.at(4);
        Point &f22 = cornerPoints.at(5);
        Point &f23 = cornerPoints.at(6);
        Point &f24 = cornerPoints.at(7);

        m.create(GL_LINES);
        m.color(Red());
        m.vertex(f11);
        m.vertex(f12);
        m.vertex(f12);
        m.vertex(f13);
        m.vertex(f13);
        m.vertex(f14);
        m.vertex(f14);
        m.vertex(f11);
        m.vertex(f21);
        m.vertex(f22);
        m.vertex(f22);
        m.vertex(f23);
        m.vertex(f23);
        m.vertex(f24);
        m.vertex(f24);
        m.vertex(f21);

        m.vertex(f24);
        m.vertex(f14);
        m.vertex(f23);
        m.vertex(f13);
        m.vertex(f22);
        m.vertex(f12);
        m.vertex(f21);
        m.vertex(f11);
    }

    BBox &insert(const Point &p) {
        pmin = min(pmin, p);
        pmax = max(pmax, p);
        init();
        return *this;
    }
    BBox &insert(const BBox &box) {
        pmin = min(pmin, box.pmin);
        pmax = max(pmax, box.pmax);
        return *this;
    }

    bool isInside(const Point &p) const {
        return (
            (p.x >= pmin.x && p.x <= pmax.x) &&
            (p.y >= pmin.y && p.y <= pmax.y) &&
            (p.z >= pmin.z && p.z <= pmax.z));
    }

    Point centroid() {
        return Point((Vector(pmin) + Vector(pmax)) / 2.);
    }
    float centroid(const int axis) const {
        return (pmin(axis) + pmax(axis)) / 2;
    }

    std::vector<BBox> subdivision() {
        auto center = centroid();
        auto boxes = std::vector<BBox>(8);
        for (size_t i = 0; i < 8; i++) {
            boxes.at(i).pmin = min(center, cornerPoints.at(i));
            boxes.at(i).pmax = max(center, cornerPoints.at(i));
        }
        return boxes;
    }

    std::vector<BBox> subdivide(double boxSize) {
        auto boxes = std::vector<BBox>();
        for (double z = pmin.z; z <= pmax.z; z += boxSize) {
            for (double y = pmin.y; y <= pmax.y; y += boxSize) {
                for (double x = pmin.x; x <= pmax.x; x += boxSize) {
                    boxes.push_back(BBox(Point(x, y, z),
                                         Point(x + boxSize, y + boxSize, z + boxSize)));
                }
            }
        }
        return boxes;
    }
};

class Frustum {
    Transform m_view, m_projection, m_proj2World;
    std::vector<Point> m_worldPoints;

    // p point repère projectif
    bool isInside(const vec4 &p) const {
        for (uint i = 0; i < 3u; i++) {
            if (p(i) <= -p.w || p(i) >= p.w)
                return false;
        }
        return true;
    }

   public:
    Mesh m_mesh;

    Frustum() {}

    Frustum(const Transform &view, const Transform &projection) : m_view(view), m_projection(projection) {
        m_mesh.create(GL_LINE_LOOP);
        trace();
    }

    void trace() {
        m_mesh.clear();
        m_mesh.color(Green());
        m_proj2World = m_view.inverse() * m_projection.inverse();
        Point a, b, c, d, A, B, C, D;
        m_worldPoints = {
            a = m_proj2World(Point(-1, 1, -1)),
            b = m_proj2World(Point(1, 1, -1)),
            c = m_proj2World(Point(1, -1, -1)),
            d = m_proj2World(Point(-1, -1, -1)),
            A = m_proj2World(Point(-1, 1, 1)),
            B = m_proj2World(Point(1, 1, 1)),
            C = m_proj2World(Point(1, -1, 1)),
            D = m_proj2World(Point(-1, -1, 1))};
        // quadrilatère proche
        m_mesh.restart_strip();
        m_mesh.vertex(a);
        m_mesh.vertex(b);
        m_mesh.vertex(c);
        m_mesh.vertex(d);
        // quadrilatère éloigné
        m_mesh.restart_strip();
        m_mesh.vertex(A);
        m_mesh.vertex(B);
        m_mesh.vertex(C);
        m_mesh.vertex(D);
        // arêtes
        m_mesh.restart_strip();
        m_mesh.vertex(a);
        m_mesh.vertex(A);
        m_mesh.restart_strip();
        m_mesh.vertex(b);
        m_mesh.vertex(B);
        m_mesh.restart_strip();
        m_mesh.vertex(c);
        m_mesh.vertex(C);
        m_mesh.restart_strip();
        m_mesh.vertex(d);
        m_mesh.vertex(D);
    }

    void update(const Transform &view, const Transform &projection) {
        m_view = view;
        m_projection = projection;
        trace();
    }

    bool isInside(const BBox &bbox) const {
        auto world2Projection = m_projection * m_view;
        auto pmin = vec4(bbox.pmin);
        auto pmax = vec4(bbox.pmax);
        auto boxCornerPoints = std::vector<vec4>{
            pmin,
            vec4(pmax.x, pmin.y, pmin.z, 1.f),
            vec4(pmax.x, pmax.y, pmin.z, 1.f),
            vec4(pmin.x, pmax.y, pmin.z, 1.f),
            vec4(pmin.x, pmax.y, pmax.z, 1.f),
            vec4(pmin.x, pmin.y, pmax.z, 1.f),
            vec4(pmax.x, pmin.y, pmax.z, 1.f),
            pmax};

        // verifier si box dans frustum
        for (auto &&point : boxCornerPoints) {
            // on applique la transformation projective avant de tester
            if (isInside(world2Projection(point)))
                return true;
        }

        //verifier si frustum dans box
        for (auto &&point : m_worldPoints) {
            if (bbox.isInside(point))
                return true;
        }
        // si aucun corner du frustum ou de la boite est dans l'autre objet, faux
        return false;
    }
};

// Orbiter avec un mesh pour dessiner son frustum
class FrustumOrbiter : public Orbiter {
   public:
    Frustum m_frustum;

    FrustumOrbiter() : Orbiter() {
        m_frustum = Frustum(view(), projection());
        projection(window_width(), window_height(), 45);
    }

    //! observe le point center a une distance size.
    void lookat(const Point &center, const float size) {
        Orbiter::lookat(center, size);
        m_frustum.update(view(), projection());
    }
    //! change le point de vue / la direction d'observation.
    void rotation(const float x, const float y) {
        Orbiter::rotation(x, y);
        m_frustum.update(view(), projection());
    }
    //! deplace le centre / le point observe.
    void translation(const float x, const float y) {
        Orbiter::translation(x, y);
        m_frustum.update(view(), projection());
    }
    //! rapproche / eloigne la camera du centre.
    void move(const float z) {
        Orbiter::move(z);
        m_frustum.update(view(), projection());
    }
    void lookat(const Point &pmin, const Point &pmax) {
        lookat(center(pmin, pmax), distance(pmin, pmax));
    }

    Mesh &getMesh() { return m_frustum.m_mesh; }
};

class TP : public AppTime {
   public:
    // constructeur : donner les dimensions de l'image, et eventuellement la version d'openGL.
    TP() : AppTime(1024, 640) {}

    int init() {
        m_objet = read_mesh("data/robot.obj");
        // m_objet = read_mesh("data/assets/bistro-small/exterior.obj");
        m_groups = m_objet.groups();

        Point pmin, pmax;
        m_objet.bounds(pmin, pmax);
        m_camera.lookat(pmin, pmax);
        m_frustumCamera = FrustumOrbiter();
        m_frustumCamera.lookat(pmin, pmax);
        m_camera.move(-100.);

        m_boxes = BBox(pmin, pmax).subdivide((pmax.x - pmin.x) / 50);

        // trouver a quelle box chaque triangle appartient
        std::vector<unsigned int> triangleBoxIdx(m_objet.triangle_count(), 0);
        for (auto i = 0; i < m_objet.triangle_count(); ++i) {
            const auto &triangle = m_objet.triangle(i);
            auto triangleCenter = centroid(triangle);
            for (size_t j = 0; j < m_boxes.size(); j++) {
                if (m_boxes[j].isInside(triangleCenter)) {
                    m_boxes[j].triangleCount++;
                    m_boxes.at(j).trianglesInside.push_back(triangle);
                    triangleBoxIdx[i] = j;
                }
            }
        }

        auto realBoxes = std::vector<BBox>();
        // pour chaque grosse box on crée un petite box qui épouse au mieux
        // la forme des triangles qui sont dedans.
        for (auto i = 0u; i < m_boxes.size(); ++i) {
            if (m_boxes[i].triangleCount) {
                auto box = BBox();
                // iterer sur les triangles contenus dans la box pour s'adapter au mieux
                for (const auto &triangle : m_boxes[i].trianglesInside) {
                    box.insert(Point(triangle.a));
                    box.insert(Point(triangle.b));
                    box.insert(Point(triangle.c));
                    // box.insert(centroid(triangle));
                }
                box.trianglesInside = m_boxes[i].trianglesInside;
                box.trace();
                realBoxes.push_back(box);
            }
        }
        m_boxes.clear();
        m_boxes = realBoxes;

        // grouper les triangles par leur appartenance a une boite
        m_groups = m_objet.groups(triangleBoxIdx);

        m_program = read_program("src/shader/frustum_culling.glsl");
        m_bbox_program = read_program("src/shader/bbox.glsl");
        program_print_errors(m_program);

        m_colors.resize(256);
        const Materials &materials = m_objet.materials();
        assert(materials.count() <= int(m_colors.size()));
        for (int i = 0; i < materials.count(); i++)
            m_colors[i] = materials.material(i).diffuse;

        // etat openGL par defaut
        glClearColor(0.2f, 0.2f, 0.2f, 1.f);  // couleur par defaut de la fenetre

        glClearDepth(1.f);        // profondeur par defaut
        glDepthFunc(GL_LESS);     // ztest, conserver l'intersection la plus proche de la camera
        glEnable(GL_DEPTH_TEST);  // activer le ztest

        return 0;  // ras, pas d'erreur
    }

    int quit() {
        release_program(m_program);
        release_program(m_bbox_program);
        m_objet.release();
        // relase boxes
        std::for_each(m_boxes.begin(), m_boxes.end(), [](BBox &b) { b.m.release(); });
        m_frustumCamera.getMesh().release();
        return 0;
    }

    void handleKeyState() {
        if (key_state('r')) {  // recharger le shader
            clear_key_state('r');
            reload_program(m_program, "src/shader/frustum_culling.glsl");
            reload_program(m_bbox_program, "src/shader/bbox.glsl");
        }
        int mx, my;
        unsigned int mb = SDL_GetRelativeMouseState(&mx, &my);
        if (mb & SDL_BUTTON(1))  // le bouton gauche est enfonce
            m_camera.rotation(mx, my);
        else if (mb & SDL_BUTTON(3))  // le bouton droit est enfonce
            m_camera.move(mx);
        else if (mb & SDL_BUTTON(2))  // le bouton du milieu est enfonce
            m_camera.translation((float)mx / (float)window_width(), (float)my / (float)window_height());

        // recupere l'etat de la molette / touch
        SDL_MouseWheelEvent wheel = wheel_event();
        if (wheel.y != 0) {
            clear_wheel_event();
            m_camera.move(8.f * wheel.y);  // approche / eloigne l'objet
        }
        // camera frustum deplacement
        if (key_state('i'))
            m_frustumCamera.translation(0.f, 10.f / (float)window_width());
        if (key_state('k'))
            m_frustumCamera.translation(0.f, -10.f / (float)window_width());
        if (key_state('j'))
            m_frustumCamera.translation(10.f / (float)window_width(), 0.f);
        if (key_state('l'))
            m_frustumCamera.translation(-10.f / (float)window_width(), 0.f);
        // Zoom
        if (key_state('u'))
            m_frustumCamera.move(1.f);
        if (key_state('o'))
            m_frustumCamera.move(-1.f);
        // Rotation
        if (key_state('h'))
            m_frustumCamera.rotation(0.f, 1.f);
        if (key_state('n'))
            m_frustumCamera.rotation(0.f, -1.f);

        if (key_state('p')) {  // changer pov
            clear_key_state('p');
            m_pov = !m_pov;
        }
    }

    Point centroid(const TriangleData &t) {
        return Point((Vector(t.a) + Vector(t.b) + Vector(t.c)) / 3.f);
    }

    int render() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        handleKeyState();

        glUseProgram(m_program);

        Transform model = Identity();
        Transform frustumView, frustumProjection, freeView, freeProjection, view, projection;
        frustumView = m_frustumCamera.view();
        frustumProjection = m_frustumCamera.projection();
        freeView = m_camera.view();
        freeProjection = m_camera.projection(window_width(), window_height(), 45);
        if (m_pov) {
            view = frustumView;
            projection = frustumProjection;
        } else {
            view = freeView;
            projection = freeProjection;
        }

        Transform mv = view * model;
        Transform mvp = projection * mv;
        Transform frustumMVP = frustumProjection * frustumView * model;

        program_uniform(m_program, "mvMatrix", mv);
        program_uniform(m_program, "mvpMatrix", mvp);

        int location = glGetUniformLocation(m_program, "materials");
        glUniform4fv(location, m_colors.size(), &m_colors[0].r);

        // for (auto &group : m_groups) {
        for (auto i = 0u; i < m_groups.size(); i++) {
            auto &group = m_groups[i];
            // test si la bbox du groupe de triangle est dans le frustum
            if (m_frustumCamera.m_frustum.isInside(m_boxes.at(i))) {
                m_boxes[group.index].drawNextTime = true;
                m_objet.draw(group.first, group.n, m_program, /* use position */ true, /* use texcoord */ false, /* use normal */ true, /* use color */ false, /* use material index*/ true);
            }
        }

        // BBOX
        glUseProgram(m_bbox_program);
        program_uniform(m_bbox_program, "mvpMatrix", mvp);
        for (auto &&box : m_boxes) {
            if (box.drawNextTime) {
                box.drawNextTime = false;
                box.m.draw(m_bbox_program, true, false, false, true, false);
            }
        }
        m_frustumCamera.getMesh().draw(m_bbox_program, true, false, false, true, false);

        return 1;
    }

   protected:
    Mesh m_objet;
    std::vector<TriangleGroup> m_groups;
    // BBox m_bbox;
    std::vector<BBox> m_boxes;
    Orbiter m_camera;
    FrustumOrbiter m_frustumCamera;
    // Quelle POV camera
    int m_pov = 0;
    GLuint m_texture;
    GLuint m_program;
    GLuint m_bbox_program;
    std::vector<Color> m_colors;
};

int main(int argc, char **argv) {
    TP tp;
    tp.run();

    return 0;
}
