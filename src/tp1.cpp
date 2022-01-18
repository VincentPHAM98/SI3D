//! \file tuto9.cpp utilisation d'un shader 'utilisateur' pour afficher un objet Mesh

#include "app_camera.h"
#include "app_time.h"  // classe Application a deriver
#include "draw.h"
#include "mat.h"
#include "mesh.h"
#include "orbiter.h"
#include "program.h"
#include "texture.h"
#include "uniforms.h"
#include "wavefront.h"

struct Buffers {
    GLuint vao;
    GLuint buffer;

    // GLuint normal_buffer;
    // GLuint material_buffer;
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

GLuint make_texture_array(const int unit, const std::vector<ImageData> &images, const GLenum texel_format = GL_RGBA) {
    assert(images.size());

    // verifie que toutes les images sont au meme format
    int w = images[0].width;
    int h = images[0].height;
    int c = images[0].channels;
    int s = images[0].size;
    int d = int(images.size());

    for (unsigned i = 1; i < images.size(); i++) {
        if (images[i].pixels.size() == 0)
            continue;  // pas de pixels, image pas chargee ?

        if (images[i].width != w)
            return 0;  //  pas la meme largeur
        if (images[i].height != h)
            return 0;  // pas la meme hauteur
    }

    // alloue le tableau de textures
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture);

    glTexImage3D(GL_TEXTURE_2D_ARRAY, /* mipmap */ 0,
                 texel_format, w, h, d, /* border */ 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    // transfere les textures
    for (unsigned i = 0; i < images.size(); i++) {
        if (images[i].pixels.size() == 0)
            continue;

        // recupere les parametres de conversion...
        GLenum format;
        switch (images[i].channels) {
            case 1:
                format = GL_RED;
                break;
            case 2:
                format = GL_RG;
                break;
            case 3:
                format = GL_RGB;
                break;
            case 4:
                format = GL_RGBA;
                break;
            default:
                format = GL_RGBA;
        }

        GLenum type;
        switch (images[i].size) {
            case 1:
                type = GL_UNSIGNED_BYTE;
                break;
            case 4:
                type = GL_FLOAT;
                break;
            default:
                type = GL_UNSIGNED_BYTE;
        }

        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, /* mipmap */ 0,
                        /* x offset */ 0, /* y offset */ 0, /* z offset == index */ i,
                        w, h, 1,
                        format, type, images[i].pixels.data());
    }

    // dimension max des textures 2d
    int max_2d = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_2d);
    int max_2d_array = 0;
    // dimension max d'un tableau de textures 2d
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &max_2d_array);
    printf("texture 2d maxsize %dx%d\n", max_2d, max_2d);
    printf("texture 2d array maxsize %dx%dx%d\n", max_2d, max_2d, max_2d_array);

    printf("texture array: %dx%dx%d %dMo\n", w, h, d, 4 * w * h * d / 1024 / 1024);

    //mipmaps
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

    return texture;
}

class TP : public AppTime {
   public:
    // constructeur : donner les dimensions de l'image, et eventuellement la version d'openGL.
    TP() : AppTime(1024, 640) {}

    int init() {
        // Mesh mesh= read_mesh("data/cube.obj");
        // Mesh mesh= read_mesh("data/robot.obj");
        // Mesh mesh= read_mesh("data/assets/roads_quaternion/OBJ/Street_3Way_2.obj");
        // Mesh mesh= read_mesh("data/assets/roads_quaternion/OBJ/Street_Bridgeroads_quaternion/OBJ/Street_Bridge_Ramp.obj");
        m_test_mesh = read_mesh("data/robot.obj");
        Mesh mesh = read_mesh("data/assets/bistro-small/exterior.obj");
        if (mesh.materials().count() == 0)
            return -1;  // pas de matieres, pas d'affichage

        m_objet.create(mesh);

        Point pmin, pmax;
        mesh.bounds(pmin, pmax);
        m_camera.lookat(pmin, pmax);

        m_program = read_program("src/shader/tp1_color.glsl");
        program_print_errors(m_program);

        // m_colors.resize(256);
        m_textures_diffuse.resize(256);
        m_textures_specular.resize(256);
        m_textures_emissive.resize(256);

        // copier les matieres utilisees
        const Materials &materials = mesh.materials();
        assert(materials.count() <= static_cast<int>(m_textures_diffuse.size()));

        for (int i = 0; i < materials.count(); i++) {
            // m_colors[i] = materials.material(i).diffuse;
            m_textures_diffuse[i] = materials.material(i).diffuse_texture;
            m_textures_specular[i] = materials.material(i).specular_texture;
            m_textures_emissive[i] = materials.material(i).emission_texture;
        }

        std::vector<ImageData> images;
        for (int i = 0; i < materials.filename_count(); i++) {
            ImageData image = read_image_data(materials.filename(i));
            images.emplace_back(image);
        }

        // cree le texture 2d array
        m_texture_array = make_texture_array(0, images);

        // m_textures.resize(materials.filename_count(), -1);
        // for (unsigned i = 0; i < m_textures.size(); i++)
        // {
        //     m_textures[i] = read_texture(0, materials.filename(i));
        //     // repetition des textures si les texccord sont > 1, au lieu de clamp...
        //     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        //     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // }

        // cree le tableaux des positions des lumières
        m_light_positions.resize(256);
        for (auto &pos : m_light_positions) {
            pos = Point(rand() % 10, 20, rand() % 10);
        }

        // etat openGL par defaut
        glClearColor(0.2f, 0.2f, 0.2f, 1.f);  // couleur par defaut de la fenetre

        glClearDepth(1.f);        // profondeur par defaut
        glDepthFunc(GL_LESS);     // ztest, conserver l'intersection la plus proche de la camera
        glEnable(GL_DEPTH_TEST);  // activer le ztest

        return 0;  // ras, pas d'erreur
    }

    // destruction des objets de l'application
    int quit() {
        // etape 3 : detruire le shader program
        release_program(m_program);
        m_objet.release();
        return 0;
    }

    // dessiner une nouvelle image
    int render() {
        if (key_state('r')) {
            clear_key_state('r');
            reload_program(m_program, "src/shader/tp1_color.glsl");
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // // deplace la camera
        // int mx, my;
        // unsigned int mb = SDL_GetRelativeMouseState(&mx, &my);
        // if (mb & SDL_BUTTON(1))  // le bouton gauche est enfonce
        //     m_camera.rotation(mx, my);
        // else if (mb & SDL_BUTTON(3))  // le bouton droit est enfonce
        //     m_camera.move(mx);
        // else if (mb & SDL_BUTTON(2))  // le bouton du milieu est enfonce
        //     m_camera.translation((float)mx / (float)window_width(), (float)my / (float)window_height());

        // recupere les mouvements de la souris
        int mx, my;
        unsigned int mb = SDL_GetRelativeMouseState(&mx, &my);
        int mousex, mousey;
        SDL_GetMouseState(&mousex, &mousey);

        // deplace la camera
        if (mb & SDL_BUTTON(1))
            m_camera.rotation(mx, my);  // tourne autour de l'objet
        else if (mb & SDL_BUTTON(3))
            m_camera.translation((float)mx / (float)window_width(), (float)my / (float)window_height());  // deplace le point de rotation
        else if (mb & SDL_BUTTON(2))
            m_camera.move(mx);  // approche / eloigne l'objet

        SDL_MouseWheelEvent wheel = wheel_event();
        if (wheel.y != 0) {
            clear_wheel_event();
            m_camera.move(8.f * wheel.y);  // approche / eloigne l'objet
        }

        const char *orbiter_filename = "app_orbiter.txt";
        // copy / export / write orbiter
        if (key_state('c')) {
            clear_key_state('c');
            m_camera.write_orbiter(orbiter_filename);
        }
        // paste / read orbiter
        if (key_state('v')) {
            clear_key_state('v');

            Orbiter tmp;
            if (tmp.read_orbiter(orbiter_filename) < 0)
                // ne pas modifer la camera en cas d'erreur de lecture...
                tmp = m_camera;

            m_camera = tmp;
        }

        glUseProgram(m_program);

        Transform model = Identity();
        Transform view = m_camera.view();
        Transform projection = m_camera.projection(window_width(), window_height(), 45);

        Transform mv = view * model;
        Transform mvp = projection * mv;

        program_uniform(m_program, "mvMatrix", mv);
        program_uniform(m_program, "mvpMatrix", mvp);
        //   int location= glGetUniformLocation(program, "mvpMatrix");
        //   glUniformMatrix4fv(location, 1, GL_TRUE, mvp.buffer());

        // selectionne le texture 2d array sur une unite de texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_texture_array);

        // parametrer le shader pour dessiner avec la couleur
        glUseProgram(m_program);

        // indice de l'unite de texture associee au texture 2d array, cf le ActiveTexture() au dessus...
        program_uniform(m_program, "texture_array", 0);

        // couleur diffuse des matieres, cf la declaration 'uniform vec4 materials[];' dans le fragment shader
        int location = glGetUniformLocation(m_program, "materials");
        glUniform4fv(location, m_colors.size(), &m_colors[0].r);

        location = glGetUniformLocation(m_program, "textures_diffuse");
        glUniform1iv(location, m_textures_diffuse.size(), &m_textures_diffuse[0]);

        location = glGetUniformLocation(m_program, "textures_specular");
        glUniform1iv(location, m_textures_specular.size(), &m_textures_specular[0]);

        location = glGetUniformLocation(m_program, "textures_emissive");
        glUniform1iv(location, m_textures_emissive.size(), &m_textures_emissive[0]);

        for (auto &pos : m_light_positions) {
            pos = view(pos);
        }
        location = glGetUniformLocation(m_program, "lights_position");
        glUniform3fv(location, m_light_positions.size(), &m_light_positions[0].x);

        glBindVertexArray(m_objet.vao);
        // dessiner les triangles du groupe
        glDrawArrays(GL_TRIANGLES, 0, m_objet.vertex_count);

        // draw(m_test_mesh, m_camera);

        return 1;
    }

   protected:
    Mesh m_test_mesh;
    Transform m_model;
    Buffers m_objet;
    Orbiter m_camera;
    GLuint m_texture;
    GLuint m_texture_array;
    GLuint m_program;
    std::vector<Color> m_colors;
    std::vector<int> m_textures_diffuse;
    std::vector<int> m_textures_specular;
    std::vector<int> m_textures_emissive;
    std::vector<Point> m_light_positions;
};

int main(int argc, char **argv) {
    TP tp;
    tp.run();

    return 0;
}
