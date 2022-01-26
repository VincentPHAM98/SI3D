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

struct Buffers {
    GLuint vao;
    GLuint buffer;

    int vertex_count;

    Buffers() : vao(0), buffer(0), vertex_count(0) {}

    void create(Mesh &mesh) {
        if (!mesh.vertex_buffer_size())
            return;

        auto groups = mesh.groups();
        std::vector<int> material_idx;
        for (int i = 0; i < int(groups.size()); i++) {
            for (int j = 0; j < groups[i].n; ++j) {
                material_idx.push_back(groups[i].index);
            }
        }

        size_t size = mesh.vertex_buffer_size() + mesh.texcoord_buffer_size() + mesh.normal_buffer_size() + material_idx.size() * sizeof(int);
        // cree et configure le vertex array object: conserve la description des attributs de sommets
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        // cree et initialise le buffer: conserve la positions des sommets
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_STATIC_DRAW);

        // attribut 0, position des sommets, declare dans le vertex shader : in vec3 position;
        size_t offset = 0;
        size = mesh.vertex_buffer_size();
        glBufferSubData(GL_ARRAY_BUFFER, offset, size, mesh.vertex_buffer());
        glVertexAttribPointer(0,
                              3, GL_FLOAT,  // size et type, position est un vec3 dans le vertex shader
                              GL_FALSE,     // pas de normalisation des valeurs
                              0,            // stride 0, les valeurs sont les unes a la suite des autres
                              0             // offset 0, les valeurs sont au debut du buffer
        );
        glEnableVertexAttribArray(0);

        // attribut 1 textures
        offset += size;
        size = mesh.texcoord_buffer_size();
        glBufferSubData(GL_ARRAY_BUFFER, offset, size, mesh.texcoord_buffer());
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (const GLvoid *)offset);
        glEnableVertexAttribArray(1);

        // attribut 2 normal
        offset += size;
        size = mesh.normal_buffer_size();
        glBufferSubData(GL_ARRAY_BUFFER, offset, size, mesh.normal_buffer());
        glVertexAttribPointer(2,
                              3, GL_FLOAT,  // size et type, normal est un vec3 dans le vertex shader
                              GL_FALSE,     // pas de normalisation des valeurs
                              0,            // stride 0, les valeurs sont les unes a la suite des autres
                              (const GLvoid *)offset);
        glEnableVertexAttribArray(2);

        // attribut 4 material
        offset += size;
        size = material_idx.size() * sizeof(unsigned int);
        glBufferSubData(GL_ARRAY_BUFFER, offset, size, &material_idx.front());
        glVertexAttribIPointer(4,
                               1, GL_UNSIGNED_INT,  // size et type, normal est un vec3 dans le vertex shader
                               0,                   // stride 0, les valeurs sont les unes a la suite des autres
                               (const GLvoid *)offset);
        glEnableVertexAttribArray(4);

        // conserve le nombre de sommets
        vertex_count = mesh.vertex_count();
    }

    void release() {
        glDeleteBuffers(1, &buffer);
        glDeleteVertexArrays(1, &vao);
    }
};

struct BBox {
    Mesh m;
    Point pmin, pmax;
    std::array<Point, 8> cornerPoints;
    uint idxStart = 0u;
    uint triangleCount = 0u;
    std::vector<TriangleData> trianglesInside;
    // mis a jour a chaque frame. Dessine le mesh si les triangles sont ds le frustum
    bool drawNextTime = true;

    BBox() : m(GL_LINES), pmin(), pmax() {
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
    BBox(const BBox &box) : m(box.m), pmin(box.pmin), pmax(box.pmax), cornerPoints(box.cornerPoints), idxStart(box.idxStart), triangleCount(box.triangleCount), trianglesInside(box.trianglesInside) {
    }

    BBox &operator=(const BBox &b) {
        m = b.m;
        pmin = b.pmin;
        pmax = b.pmax;
        cornerPoints = b.cornerPoints;
        idxStart = b.idxStart;
        triangleCount = b.triangleCount;
        trianglesInside = b.trianglesInside;

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

        // verifier si frustum dans box
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

struct alignas(16) Object {
    Point pmin;
    unsigned int vertex_count;
    Point pmax;
    unsigned int vertex_base;
};

// representation des parametres
struct alignas(4) IndirectParam {
    unsigned int vertex_count;
    unsigned int instance_count;
    unsigned int first_vertex;
    unsigned int first_instance;
};
// alignement GLSL correct...

class TP : public AppTime {
   public:
    // constructeur : donner les dimensions de l'image, et eventuellement la version d'openGL.
    TP() : AppTime(1024, 640) {}

    int init() {
        // verifier la presence de l'extension
        if (GLEW_ARB_shader_draw_parameters == 0) {
            std::cerr << "Erreur, extension non disponible\n";
            return -1;
        }

        // m_objet = read_mesh("data/robot.obj");
        m_objet = read_mesh("data/assets/bistro-small/exterior.obj");

        Point pmin, pmax;
        m_objet.bounds(pmin, pmax);
        m_camera.lookat(pmin, pmax);
        m_frustumCamera = FrustumOrbiter();
        m_frustumCamera.lookat(pmin, pmax);
        m_camera.move(-100.);

        m_boxes = BBox(pmin, pmax).subdivide((pmax.z - pmin.z) / 10.);

        // trouver a quelle box chaque triangle appartient
        std::vector<unsigned int> triangleBoxIdx(m_objet.triangle_count(), 0);
        for (auto i = 0; i < m_objet.triangle_count(); ++i) {
            const auto &triangle = m_objet.triangle(i);
            auto triangleCenter = centroid(triangle);
            for (size_t j = 0; j < m_boxes.size(); j++) {
                if (m_boxes[j].isInside(triangleCenter)) {
                    m_boxes[j].triangleCount++;
                    m_boxes[j].trianglesInside.push_back(triangle);
                    triangleBoxIdx[i] = j;
                }
            }
        }

        auto realBoxes = std::vector<BBox>();
        // pour chaque grosse box on crée un petite box qui épouse au mieux
        // la forme des triangles qui sont dedans.
        for (auto i = 0u; i < m_boxes.size(); ++i) {
            if (m_boxes[i].triangleCount) {
                auto box = BBox(m_boxes[i]);
                box.pmin = box.trianglesInside[0].a;
                box.pmax = box.trianglesInside[0].a;
                // iterer sur les triangles contenus dans la box pour s'adapter au mieux
                for (const auto &triangle : m_boxes[i].trianglesInside) {
                    box.insert(Point(triangle.a));
                    box.insert(Point(triangle.b));
                    box.insert(Point(triangle.c));
                }
                realBoxes.push_back(box);
            }
        }
        // m_boxes.clear();
        m_boxes = realBoxes;

        std::vector<BBox> temp;
        for (const auto &box : m_boxes) {
            if (box.triangleCount)
                temp.push_back(box);
        }
        // m_boxes = temp;

        // grouper les triangles par leur appartenance a une boite
        m_groups = m_objet.groups(triangleBoxIdx);

        // mettre en place les parametres par region pour le mdi
        for (auto i = 0u; i < m_groups.size(); i++) {
            auto &group = m_groups[i];
            auto &box = temp.at(i);
            Object b;
            b.pmin = box.pmin;
            b.vertex_count = box.triangleCount * 3;
            b.pmax = box.pmax;
            b.vertex_base = group.first;
            m_box.push_back(b);

            m_multi_indirect.push_back({
                static_cast<unsigned>(group.n),      // vertex_count
                1u,                                  // instance_count
                static_cast<unsigned>(group.first),  // first_vertex
                0u                                   // first_instance
            });
        }

        // regions a tester
        glGenBuffers(1, &m_box_buffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_box_buffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Object) * m_box.size(), m_box.data(), GL_DYNAMIC_DRAW);

        // re-indexation des regions visibles
        glGenBuffers(1, &m_remap_buffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_remap_buffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * m_box.size(), nullptr, GL_DYNAMIC_DRAW);

        // parametres du multi draw indirect
        glGenBuffers(1, &m_indirect_buffer);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirect_buffer);
        glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(IndirectParam) * m_box.size(), nullptr, GL_DYNAMIC_DRAW);
        // glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(IndirectParam) * m_multi_indirect.size(), m_multi_indirect.data(), GL_DYNAMIC_DRAW);

        // nombre de draws de multi draw indirect count
        glGenBuffers(1, &m_parameter_buffer);
        glBindBuffer(GL_PARAMETER_BUFFER_ARB, m_parameter_buffer);
        glBufferData(GL_PARAMETER_BUFFER_ARB, sizeof(int), nullptr, GL_DYNAMIC_DRAW);

        m_vao = m_objet.create_buffers(/* texcoord */ false, /* normal */ true, /* color */ true, /* material */ true);

        m_program = read_program("src/shader/gpu_culling.glsl");
        m_program_compute = read_program("src/shader/gpu_culling_compute.glsl");
        m_bbox_program = read_program("src/shader/bbox.glsl");
        program_print_errors(m_program_compute);
        program_print_errors(m_program);
        program_print_errors(m_bbox_program);

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
        release_program(m_program_compute);
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
            reload_program(m_program_compute, "src/shader/gpu_culling_compute.glsl");
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
        Transform frustumMVP = frustumProjection * frustumView;

        // etape 1: compute shader, tester l'inclusion des objets dans le frustum
        glUseProgram(m_program_compute);

        // uniforms
        program_uniform(m_program_compute, "world2projection", frustumMVP);
        program_uniform(m_program_compute, "projection2world", frustumMVP.inverse());

        // glBindBufferBase
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_box_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_remap_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_indirect_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_parameter_buffer);

        unsigned int zero = 0;
        glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);

        int n = m_groups.size() / 256;
        if (m_groups.size() % 256)
            n += 1;
        glDispatchCompute(n, 1, 1);
        // etape 2 : attendre le resultat
        glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

        // etape 3 : afficher les objets visibles (resultat de l'etape 1) avec 1 seul appel a glMultiDrawArraysIndirectCount
        glBindVertexArray(m_vao);
        glUseProgram(m_program);

        // uniforms
        program_uniform(m_program, "mvMatrix", mv);
        program_uniform(m_program, "mvpMatrix", mvp);
        int location = glGetUniformLocation(m_program, "materials");
        glUniform4fv(location, m_colors.size(), &m_colors[0].r);

        // SSBOs
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_remap_buffer);

        // parametres du multi draw...
        glBindBuffer(GL_PARAMETER_BUFFER_ARB, m_parameter_buffer);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirect_buffer);

        glMultiDrawArraysIndirectCountARB(m_objet.primitives(), 0, 0, m_box.size(), 0);

        // BBOX
        glUseProgram(m_bbox_program);
        program_uniform(m_bbox_program, "mvpMatrix", mvp);
        for (auto &&box : m_boxes) {
            if (box.drawNextTime) {
                box.m.draw(m_bbox_program, true, false, false, true, false);
            }
        }
        m_frustumCamera.getMesh().draw(m_bbox_program, true, false, false, true, false);

        return 1;
    }

   protected:
    Mesh m_objet;
    Buffers m_objet_buf;
    std::vector<TriangleGroup> m_groups;
    std::vector<BBox> m_boxes;
    Orbiter m_camera;
    FrustumOrbiter m_frustumCamera;
    // Quelle POV camera
    int m_pov = 0;
    GLuint m_texture;
    GLuint m_vao;
    GLuint m_program;
    GLuint m_program_compute;
    GLuint m_bbox_program;
    std::vector<Color> m_colors;
    GLuint m_box_buffer;
    std::vector<Object> m_box;
    GLuint m_remap_buffer;
    GLuint m_indirect_buffer;
    std::vector<IndirectParam> m_multi_indirect;
    GLuint m_parameter_buffer;
};

int main(int argc, char **argv) {
    TP tp;
    tp.run();

    return 0;
}
