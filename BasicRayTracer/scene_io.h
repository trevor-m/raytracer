/********
*
*  scene_io.h
*
*  Description:
*    Contains data structures for reading and writing scene descriptions.
*    The scene descriptions are originally from Inventor applications
*    (a toolkit provided by SGI), however rather than forcing you to know
*    how to write an Inventor application, we convert them for you
*    to the much simpler data structures below.
*
*    Do not modify the data structures in this file.
*    You should define your own structures, and write a procedure that
*    that initializes them by walking through the structures defined here.
*
*  Authors: Brian L. Curless, Eric Veach
*  Date of Version: January 22, 1995
*
*********/

#ifndef _SCENE_IO_
#define _SCENE_IO_

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#include <stdio.h>


    /* Definitions of fundamental types a la Heckbert	*/
    /*  in Glassner''s _Introduction_to_Ray_Tracing_	*/

typedef float Flt;	    /* Inventor requires float not double!!	*/
			    /*   Could be coerced to double with effort */
typedef Flt Vec[3];	    /* Vector in 3-space (nx, ny, nz)		*/
typedef Vec Point;	    /* Point in 3-space (x, y, z)		*/
typedef Vec Color;	    /* Float rep. of a color (r, g, b) where	*/
			    /*   0.0 <= r,g,b <= 1.0.			*/


    /* Definition of the root of the scene description */

typedef struct SceneIO {
    struct CameraIO *camera;  /* Perspective camera		      */
    struct LightIO *lights;   /* Head of the linked list of lights    */
    struct ObjIO *objects;    /* Head of the linked list of objects   */
} SceneIO;


    /* Definition of a perspective camera (projection/viewing) */

typedef struct CameraIO {
    Point position;	/* Position of viewer				*/
    Vec viewDirection;	/* Viewing direction				*/
    Flt focalDistance;	/* Distance from viewer to focal plane		*/
    Vec orthoUp;	/* Up direction, orthogonal to viewDirection	*/
    Flt verticalFOV;	/* Total vertical field of view, in radians	*/
} CameraIO;


    /* We allow point, directional, and spot lights. */

enum LightType {POINT_LIGHT, DIRECTIONAL_LIGHT, SPOT_LIGHT};

typedef struct LightIO {
    struct LightIO *next; /* Pointer to next light in linked list	*/
    enum LightType type;  /* Point, directional, or spot light		*/
    
    Point position;	/* Position of a point light source		*/
    Vec direction;	/* Direction of a directional light source	*/
    Color color;	/* Color and intensity				*/
    
    Flt dropOffRate;	/* For spot lights, 0 <= x <= 1.  See man pages */
    Flt cutOffAngle;	/* Angle at which spot light intensity is zero  */
} LightIO;


    /* Definition of a generic object.		*/
    /* We support spheres and sets of polygons. */

enum ObjType {SPHERE_OBJ, POLYSET_OBJ};

typedef struct ObjIO {
    struct ObjIO *next;		/* Pointer to next object node	    */
    char *name;                 /* Name of the object               */

    /* Most objects are made out of one material.  However, there */
    /* may be several materials for one object if per-vertex materials */
    /* are used.  Each vertex gives an index into the material list. */
    /* "material" is a pointer to an array of MaterialIO structures */
    /* (although often this array will have length 1). */

    long numMaterials;
    struct MaterialIO *material;
    
    enum ObjType type;		/* See enum ObjType		    */
    void *data;			/* data for this type (see below)   */
} ObjIO;


    /* Definition of material parameters assignable in Inventor		*/
    /*									*/
    /* Note that Inventor does not supply an index of refraction.	*/
    /* Of course, you can interpret each of these fields any way you	*/
    /* like.  For example, in a ray tracer the "emissive" and "ambient"	*/
    /* colors are pretty much the same thing, so you could interpret	*/
    /* one of them as something else.                                   */
    /*									*/
    /* If you need more parameters, you can hide them in the object 	*/
    /* name ("composer" and "i3dm" let you set this), and parse the	*/
    /* name as a string.						*/

typedef struct MaterialIO {
    Color diffColor;	/* Diffuse color				*/
    Color ambColor;	/* Ambient color				*/
    Color specColor;	/* Specular color 				*/
    Color emissColor;	/* Emissive color				*/
    Flt shininess;	/* Shininess: 0.0 - 1.0.  Must be scaled (multiply by 128) before use as a Phong 
					   cosine exponent (q in our equation).  */
    Flt ktran;		/* Transparency: 0.0 - 1.0			*/
} MaterialIO;


    /* Definition of a spherical object.			    */

    /* In fact, due to the ability to scale the axes		    */
    /* independently, spheres can be changed to ellipsoids.	    */
    /* Accordingly, the axis orientations are defined here	    */
    /* as well as their lengths.  The "radius" is defined here      */
    /* for convenience as half the average axis length, which is    */
    /* exact for spheres.					    */

typedef struct SphereIO {
    Point origin;   /* Origin of the sphere/ellipsoid		*/
    Flt radius;	    /* Half the sum of the axis lengths		*/
    
    Vec xaxis;	    /* Direction of the x axis			*/
    Flt xlength;    /* Length of the x axis			*/
    Vec yaxis;	    /* Direction of the y axis			*/
    Flt ylength;    /* Length of the y axis			*/
    Vec zaxis;	    /* Direction of the z axis			*/
    Flt zlength;    /* Length of the z axis			*/
} SphereIO;



/* Definition of a vertex */

typedef struct VertexIO {
    Point pos;			/* Vertex position */
    Vec norm;			/* Normal to vertex (if PER_VERTEX_NORMAL) */
    long materialIndex;		/* Index into materials list (see ObjIO), */
                                /* if material is PER_VERTEX_MATERIAL */
    Flt s, t;			/* Texture coordinates (if hasTextureCoords) */
} VertexIO;


    /* Definition of a polygon */

typedef struct PolygonIO {
    long numVertices;		/* Number of vertices			*/
    struct VertexIO *vert;	/* Vertex array				*/
} PolygonIO;


    /* Definition of a polygonal set.					*/
    /* We support 3 types:						*/
    /*									*/
    /*   Triangle mesh --						*/
    /*     Consists only of triangles.  There is not necessarily any	*/
    /*	   structure to the triangles; although the triangles will	*/
    /*     usually be from a single modelling primitive.		*/
    /*									*/
    /*   Face set --							*/
    /*	   A set of polygons with any number of sides.  They will	*/
    /*	   always have per-face normals and per-object materials.	*/
    /*									*/
    /*   Quad mesh --							*/
    /*	   All "polygons" have four sides, but are NOT necessarily	*/
    /*     planar, i.e. the 4 vertices of each quad may not lie in	*/
    /*     the same plane.  						*/

enum PolySetType {POLYSET_TRI_MESH, POLYSET_FACE_SET, POLYSET_QUAD_MESH};


    /* Objects (polygonal sets in particular) may have normals */
    /*   at every vertex for smooth shading of meshed objects  */
    /*   or at every face for sharp-edged shading as in boxes  */

enum NormType {PER_VERTEX_NORMAL, PER_FACE_NORMAL};


    /* We support two kinds of material bindings:			*/
    /*	PER_OBJECT    one material for the whole object			*/
    /*  PER_VERTEX    a material is defined at each vertex, and you	*/
    /*			must interpolate across each face.		*/
    /*									*/
    /* PER_VERTEX obviously only applies to polygonal objects.		*/

enum MaterialBinding {PER_OBJECT_MATERIAL, PER_VERTEX_MATERIAL};


typedef struct PolySetIO {
    enum PolySetType		type;
    enum NormType		normType;
    enum MaterialBinding	materialBinding;
    int /* boolean */		hasTextureCoords;
    
    /* Sometimes it may be useful to know when the polygonal set is
     * organized into a rectangular pattern.  If the primitives are
     * are grouped into rows of equal length, "rowSize" gives the
     * number of primitives in each row, otherwise "rowSize" is 0.
     * The number of rows is (numPolys / rowSize), an integer.
     */
    long rowSize;     	    /* Number of primitives per row		*/
    long numPolys;	    /* Number of polygons		   	*/
    struct PolygonIO *poly; /* Polygonal array			   	*/
} PolySetIO;


#ifdef __cplusplus
extern "C" {
#endif

/* SceneIO *readScene(filename)
 *    - loads a scene description
 *
 * void delete_scene(scene)
 *    - call this when you are finished with a scene returned by
 *      read_scene()
 */

SceneIO *readScene(const char *filename);
void deleteScene(SceneIO *);


/* The following routines are used to construct new scenes.  They are
 * used by "composer" when translating Inventor files.  You should not
 * need to access them.
 */

struct SceneIO *new_scene(void);
struct CameraIO *new_camera(void);
struct LightIO *new_light(void);
struct ObjIO *new_object(void);
struct MaterialIO *new_material(long n);

struct LightIO *append_light(struct LightIO **);
struct ObjIO *append_object(struct ObjIO **);

/* writeSceneAscii() writes a scene in an ASCII format.  This format is
 * useful for debugging, or transferring scene files between different
 * machine architectures.  By writing to the file "/dev/tty" you can
 * write to the terminal window in which your application was invoked.
 *
 * writeSceneBinary() writes a scene in binary format; the Export function
 * in composer uses this format to write the scene.  This format can only
 * be read back on a machine that uses the same format for integers
 * and floating-point numbers.
 */
void writeSceneAscii(struct SceneIO *, const char *);
void writeSceneBinary(struct SceneIO *, const char *);

#ifdef __cplusplus
}
#endif

/* For interaction with the "composer" tool */

#define COMPOSER_EXPORT_REQUESTED	0
#define COMPOSER_EXPORT_SENDING		1
#define COMPOSER_EXPORT_DONE		2

#define COMPOSER_DEFAULT_EXPORT_NAME	"composer.out"


#endif


