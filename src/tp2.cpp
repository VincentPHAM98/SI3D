
//! \file tuto_rayons.cpp

#include <vector>
#include <cfloat>
#include <chrono>
#include <random>

#include "vec.h"
#include "mat.h"
#include "color.h"
#include "image.h"
#include "image_io.h"
#include "image_hdr.h"
#include "orbiter.h"
#include "mesh.h"
#include "wavefront_fast.h"
#include "sampler.h"

struct World
{
    World( const Vector& _n ) : n(_n) 
    {
        float sign= std::copysign(1.0f, n.z);
        float a= -1.0f / (sign + n.z);
        float d= n.x * n.y * a;
        t= Vector(1.0f + sign * n.x * n.x * a, sign * d, -sign * n.x);
        b= Vector(d, sign + n.y * n.y * a, -n.y);        
    }
    
    // transforme le vecteur du repere local vers le repere du monde
    Vector operator( ) ( const Vector& local )  const { return local.x * t + local.y * b + local.z * n; }
    
    // transforme le vecteur du repere du monde vers le repere local
    Vector inverse( const Vector& global ) const { return Vector(dot(global, t), dot(global, b), dot(global, n)); }
    
    Vector t;
    Vector b;
    Vector n;
};

struct Ray
{
    Point o;            // origine
    Vector d;           // direction
    float tmax;             // intervalle [0 tmax]
    
    Ray( const Point& _o, const Point& _e ) :  o(_o), d(Vector(_o, _e)) {}
    
    Ray( const Point& origine, const Vector& direction ) : o(origine), d(direction), tmax(FLT_MAX) {}
};

struct Hit
{
    float t;            // p(t)= o + td, position du point d'intersection sur le rayon
    float u, v;         // p(u, v), position du point d'intersection sur le triangle
    int triangle_id;    // indice du triangle dans le mesh
    
    Hit( ) : t(FLT_MAX), u(), v(), triangle_id(-1) {}
    Hit( const float _t, const float _u, const float _v, const int _id ) : t(_t), u(_u), v(_v), triangle_id(_id) {}
    operator bool ( ) { return (triangle_id != -1); }
};

struct RayHit
{
    Point o;            // origine
    float t;            // p(t)= o + td, position du point d'intersection sur le rayon
    Vector d;           // direction
    int triangle_id;    // indice du triangle dans le mesh
    float u, v;
    int x, y;
    
    RayHit( const Point& _o, const Point& _e ) :  o(_o), t(1), d(Vector(_o, _e)), triangle_id(-1), u(), v(), x(), y() {}
    RayHit( const Point& _o, const Point& _e, const int _x, const int _y ) :  o(_o), t(1), d(Vector(_o, _e)), triangle_id(-1), u(), v(), x(_x), y(_y) {}
    operator bool ( ) { return (triangle_id != -1); }
};

struct BBoxHit
{
    float tmin, tmax;
    
    BBoxHit() : tmin(FLT_MAX), tmax(-FLT_MAX) {}
    BBoxHit( const float _tmin, const float _tmax ) : tmin(_tmin), tmax(_tmax) {}
    float centroid( ) const { return (tmin + tmax) / 2; }
    operator bool( ) const { return tmin <= tmax; }
};


struct BBox
{
    Point pmin, pmax;
    
    BBox( ) : pmin(), pmax() {}
    
    BBox( const Point& p ) : pmin(p), pmax(p) {}
    BBox( const BBox& box ) : pmin(box.pmin), pmax(box.pmax) {}
    
    BBox& insert( const Point& p ) { pmin= min(pmin, p); pmax= max(pmax, p); return *this; }
    BBox& insert( const BBox& box ) { pmin= min(pmin, box.pmin); pmax= max(pmax, box.pmax); return *this; }
    
    float centroid( const int axis ) const { return (pmin(axis) + pmax(axis)) / 2; }
    
    BBoxHit intersect( const RayHit& ray ) const
    {
        Vector invd= Vector(1 / ray.d.x, 1 / ray.d.y, 1 / ray.d.z);
        return intersect(ray, invd);
    }
    
    BBoxHit intersect( const RayHit& ray, const Vector& invd ) const
    {
        Point rmin= pmin;
        Point rmax= pmax;
        if(ray.d.x < 0) std::swap(rmin.x, rmax.x);
        if(ray.d.y < 0) std::swap(rmin.y, rmax.y);
        if(ray.d.z < 0) std::swap(rmin.z, rmax.z);
        Vector dmin= (rmin - ray.o) * invd;
        Vector dmax= (rmax - ray.o) * invd;
        
        float tmin= std::max(dmin.z, std::max(dmin.y, std::max(dmin.x, 0.f)));
        float tmax= std::min(dmax.z, std::min(dmax.y, std::min(dmax.x, ray.t)));
        return BBoxHit(tmin, tmax);
    }
};

struct Triangle
{
    Point p;            // sommet a du triangle
    Vector e1, e2;      // aretes ab, ac du triangle
    int id;
    
    float aire;

    Triangle( const TriangleData& data, const int _id ) : p(data.a), e1(Vector(data.a, data.b)), e2(Vector(data.a, data.c)), id(_id) {
        float ab = length(e1);
        float ac = length(e2);
        float bc = length((p+e2) - (p+e1));
        float p = (ab + ac + bc)/ 2.f;
        aire = sqrt(p * (p - ab) * (p - bc) * (p - ac));
    }
    
    /* calcule l'intersection ray/triangle
        cf "fast, minimum storage ray-triangle intersection" 
        
        renvoie faux s'il n'y a pas d'intersection valide (une intersection peut exister mais peut ne pas se trouver dans l'intervalle [0 tmax] du rayon.)
        renvoie vrai + les coordonnees barycentriques (u, v) du point d'intersection + sa position le long du rayon (t).
        convention barycentrique : p(u, v)= (1 - u - v) * a + u * b + v * c
    */
    Hit intersect( const Ray &ray, const float tmax ) const
    {
        Vector pvec= cross(ray.d, e2);
        float det= dot(e1, pvec);
        
        float inv_det= 1 / det;
        Vector tvec(p, ray.o);
        
        float u= dot(tvec, pvec) * inv_det;
        if(u < 0 || u > 1) return Hit();
        
        Vector qvec= cross(tvec, e1);
        float v= dot(ray.d, qvec) * inv_det;
        if(v < 0 || u + v > 1) return Hit();
        
        float t= dot(e2, qvec) * inv_det;
        if(t > tmax || t < 0) return Hit();
        
        return Hit(t, u, v, id);           // p(u, v)= (1 - u - v) * a + u * b + v * c
    }

    //GI compemdium eq 18
    Point sample18(const float u1, const float u2) const
    {
        float r1 = std::sqrt(u1);
        float a = 1 - r1;
        float b = (1 - u2) * r1;
        float g = u2 * r1;
        return a * p + b * (p + e1) + g * (p + e2);
    }

    float pdf18(const Point& p) const
    {
        return 1.0 / aire;
    }

    void intersect( RayHit &ray ) const
    {
        Vector pvec= cross(ray.d, e2);
        float det= dot(e1, pvec);
        
        float inv_det= 1 / det;
        Vector tvec(p, ray.o);
        
        float u= dot(tvec, pvec) * inv_det;
        if(u < 0 || u > 1) return;
        
        Vector qvec= cross(tvec, e1);
        float v= dot(ray.d, qvec) * inv_det;
        if(v < 0 || u + v > 1) return;
        
        float t= dot(e2, qvec) * inv_det;
        if(t < 0 || t > ray.t) return;
        
        // touche !!
        ray.t= t;
        ray.triangle_id= id;
        ray.u= u;
        ray.v= v;
    }

    BBox bounds( ) const 
    {
        BBox box(p);
        return box.insert(p+e1).insert(p+e2);
    }
};

Vector normal( const Mesh& mesh, const Hit& hit )
{
    // recuperer le triangle complet dans le mesh
    const TriangleData& data= mesh.triangle(hit.triangle_id);
    // interpoler la normale avec les coordonn�es barycentriques du point d'intersection
    float w= 1 - hit.u - hit.v;
    Vector n= w * Vector(data.na) + hit.u * Vector(data.nb) + hit.v * Vector(data.nc);
    return normalize(n);
}

Color diffuse_color( const Mesh& mesh, const Hit& hit )
{
    const Material& material= mesh.triangle_material(hit.triangle_id);
    return material.diffuse;
}

struct Source
{
    Point s;
    Color emission;
    int triangle_id;
};

// construction de l'arbre / BVH
struct Node
{
    BBox bounds;
    int left;
    int right;
    
    bool internal( ) const { return right > 0; }                        // renvoie vrai si le noeud est un noeud interne
    int internal_left( ) const { assert(internal()); return left; }     // renvoie le fils gauche du noeud interne 
    int internal_right( ) const { assert(internal()); return right; }   // renvoie le fils droit
    
    bool leaf( ) const { return right < 0; }                            // renvoie vrai si le noeud est une feuille
    int leaf_begin( ) const { assert(leaf()); return -left; }           // renvoie le premier objet de la feuille
    int leaf_end( ) const { assert(leaf()); return -right; }            // renvoie le dernier objet
};

// creation d'un noeud interne
Node make_node( const BBox& bounds, const int left, const int right )
{
    Node node { bounds, left, right };
    assert(node.internal());    // verifie que c'est bien un noeud...
    return node;
}

// creation d'une feuille
Node make_leaf( const BBox& bounds, const int begin, const int end )
{
    Node node { bounds, -begin, -end };
    assert(node.leaf());        // verifie que c'est bien une feuille...
    return node;
}


struct triangle_less1
{
    int axis;
    float cut;
    
    triangle_less1( const int _axis, const float _cut ) : axis(_axis), cut(_cut) {}
    
    bool operator() ( const Triangle& triangle ) const
    {
        // re-construit l'englobant du triangle
        BBox bounds= triangle.bounds();
        return bounds.centroid(axis) < cut;
    }
};


struct BVH
{
    std::vector<Node> nodes;
    std::vector<Triangle> triangles;
    int root;
    
    int direct_tests;
    
    // construit un bvh pour l'ensemble de triangles
    int build( const BBox& _bounds, const std::vector<Triangle>& _triangles )
    {
        triangles= _triangles;  // copie les triangles pour les trier
        nodes.clear();          // efface les noeuds
        nodes.reserve(triangles.size());
        
        // construit l'arbre... 
        root= build(_bounds, 0, triangles.size());
        // et renvoie la racine
        return root;
    }
    
    void intersect( RayHit& ray ) const
    {
        Vector invd= Vector(1 / ray.d.x, 1 / ray.d.y, 1 / ray.d.z);
        intersect(root, ray, invd);
    }
    
    void intersect_fast( RayHit& ray ) const
    {
        Vector invd= Vector(1 / ray.d.x, 1 / ray.d.y, 1 / ray.d.z);
        intersect_fast(root, ray, invd);
    }
    
protected:
    // construction d'un noeud
    int build( const BBox& bounds, const int begin, const int end )
    {
        if(end - begin < 2)
        {
            // inserer une feuille et renvoyer son indice
            int index= nodes.size();
            nodes.push_back(make_leaf(bounds, begin, end));
            return index;
        }
        
        // axe le plus etire de l'englobant
        Vector d= Vector(bounds.pmin, bounds.pmax);
        int axis;
        if(d.x > d.y && d.x > d.z)  // x plus grand que y et z ?
            axis= 0;
        else if(d.y > d.z)          // y plus grand que z ? (et que x implicitement)
            axis= 1;
        else                        // x et y ne sont pas les plus grands...
            axis= 2;

        // coupe l'englobant au milieu
        float cut= bounds.centroid(axis);
        
        // repartit les triangles 
        Triangle *pm= std::partition(triangles.data() + begin, triangles.data() + end, triangle_less1(axis, cut));
        int m= std::distance(triangles.data(), pm);
        
        // la repartition des triangles peut echouer, et tous les triangles sont dans la meme partie... 
        // forcer quand meme un decoupage en 2 ensembles 
        if(m == begin || m == end)
            m= (begin + end) / 2;
        assert(m != begin);
        assert(m != end);
        
        // construire le fils gauche
        // les triangles se trouvent dans [begin .. m)
        BBox bounds_left= triangle_bounds(begin, m);
        int left= build(bounds_left, begin, m);
        
        // on recommence pour le fils droit
        // les triangles se trouvent dans [m .. end)
        BBox bounds_right= triangle_bounds(m, end);
        int right= build(bounds_right, m, end);
        
        int index= nodes.size();
        nodes.push_back(make_node(bounds, left, right));
        return index;
    }
    
    BBox triangle_bounds( const int begin, const int end )
    {
        BBox bbox= triangles[begin].bounds();
        for(int i= begin +1; i < end; i++)
            bbox.insert(triangles[i].bounds());
        
        return bbox;
    }
    
    void intersect( const int index, RayHit& ray, const Vector& invd ) const
    {
        const Node& node= nodes[index];
        if(node.bounds.intersect(ray, invd))
        {
            if(node.leaf())
            {
                for(int i= node.leaf_begin(); i < node.leaf_end(); i++)
                    triangles[i].intersect(ray);
            }
            else // if(node.internal())
            {
                intersect(node.internal_left(), ray, invd);
                intersect(node.internal_right(), ray, invd);
            }
        }
    }
    
    void intersect_fast( const int index, RayHit& ray, const Vector& invd ) const
    {
        const Node& node= nodes[index];
        if(node.leaf())
        {
            for(int i= node.leaf_begin(); i < node.leaf_end(); i++)
                triangles[i].intersect(ray);
        }
        else // if(node.internal())
        {
            const Node& left_node= nodes[node.left];
            const Node& right_node= nodes[node.right];
            
            BBoxHit left= left_node.bounds.intersect(ray, invd);
            BBoxHit right= right_node.bounds.intersect(ray, invd);
            if(left && right)                                                   // les 2 fils sont touches par le rayon...
            {
                if(left.centroid() < right.centroid())                          // parcours de gauche a droite
                {
                    intersect_fast(node.internal_left(), ray, invd);
                    intersect_fast(node.internal_right(), ray, invd);
                }
                else                                                            // parcours de droite a gauche                                        
                {
                    intersect_fast(node.internal_right(), ray, invd);
                    intersect_fast(node.internal_left(), ray, invd);
                }
            }
            else if(left)                                                       // uniquement le fils gauche
                intersect_fast(node.internal_left(), ray, invd);
            else if(right)
                intersect_fast(node.internal_right(), ray, invd);               // uniquement le fils droit
        }
    }
};

Color shadeFlat(const Point o, const Hit hit, const std::vector<Triangle> triangles, const std::vector<Source> sources, const std::vector<Color> diffuse){
    bool isLit = false;

    // Test si le point est visible par au moins une des sources
    for(unsigned int i = 0; i < sources.size(); i++){
        Ray r(o, sources[i].s);
        Hit htemp;
        for(int j= 0; j < int(triangles.size()); j++)
        {
            if(Hit h= triangles[j].intersect(r, htemp.t)){
                htemp = h;
            }
        }
        isLit = isLit || (htemp.triangle_id == sources[i].triangle_id);
    }
    if(isLit)
        return diffuse[hit.triangle_id];
    else
        return Color();
}

Color shade(const int N, std::uniform_real_distribution<float> &u01, std::default_random_engine &random, const Point o, const Vector n, const Mesh mesh,
            const std::vector<Triangle> triangles, const std::vector<Source> sources, const  std::vector<Color> diffuse, const Material mat){
    Color finalColor = Color(0,0,0);
    for(unsigned int i = 0; i < sources.size(); i++){
        for(int k = 0; k < N; k++){
            float u1 = u01(random);
            float u2 = u01(random);
            // on génère un point random sur le triangle
            Point sourcePos = triangles[sources[i].triangle_id].sample18(u1, u2);
            Vector v = Vector(o, sourcePos);
            Ray r(o, normalize(v));
            //on vérifie qu'on voit bien la lumière depuis ce point
            Hit htemp;
            for(int j= 0; j < int(triangles.size()); j++)
            {
                if(Hit h= triangles[j].intersect(r, htemp.t)){
                    htemp = h;
                }
            }
            if(htemp.triangle_id == sources[i].triangle_id){
                //si c'est le cas on applique le calcul de l'éclairage direct
                Color fr = mat.diffuse / M_PI;
                Vector lightNormal = normal(mesh, htemp);
                finalColor = finalColor + sources[i].emission * fr
                           *((std::fmax(0.,dot(n, r.d))*std::fmax(0.,dot(lightNormal, -r.d))) / dot(v, v))
                           * triangles[htemp.triangle_id].aire;
            }
        }
    }
    return mat.emission + finalColor / N;
}

Color occultationAmb(const int N, std::uniform_real_distribution<float> &u01, std::default_random_engine &random, const Point o, const Vector n,
            const std::vector<Triangle> triangles){
    World _w = World(n);
    float occultation = 0.;
    for(int i = 0; i < N; i++){
        Ray r(o,_w(sample35(u01(random), u01(random))));
        Hit htemp;
        for(int j= 0; j < int(triangles.size()); j++)
        {
            if(Hit h= triangles[j].intersect(r, htemp.t)){
                htemp = h;
            }
        }
        if(!htemp)
            occultation += 1.;
    }
    return Color(occultation / N);
}

int main( const int argc, const char **argv )
{
    const char *mesh_filename= "data/cornell.obj";
    if(argc > 1)
        mesh_filename= argv[1];
        
    const char *orbiter_filename= "orbiter.txt";
    if(argc > 2)
        orbiter_filename= argv[2];
    
    Orbiter camera;
    if(camera.read_orbiter(orbiter_filename) < 0)
        return 1;

    Mesh mesh= read_mesh_fast(mesh_filename);

    
    // recupere les triangles
    std::vector<Triangle> triangles;
    {
        int n= mesh.triangle_count();
        for(int i= 0; i < n; i++)
            triangles.emplace_back(mesh.triangle(i), i);
    }
    
    // recupere les materiaux diffus
    std::vector<Color> diffuse;
    
    // recupere les sources
    std::vector<Source> sources;
    {
        int n= mesh.triangle_count();
        for(int i= 0; i < n; i++)
        {
            const Material& material= mesh.triangle_material(i);
            diffuse.push_back(material.diffuse);
            if(material.emission.r + material.emission.g + material.emission.b > 0)
            {
                // utiliser le centre du triangle comme source de lumi�re
                const TriangleData& data= mesh.triangle(i);
                Point p= (Point(data.a) + Point(data.b) + Point(data.c)) / 3;
                
                sources.push_back( { p, material.emission, i} );
            }
        }
        
        printf("%d sources\n", int(sources.size()));
        assert(sources.size() > 0);
    }

    
    Image image(1024, 768);

    // recupere les transformations
    camera.projection(image.width(), image.height(), 45);
    Transform model= Identity();
    Transform view= camera.view();
    Transform projection= camera.projection();
    Transform viewport= camera.viewport();
    Transform inv= Inverse(viewport * projection * view * model);
    
auto startA= std::chrono::high_resolution_clock::now();

    srand(time(0));
    std::random_device seed;
    std::default_random_engine random(seed());
    std::uniform_real_distribution<float> u01(0.f, 1.f);
    

    // std::vector<RayHit> rays;
    // c'est parti, parcours tous les pixels de l'image
    #pragma omp parallel for schedule(dynamic, 1)
    for(int y= 0; y < image.height(); y++)
    for(int x= 0; x < image.width(); x++)
    {
        // generer le rayon
        Point origine= inv(Point(x + .5f, y + .5f, 0));
        Point extremite= inv(Point(x + .5f, y + .5f, 1));
        Ray ray(origine, extremite);

        // rays.emplace_back(origine, extremite, x, y);
        
    #if 1 // CODE SANS BVH
        // calculer les intersections avec tous les triangles
        Hit hit;
        for(int i= 0; i < int(triangles.size()); i++)
        {
            if(Hit h= triangles[i].intersect(ray, hit.t))
                // ne conserve que l'intersection la plus proche de l'origine du rayon
                hit= h;
        }

        if(hit)
        {
            // EXO 2 materiaux diffus //
            // image(x, y) = diffuse[hit.triangle_id];

            Vector n= normal(mesh, hit);
            const Material &mat = mesh.triangle_material(hit.triangle_id);
            Point p = triangles[hit.triangle_id].p*(1 - hit.u - hit.v) +
                      (triangles[hit.triangle_id].p + triangles[hit.triangle_id].e1)*hit.u +
                      (triangles[hit.triangle_id].p + triangles[hit.triangle_id].e2)*hit.v;
            Point o = p + 0.001 * n;

            // EXO 4 ombre et eclairage direct //
            // image(x, y) = shadeFlat(o, hit, triangles, sources, diffuse);


            // EXO 5 pénombre et eclairage direct //
            //image(x, y) = shade(16, u01, random, o, n, mesh, triangles, sources, diffuse, mat);


            // PARTIE 2 OCULTATION AMBIANTE 
            image(x, y) = occultationAmb(16, u01, random, o, n, triangles);
        }
    #endif
    }

/*    BVH bvh;
    {
        auto start= std::chrono::high_resolution_clock::now();
        
        // intersection
        const int n= int(rays.size());
        #pragma omp parallel for schedule(dynamic, 1024)
        for(int i= 0; i < n; i++)
            ;//bvh.intersect_fast(rays[i]);
        
        auto stop= std::chrono::high_resolution_clock::now();
        int cpu= std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
        printf("bvh fast mt %dms\n", cpu);
    }

    for(int i= 0; i < rays.size(); i++)
    {
        if(rays[i])
        {
            image(x, y) = ;
        }
    }*/

    auto stopA= std::chrono::high_resolution_clock::now();
    int cpu= std::chrono::duration_cast<std::chrono::milliseconds>(stopA - startA).count();
    printf("%dms\n", cpu);
    
    write_image(image, "render.png");
    write_image_hdr(image, "shadow.hdr");
    return 0;
}
