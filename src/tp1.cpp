//! \file tuto9.cpp utilisation d'un shader 'utilisateur' pour afficher un objet Mesh

#include "mat.h"
#include "mesh.h"
#include "wavefront.h"

#include "orbiter.h"
#include "program.h"
#include "uniforms.h"
#include "draw.h"

#include "app.h" // classe Application a deriver

struct Buffers
{
    GLuint vao;
    GLuint buffer;
    // GLuint normal_buffer;
    // GLuint material_buffer;
    int vertex_count;

    Buffers( ) : vao(0), buffer(0), vertex_count(0) {}
    
    void create( Mesh& mesh )
    {
        if(!mesh.vertex_buffer_size()) return;
        
        auto groups= mesh.groups();
        std::vector<int> material_idx;
        for(int i= 0; i < int(groups.size()); i++) {
            std::cout << "group n: " << groups[i].n << std::endl;
            for (int j = 0 ; j < groups[i].n; ++j)
                material_idx.push_back(groups[i].material_index);
        }


        size_t size = mesh.vertex_buffer_size() + mesh.normal_buffer_size() + material_idx.size();
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
            3, GL_FLOAT,    // size et type, position est un vec3 dans le vertex shader
            GL_FALSE,       // pas de normalisation des valeurs
            0,              // stride 0, les valeurs sont les unes a la suite des autres
            0               // offset 0, les valeurs sont au debut du buffer
        );
        glEnableVertexAttribArray(0);

        // attribut 2 normal
        offset += size;
        size = mesh.normal_buffer_size();
        glBufferSubData(GL_ARRAY_BUFFER, offset, size, mesh.normal_buffer());
        glVertexAttribPointer(2, 
            3, GL_FLOAT,    // size et type, normal est un vec3 dans le vertex shader
            GL_FALSE,       // pas de normalisation des valeurs
            0,              // stride 0, les valeurs sont les unes a la suite des autres
            (const GLvoid *)offset
        );
        glEnableVertexAttribArray(2);

        
        // attribut 4 material
        offset += size;
        size = material_idx.size();
        glBufferSubData(GL_ARRAY_BUFFER, offset, size, &material_idx[0]);
        glVertexAttribIPointer(4, 
            1, GL_UNSIGNED_INT,    // size et type, normal est un vec3 dans le vertex shader
            0,              // stride 0, les valeurs sont les unes a la suite des autres
            (const GLvoid *)offset
        );
        glEnableVertexAttribArray(4);

        // conserve le nombre de sommets
        vertex_count= mesh.vertex_count();
    }
    
    void release( )
    {
        glDeleteBuffers(1, &buffer);
        glDeleteVertexArrays(1, &vao);
    }
};

class TP : public App
{
public:
    // constructeur : donner les dimensions de l'image, et eventuellement la version d'openGL.
    TP() : App(1024, 640) {}

    int init()
    {
        // Mesh mesh= read_mesh("data/cube.obj");
        Mesh mesh= read_mesh("data/robot.obj");
        // Mesh mesh= read_mesh("data/assets/roads_quaternion/OBJ/Street_3Way_2.obj");
        // Mesh mesh= read_mesh("data/assets/roads_quaternion/OBJ/Street_Bridge_Ramp.obj");
        if (mesh.materials().count() == 0)
            return -1; // pas de matieres, pas d'affichage

        m_objet.create(mesh);

        Point pmin, pmax;
        mesh.bounds(pmin, pmax);
        m_camera.lookat(pmin, pmax);

        m_program = read_program("src/shader/tp1_color.glsl");
        program_print_errors(m_program);
        
        m_colors.resize(16);

        // copier les matieres utilisees
        const Materials& materials= mesh.materials();
        assert(materials.count() <= int(m_colors.size()));
        for(int i= 0; i < materials.count(); i++)
            m_colors[i]= materials.material(i).diffuse;


        // etat openGL par defaut
        glClearColor(0.2f, 0.2f, 0.2f, 1.f); // couleur par defaut de la fenetre

        glClearDepth(1.f);       // profondeur par defaut
        glDepthFunc(GL_LESS);    // ztest, conserver l'intersection la plus proche de la camera
        glEnable(GL_DEPTH_TEST); // activer le ztest

        return 0; // ras, pas d'erreur
    }

    // destruction des objets de l'application
    int quit()
    {
        // etape 3 : detruire le shader program
        release_program(m_program);
        m_objet.release();
        return 0;
    }

    // dessiner une nouvelle image
    int render()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // deplace la camera
        int mx, my;
        unsigned int mb = SDL_GetRelativeMouseState(&mx, &my);
        if (mb & SDL_BUTTON(1)) // le bouton gauche est enfonce
            m_camera.rotation(mx, my);
        else if (mb & SDL_BUTTON(3)) // le bouton droit est enfonce
            m_camera.move(mx);
        else if (mb & SDL_BUTTON(2)) // le bouton du milieu est enfonce
            m_camera.translation((float)mx / (float)window_width(), (float)my / (float)window_height());

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

        // parametrer le shader pour dessiner avec la couleur
        glUseProgram(m_program);

        //   . couleur diffuse des matieres, cf la declaration 'uniform vec4 materials[];' dans le fragment shader
        int location= glGetUniformLocation(m_program, "materials");
        glUniform4fv(location, m_colors.size(), &m_colors[0].r);

        glBindVertexArray(m_objet.vao);
        // dessiner les triangles du groupe
        glDrawArrays(GL_TRIANGLES, 0, m_objet.vertex_count);

        return 1;
    }

protected:
    Transform m_model;
    Buffers m_objet;
    Orbiter m_camera;
    GLuint m_texture;
    GLuint m_program;
    std::vector<Color> m_colors;
};

int main(int argc, char **argv)
{
    TP tp;
    tp.run();

    return 0;
}
