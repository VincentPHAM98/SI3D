//! \file tuto9.cpp utilisation d'un shader 'utilisateur' pour afficher un objet Mesh

#include "app.h"
#include "draw.h"
#include "mat.h"
#include "mesh.h"
#include "orbiter.h"
#include "program.h"
#include "uniforms.h"
#include "wavefront.h"

// Orbiter avec un mesh pour dessiner son frustum
class FrustumOrbiter : public Orbiter {
   public:
    Mesh m;
    FrustumOrbiter() : Orbiter() {
        m.create(GL_LINE_LOOP);
        projection(window_width(), window_height(), 45);
    }

    void traceFrustum() {
        m.clear();
        m.color(Green());
        Transform proj2World = view().inverse() * projection().inverse();
        Point a, b, c, d, A, B, C, D;
        a = proj2World(Point(-1, 1, -1));
        b = proj2World(Point(1, 1, -1));
        c = proj2World(Point(1, -1, -1));
        d = proj2World(Point(-1, -1, -1));
        A = proj2World(Point(-1, 1, 1));
        B = proj2World(Point(1, 1, 1));
        C = proj2World(Point(1, -1, 1));
        D = proj2World(Point(-1, -1, 1));
        // quadrilatère proche
        m.restart_strip();
        m.vertex(a);
        m.vertex(b);
        m.vertex(c);
        m.vertex(d);
        // quadrilatère éloigné
        m.restart_strip();
        m.vertex(A);
        m.vertex(B);
        m.vertex(C);
        m.vertex(D);
        // arêtes
        m.restart_strip();
        m.vertex(a);
        m.vertex(A);
        m.restart_strip();
        m.vertex(b);
        m.vertex(B);
        m.restart_strip();
        m.vertex(c);
        m.vertex(C);
        m.restart_strip();
        m.vertex(d);
        m.vertex(D);
    }

    //! observe le point center a une distance size.
    void lookat(const Point &center, const float size) {
        Orbiter::lookat(center, size);
        traceFrustum();
    }

    //! change le point de vue / la direction d'observation.
    void rotation(const float x, const float y) {
        Orbiter::rotation(x, y);
        traceFrustum();
    }
    //! deplace le centre / le point observe.
    void translation(const float x, const float y) {
        Orbiter::translation(x, y);
        traceFrustum();
    }
    //! rapproche / eloigne la camera du centre.
    void move(const float z) {
        Orbiter::move(z);
        traceFrustum();
    }

    void lookat(const Point &pmin, const Point &pmax) {
        lookat(center(pmin, pmax), distance(pmin, pmax));
    }
};

struct BBox {
    Mesh m;
    Point pmin, pmax;
    void init() {
        Point f11 = pmin;
        Point f12 = Point(pmax.x, pmin.y, pmin.z);
        Point f13 = Point(pmax.x, pmax.y, pmin.z);
        Point f14 = Point(pmin.x, pmax.y, pmin.z);

        Point f21 = Point(pmin.x, pmin.y, pmax.z);
        Point f22 = Point(pmax.x, pmin.y, pmax.z);
        Point f23 = pmax;
        Point f24 = Point(pmin.x, pmax.y, pmax.z);

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

    BBox() : pmin(), pmax() {}

    BBox(const Point &p) : pmin(p), pmax(p) {
        init();
    }
    BBox(const Point &_pmin, const Point &_pmax) : pmin(_pmin), pmax(_pmax) {
        init();
    }
    BBox(const BBox &box) : pmin(box.pmin), pmax(box.pmax) {
        *this = BBox(pmin, pmax);
    }
    BBox &operator=(const BBox &b) {
        m.release();
        m = b.m;
        pmin = b.pmin;
        pmax = b.pmax;
        return *this;
    }

    BBox &insert(const Point &p) {
        pmin = min(pmin, p);
        pmax = max(pmax, p);
        return *this;
    }
    BBox &insert(const BBox &box) {
        pmin = min(pmin, box.pmin);
        pmax = max(pmax, box.pmax);
        return *this;
    }

    float centroid(const int axis) const { return (pmin(axis) + pmax(axis)) / 2; }
};

class TP : public App {
   public:
    // constructeur : donner les dimensions de l'image, et eventuellement la version d'openGL.
    TP() : App(1024, 640) {}

    int init() {
        m_objet = read_mesh("data/robot.obj");

        Point pmin, pmax;
        m_objet.bounds(pmin, pmax);
        m_camera.lookat(pmin, pmax);
        m_frustum = FrustumOrbiter();
        m_frustum.lookat(pmin, pmax);
        m_camera.move(-100.);

        m_bbox = BBox(pmin, pmax);

        m_program = read_program("src/shader/frustum_culling.glsl");
        m_bbox_program = read_program("src/shader/bbox.glsl");
        program_print_errors(m_program);

        m_colors.resize(16);
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
        m_bbox.m.release();
        m_frustum.m.release();
        return 0;
    }

    int render() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int mx, my;
        unsigned int mb = SDL_GetRelativeMouseState(&mx, &my);
        if (mb & SDL_BUTTON(1))  // le bouton gauche est enfonce
            m_camera.rotation(mx, my);
        else if (mb & SDL_BUTTON(3))  // le bouton droit est enfonce
            m_camera.move(mx);
        else if (mb & SDL_BUTTON(2))  // le bouton du milieu est enfonce
            m_camera.translation((float)mx / (float)window_width(), (float)my / (float)window_height());

        // camera frustum deplacement
        if (key_state('i'))
            m_frustum.translation(0.f, 10.f / (float)window_width());
        if (key_state('k'))
            m_frustum.translation(0.f, -10.f / (float)window_width());
        if (key_state('j'))
            m_frustum.translation(10.f / (float)window_width(), 0.f);
        if (key_state('l'))
            m_frustum.translation(-10.f / (float)window_width(), 0.f);
        // Zoom
        if (key_state('u'))
            m_frustum.move(1.f);
        if (key_state('o'))
            m_frustum.move(-1.f);
        // Rotation
        if (key_state('h'))
            m_frustum.rotation(0.f, 1.f);
        if (key_state('n'))
            m_frustum.rotation(0.f, -1.f);

        if (key_state('p')) {  // changer pov
            clear_key_state('p');
            m_pov = !m_pov;
        }

        glUseProgram(m_program);

        Transform model = Identity();
        Transform view, projection;
        if (m_pov) {
            view = m_frustum.view();
            projection = m_frustum.projection();
        } else {
            view = m_camera.view();
            projection = m_camera.projection(window_width(), window_height(), 45);
        }

        Transform mv = view * model;
        Transform mvp = projection * mv;

        program_uniform(m_program, "mvMatrix", mv);
        program_uniform(m_program, "mvpMatrix", mvp);

        int location = glGetUniformLocation(m_program, "materials");
        glUniform4fv(location, m_colors.size(), &m_colors[0].r);

        m_objet.draw(m_program, /* use position */ true, /* use texcoord */ false, /* use normal */ true, /* use color */ false, /* use material index*/ true);

        // BBOX
        glUseProgram(m_bbox_program);
        program_uniform(m_bbox_program, "mvpMatrix", mvp);
        m_bbox.m.draw(m_bbox_program, true, false, false, true, false);
        m_frustum.m.draw(m_bbox_program, true, false, false, true, false);

        return 1;
    }

   protected:
    Transform m_model;
    Mesh m_objet;
    BBox m_bbox;
    int m_pov = 0;
    Orbiter m_camera;
    FrustumOrbiter m_frustum;
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
