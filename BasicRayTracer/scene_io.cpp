#include <malloc.h>
#include "scene_io.h"
#include <string.h>

static SceneIO *readSceneA(FILE *fp);
static SceneIO *readSceneB(FILE *fp);

static void write_cameraA(CameraIO *, FILE *);
static void read_cameraA(SceneIO *scene, FILE *fp);
static void write_cameraB(CameraIO *, FILE *);
static CameraIO *read_cameraB(FILE *);
static void delete_camera(CameraIO *);

static void write_lightsA(LightIO *, FILE *);
static void write_lightA(LightIO *, FILE *);
static void read_point_lightA(SceneIO *scene, FILE *fp);
static void read_directional_lightA(SceneIO *scene, FILE *fp);
static void read_spot_lightA(SceneIO *scene, FILE *fp);
static void write_lightsB(LightIO *, FILE *);
static void write_lightB(LightIO *, FILE *);
static LightIO *read_lightsB(FILE *);
static LightIO *read_lightB(FILE *);
static int get_num_lights(LightIO *);
static void delete_lights(LightIO *);

static void write_objectsA(ObjIO *, FILE *);
static void write_objectA(ObjIO *, FILE *);
static void read_objectA(ObjIO *obj, FILE *fp);
static void write_objectsB(ObjIO *, FILE *);
static void write_objectB(ObjIO *, FILE *);
static ObjIO *read_objectsB(FILE *);
static ObjIO *read_objectB(FILE *);
static int get_num_objects(ObjIO *);
static void delete_objects(ObjIO *);

static void write_materialA(MaterialIO *, FILE *);
static void read_materialA(MaterialIO *material, FILE *fp);
static void write_materialB(MaterialIO *, FILE *);
static void read_materialB(MaterialIO *material, FILE *fp);

static void write_sphereA(ObjIO *obj, FILE *fp);
static void read_sphereA(SceneIO *scene, FILE *fp);
static void write_sphereB(ObjIO *obj, FILE *fp);
static void read_sphereB(ObjIO *obj, FILE *fp);
static void delete_sphere(SphereIO *);

static void write_poly_setA(ObjIO *obj, FILE *fp);
static void read_poly_setA(SceneIO *scene, FILE *fp);
static void write_poly_setB(ObjIO *obj, FILE *fp);
static void read_poly_setB(ObjIO *obj, FILE *fp);
static void delete_poly_set(PolySetIO *);

#define VERSION_STRING "Composer format"
#define THIS_VERSION	2.1
#define MAX_NAME	5000

#define CHECK(nE,nA)	if ((nE) != (nA)) { \
            printf( "Error during scanf: file '%s', line %d\n", \
		    __FILE__, __LINE__ ); } else /* eat semicolon */

static double Version;

SceneIO *newScene(void) {
  return (SceneIO *)calloc(1,sizeof(SceneIO));
}


SceneIO *readScene(const char *filename) {
	FILE *fp;
	char format[50], type[20];
	
	fopen_s(&fp, filename, "rb");
	SceneIO *scene = NULL;

	if (fp == NULL) {
		printf( "Can't open file '%s' for reading.\n", filename );
		return NULL;
	}

	strcpy(format,VERSION_STRING);
	strcat(format," %lg %10s\n");

	if (fscanf(fp,format,&Version,type) != 2) {
		printf( "File '%s' has wrong format.\n", filename );
	} else if (Version > THIS_VERSION) {
		printf( "Error: file '%s' is version %g, program is version %g.\n",
			filename, Version, THIS_VERSION );
	} else if (strcmp(type,"binary") == 0) {
		scene = readSceneB(fp);
	} else if (strcmp(type,"ascii") == 0) {
		scene = readSceneA(fp);
	} else {
		printf( "Error: unrecognized file type (neither ascii or binary).\n" );
	}

	fclose(fp);
	return scene;
}

void writeSceneAscii(SceneIO *scene, const char *filename) {
	FILE *fp = fopen(filename, "w");
	if (fp == NULL) {
		printf("Can't open file '%s' for writing.\n", filename);
		return;
	}
	
	fprintf(fp, "%s %g ascii\n", VERSION_STRING, THIS_VERSION);
	write_cameraA(scene->camera, fp);
	write_lightsA(scene->lights, fp);
	write_objectsA(scene->objects, fp);    
	fclose(fp);
	return;
}


static SceneIO *readSceneA(FILE *fp) {
  SceneIO *scene = newScene();
  char word[100];

  while( fscanf(fp, "%s", word) == 1 ) {
    if (strcmp(word, "camera") == 0) {
      read_cameraA(scene,fp);
    } else if (strcmp(word, "point_light") == 0) {
      read_point_lightA(scene,fp);
    } else if (strcmp(word, "directional_light") == 0) {
      read_directional_lightA(scene,fp);
    } else if (strcmp(word, "spot_light") == 0) {
      read_spot_lightA(scene,fp);
    } else if (strcmp(word, "sphere") == 0) {
      read_sphereA(scene,fp);
    } else if (strcmp(word, "poly_set") == 0) {
      read_poly_setA(scene,fp);
    } else {
      printf( "Unrecognized keyword '%s', aborting.\n", word );
      deleteScene(scene);
      return NULL;
    }
  }
  return scene;
}


static long Test_long = 123456789;
static Flt Test_Flt = (float)3.1415926;
  
void writeSceneBinary(SceneIO *scene, const char *filename) {
  FILE *fp = fopen(filename, "wb");
  if (fp == NULL) {
    printf("Can't open file '%s' for writing.\n", filename);
    return;
  }
  fprintf(fp, "%s %g binary\n", VERSION_STRING, THIS_VERSION);

  /* Make sure integer and floating-point formats are compatible */
  fwrite(&Test_long, sizeof(long), 1, fp);
  fwrite(&Test_Flt, sizeof(Flt), 1, fp);
  
  write_cameraB(scene->camera, fp);
  write_lightsB(scene->lights, fp);
  write_objectsB(scene->objects, fp);    
  fclose(fp);
}

static SceneIO *readSceneB(FILE *fp) {
  SceneIO *scene = newScene();
  long in_long;
  Flt in_Flt;

  if (Version >= 2.1) {
    /* Make sure integer and floating-point formats are compatible */
    fread(&in_long, sizeof(long), 1, fp);
    fread(&in_Flt, sizeof(Flt), 1, fp);

    if (in_long != Test_long || in_Flt != Test_Flt) {
      printf( "Binary format was written on a different architecture!\n" );
      return NULL;
    }
  }
  scene->camera = read_cameraB(fp);
  scene->lights = read_lightsB(fp);
  scene->objects = read_objectsB(fp);

  return scene;
}

void deleteScene(SceneIO *scene) {
	delete_camera(scene->camera);
	delete_lights(scene->lights);
	delete_objects(scene->objects);
	free(scene);    
}


CameraIO *
new_camera(void)
{
  return (CameraIO *)calloc(1,sizeof(CameraIO));
}

static void
write_cameraA(CameraIO *camera, FILE *fp)
{
  if (camera == NULL) return;
  
  fprintf(fp,"camera {\n");
  fprintf(fp,"  position %g %g %g\n", camera->position[0],
	  camera->position[1], camera->position[2]);
  fprintf(fp,"  viewDirection %g %g %g\n", camera->viewDirection[0],
	  camera->viewDirection[1], camera->viewDirection[2]);
  fprintf(fp,"  focalDistance %g\n", camera->focalDistance);
  fprintf(fp,"  orthoUp %g %g %g\n", camera->orthoUp[0],
	  camera->orthoUp[1], camera->orthoUp[2]);
  fprintf(fp,"  verticalFOV %g\n", camera->verticalFOV);
  fprintf(fp,"}\n");
}


static void
read_cameraA(SceneIO *scene, FILE *fp)
{
  CameraIO *camera = new_camera();
  
  scene->camera = camera;

  fscanf(fp," {");
  CHECK(3, fscanf(fp," position %g %g %g", &camera->position[0],
		  &camera->position[1], &camera->position[2]));
  CHECK(3, fscanf(fp," viewDirection %g %g %g", &camera->viewDirection[0],
		  &camera->viewDirection[1], &camera->viewDirection[2]));
  CHECK(1, fscanf(fp," focalDistance %g", &camera->focalDistance));
  CHECK(3, fscanf(fp," orthoUp %g %g %g", &camera->orthoUp[0],
		  &camera->orthoUp[1], &camera->orthoUp[2]));
  CHECK(1, fscanf(fp," verticalFOV %g", &camera->verticalFOV));
  fscanf(fp," }");
}


static void
write_cameraB(CameraIO *camera, FILE *fp)
{
  if (camera == NULL) {
    camera = new_camera();	/* Write zeros */
  }
  fwrite(&camera->position, sizeof(Point), 1, fp);
  fwrite(&camera->viewDirection, sizeof(Vec), 1, fp);
  fwrite(&camera->focalDistance, sizeof(Flt), 1, fp);
  fwrite(&camera->orthoUp, sizeof(Vec), 1, fp);
  fwrite(&camera->verticalFOV, sizeof(Flt), 1, fp);
}


static CameraIO *
read_cameraB(FILE *fp)
{
  CameraIO *camera;

  camera = new_camera();

  fread(&camera->position, sizeof(Point), 1, fp);
  fread(&camera->viewDirection, sizeof(Vec), 1, fp);
  fread(&camera->focalDistance, sizeof(Flt), 1, fp);
  fread(&camera->orthoUp, sizeof(Vec), 1, fp);
  fread(&camera->verticalFOV, sizeof(Flt), 1, fp);

  if (camera->verticalFOV == 0.0) {
    /* Invalid camera -- probably we wrote all zeros (see above) */
    return NULL;
  }
  return camera;
}


static void
delete_camera(CameraIO *camera)
{
  if (camera != NULL) {
    free(camera);
  }
}


LightIO *
new_light(void)
{
  return (LightIO *)calloc(1,sizeof(LightIO));
}


LightIO *
append_light(LightIO **plights)
{
  if (*plights == NULL) {
    *plights = new_light();
    return *plights;
  } else {
    LightIO *lts;

    lts = *plights;
    while (lts->next != NULL) {
      lts = lts->next;
    }
    lts->next = new_light();

    return lts->next;
  }
}


static void
write_lightsA(LightIO *lts, FILE *fp)
{
  while (lts != NULL) {
    write_lightA(lts,fp);
    lts = lts->next;
  }
}


static void
write_lightA(LightIO *light, FILE *fp)
{
  if (light->type == POINT_LIGHT) {
    fprintf(fp,"point_light {\n");
  } else if (light->type == DIRECTIONAL_LIGHT) {
    fprintf(fp,"directional_light {\n");
  } else if (light->type == SPOT_LIGHT) {
    fprintf(fp,"spot_light {\n");
  } else {
    printf("error -- unrecognized light type.\n");
  }
  if (light->type != DIRECTIONAL_LIGHT) {
    fprintf(fp,"  position %g %g %g\n", light->position[0],
	    light->position[1], light->position[2]);
  }
  if (light->type != POINT_LIGHT) {
    fprintf(fp,"  direction %g %g %g\n", light->direction[0],
	    light->direction[1], light->direction[2]);
  }
  fprintf(fp,"  color %g %g %g\n", light->color[0],
	  light->color[1], light->color[2]);
  if (light->type == SPOT_LIGHT) {
    fprintf(fp,"  dropOffRate %g\n", light->dropOffRate);
    fprintf(fp,"  cutOffAngle %g\n", light->cutOffAngle);
  }
  fprintf(fp,"}\n");
}


static void
read_point_lightA(SceneIO *scene, FILE *fp)
{
  LightIO *light = append_light(&scene->lights);

  light->type = POINT_LIGHT;
  fscanf(fp," {");
  CHECK(3, fscanf(fp," position %g %g %g", &light->position[0],
		  &light->position[1], &light->position[2]));
  CHECK(3, fscanf(fp," color %g %g %g", &light->color[0],
		  &light->color[1], &light->color[2]));
  fscanf(fp," }");
}


static void
read_directional_lightA(SceneIO *scene, FILE *fp)
{
  LightIO *light = append_light(&scene->lights);

  light->type = DIRECTIONAL_LIGHT;
  fscanf(fp," {");
  CHECK(3, fscanf(fp," direction %g %g %g", &light->direction[0],
		  &light->direction[1], &light->direction[2]));
  CHECK(3, fscanf(fp," color %g %g %g", &light->color[0],
		  &light->color[1], &light->color[2]));
  fscanf(fp," }");
}


static void
read_spot_lightA(SceneIO *scene, FILE *fp)
{
  LightIO *light = append_light(&scene->lights);

  light->type = SPOT_LIGHT;
  fscanf(fp," {");
  CHECK(3, fscanf(fp," position %g %g %g", &light->position[0],
		  &light->position[1], &light->position[2]));
  CHECK(3, fscanf(fp," direction %g %g %g", &light->direction[0],
		  &light->direction[1], &light->direction[2]));
  CHECK(3, fscanf(fp," color %g %g %g", &light->color[0],
		  &light->color[1], &light->color[2]));
  CHECK(1, fscanf(fp," dropOffRate %g", &light->dropOffRate));
  CHECK(1, fscanf(fp," cutOffAngle %g", &light->cutOffAngle));
  fscanf(fp," }");
}


static void write_lightsB(LightIO *lights, FILE *fp) {
  int num_lights;
  LightIO *lts;

  lts = lights;

  num_lights = get_num_lights(lts);
  fwrite(&num_lights, sizeof(long), 1, fp);

  while (lts != NULL) {
    write_lightB(lts, fp);
    lts = lts->next;
  }
}


static void
write_lightB(LightIO *light, FILE *fp)
{
  fwrite(&light->type, sizeof(int), 1, fp);
  fwrite(&light->position, sizeof(Point), 1, fp);
  fwrite(&light->direction, sizeof(Vec), 1, fp);
  fwrite(&light->color, sizeof(Color), 1, fp);
  fwrite(&light->dropOffRate, sizeof(Flt), 1, fp);
  fwrite(&light->cutOffAngle, sizeof(Flt), 1, fp);
}


static LightIO *
read_lightsB(FILE *fp)
{
  long i, num_lights;
  LightIO *lights, *lts;

  lights = NULL;

  fread(&num_lights, sizeof(long), 1, fp);
  if (num_lights == 0) return lights;

  lights = read_lightB(fp);
  if (num_lights == 1) return lights;

  lts = lights;
  for (i = 1; i < num_lights; i++) {
    lts->next = read_lightB(fp);
    lts = lts->next;
  }
  return lights;
}


static LightIO *
read_lightB(FILE *fp)
{
  LightIO *light;

  light = new_light();

  fread(&light->type, sizeof(int), 1, fp);
  fread(&light->position, sizeof(Point), 1, fp);
  fread(&light->direction, sizeof(Vec), 1, fp);
  fread(&light->color, sizeof(Color), 1, fp);
  fread(&light->dropOffRate, sizeof(Flt), 1, fp);
  fread(&light->cutOffAngle, sizeof(Flt), 1, fp);
  return light;
}


static int
get_num_lights(LightIO *lights)
{
  int num_lights;
  LightIO *lts;

  lts = lights;
  num_lights = 0;

  while (lts != NULL) {
    lts = lts->next;
    num_lights++;
  }
  return num_lights;
}


static void
delete_lights(LightIO *lights)
{
  if (lights == NULL)
    return;
  else
    delete_lights(lights->next);

  free(lights);
}


ObjIO *
new_object(void)
{
  ObjIO *object;

  object = (ObjIO *)calloc(1,sizeof(ObjIO));    
  object->name = NULL;

  return object;
}

ObjIO *
append_object(ObjIO **pobjects)
{
  if (*pobjects == NULL) {
    *pobjects = new_object();
    return *pobjects;
  } else {
    ObjIO *objs;

    objs = *pobjects;
    while (objs->next != NULL) {
      objs = objs->next;
    }
    objs->next = new_object();
    return objs->next;
  }
}


static void
write_objectsA(ObjIO *obj, FILE *fp)
{
  while (obj != NULL) {
    if (obj->type == SPHERE_OBJ) {
     write_sphereA(obj,fp);
    } else if (obj->type == POLYSET_OBJ) {
      write_poly_setA(obj,fp);
    } else {
      printf( "error -- unrecognized object type\n" );
    }
    obj = obj->next;
  }
}


static void
write_objectA(ObjIO *obj, FILE *fp)
{
  int i;

  if (obj->name == NULL) {
    fprintf(fp,"  name NULL\n");
  } else {
    fprintf(fp,"  name \"%s\"\n", obj->name);
  }
  fprintf(fp,"  numMaterials %ld\n", obj->numMaterials);
  for (i=0; i < obj->numMaterials; ++i) {
    write_materialA(obj->material + i, fp);
  }
}

static void
read_objectA(ObjIO *obj, FILE *fp)
{
  char word[MAX_NAME];
  int i;
  
  CHECK(1, fscanf(fp," name %[^\n]",word));
  if (strcmp(word,"NULL") == 0) {
    obj->name = NULL;
  } else {
    if (word[0] != '\"' || word[strlen(word)-1] != '\"' ) {
      printf("Error in object name format: %s\n", word);
      obj->name = NULL;
    } else {
      word[strlen(word)-1] = '\0';	/* eat trailing quote */
      obj->name = _strdup(word+1);	/* eat leading quote */
    }
  }
  CHECK(1,fscanf(fp," numMaterials %ld",&obj->numMaterials));
  obj->material = new_material(obj->numMaterials);
  for (i=0; i < obj->numMaterials; ++i) {
    read_materialA(obj->material + i, fp);
  }
}


static void
write_objectsB(ObjIO *obj, FILE *fp)
{
  long num_objects;

  num_objects = get_num_objects(obj);
  fwrite(&num_objects, sizeof(long), 1, fp);

  while (obj != NULL) {
    if (obj->type == SPHERE_OBJ) {
     write_sphereB(obj,fp);
    } else if (obj->type == POLYSET_OBJ) {
      write_poly_setB(obj,fp);
    } else {
      printf( "error -- unrecognized object type\n" );
    }
    obj = obj->next;
  }
}


static void
write_objectB(ObjIO *obj, FILE *fp)
{
  long name_length, i;

  fwrite(&obj->type, sizeof(int), 1, fp);
  if (obj->name == NULL) {
    name_length = -1;
    fwrite(&name_length, sizeof(long), 1, fp);
  } else {
    name_length = strlen(obj->name);
    fwrite(&name_length, sizeof(long), 1, fp);
    fwrite(obj->name, name_length + 1, 1, fp);
  }
  fwrite(&obj->numMaterials, sizeof(long), 1, fp);
  for (i = 0; i < obj->numMaterials; ++i) {
    write_materialB(obj->material + i, fp);
  }
}


static ObjIO *
read_objectsB(FILE *fp)
{
  long i, num_objects;
  ObjIO *objects, *objs;

  objects = NULL;

  fread(&num_objects, sizeof(long), 1, fp);
  if (num_objects == 0) return objects;

  objects = read_objectB(fp);
  if (num_objects == 1) return objects;

  objs = objects;
  for (i = 1; i < num_objects; i++) {	
    objs->next = read_objectB(fp);
    objs = objs->next;
  }
  return objects;
}


static ObjIO *
read_objectB(FILE *fp)
{
  long name_length;
  ObjIO *obj;
  long i;

  obj = new_object();
  fread(&obj->type, sizeof(int), 1, fp);
  fread(&name_length, sizeof(long), 1, fp);
  if (name_length == -1) {
    obj->name = NULL;
  } else {
    obj->name = (char *)malloc((name_length+1)*sizeof(char));
    fread(obj->name, name_length + 1, 1, fp);
  }
  if (Version <= 2.0) {
    obj->numMaterials = 1;
  } else {
    fread(&obj->numMaterials, sizeof(long), 1, fp);
  }
  obj->material = new_material(obj->numMaterials);
  for (i = 0; i < obj->numMaterials; ++i) {
    read_materialB(obj->material + i, fp);
  }
  
  if( obj->type == SPHERE_OBJ ) {
    read_sphereB(obj,fp);
  } else if( obj->type == POLYSET_OBJ ) {
    read_poly_setB(obj,fp);
  } else {
    printf( "Error -- unrecognized object type\n" );
  }
  return obj;
}


static int
get_num_objects(ObjIO *objects)
{
  int num_objects;
  ObjIO *objs;

  objs = objects;
  num_objects = 0;

  while (objs != NULL) {
    objs = objs->next;
    num_objects++;
  }
  return num_objects;
}


static void
delete_objects(ObjIO *obj)
{
  if (obj == NULL)
    return;
  else
    delete_objects(obj->next);

  if( obj->type == SPHERE_OBJ ) {
    delete_sphere((SphereIO *)obj->data);
  } else if( obj->type == POLYSET_OBJ ) {
    delete_poly_set((PolySetIO *)obj->data);
  } else {
    printf( "Error -- unrecognized object type\n" );
  }
  free(obj->material);
  if (obj->name != NULL) {
    free(obj->name);
  }
  free(obj);
}


MaterialIO *
new_material(long n)
{
  return (MaterialIO *)calloc((size_t)n,sizeof(MaterialIO));
}


static void
write_materialA(MaterialIO *material, FILE *fp)
{
  fprintf(fp,"  material {\n");
  fprintf(fp,"    diffColor %g %g %g\n", material->diffColor[0],
	  material->diffColor[1], material->diffColor[2]);
  fprintf(fp,"    ambColor %g %g %g\n", material->ambColor[0],
	  material->ambColor[1], material->ambColor[2]);
  fprintf(fp,"    specColor %g %g %g\n", material->specColor[0],
	  material->specColor[1], material->specColor[2]);
  fprintf(fp,"    emisColor %g %g %g\n", material->emissColor[0],
	  material->emissColor[1], material->emissColor[2]);
  fprintf(fp,"    shininess %g\n", material->shininess);
  fprintf(fp,"    ktran %g\n", material->ktran);
  fprintf(fp,"  }\n");
}


static void
read_materialA(MaterialIO *material, FILE *fp)
{
  fscanf(fp," material {");
  CHECK(3, fscanf(fp," diffColor %g %g %g", &material->diffColor[0],
		  &material->diffColor[1], &material->diffColor[2]));
  CHECK(3, fscanf(fp," ambColor %g %g %g", &material->ambColor[0],
		  &material->ambColor[1], &material->ambColor[2]));
  CHECK(3, fscanf(fp," specColor %g %g %g", &material->specColor[0],
		  &material->specColor[1], &material->specColor[2]));
  CHECK(3, fscanf(fp," emisColor %g %g %g", &material->emissColor[0],
		  &material->emissColor[1], &material->emissColor[2]));
  CHECK(1, fscanf(fp," shininess %g", &material->shininess));
  CHECK(1, fscanf(fp," ktran %g", &material->ktran));
  fscanf(fp," }");
}


static void
write_materialB(MaterialIO *material, FILE *fp)
{
  fwrite(&material->diffColor, sizeof(Color), 1, fp);
  fwrite(&material->ambColor, sizeof(Color), 1, fp);
  fwrite(&material->specColor, sizeof(Color), 1, fp);
  fwrite(&material->emissColor, sizeof(Color), 1, fp);
  fwrite(&material->shininess, sizeof(Flt), 1, fp);
  fwrite(&material->ktran, sizeof(Flt), 1, fp);
}


static void
read_materialB(MaterialIO *material, FILE *fp)
{
  fread(&material->diffColor, sizeof(Color), 1, fp);
  fread(&material->ambColor, sizeof(Color), 1, fp);
  fread(&material->specColor, sizeof(Color), 1, fp);
  fread(&material->emissColor, sizeof(Color), 1, fp);
  fread(&material->shininess, sizeof(Flt), 1, fp);
  fread(&material->ktran, sizeof(Flt), 1, fp);
}


static void
write_sphereA(ObjIO *obj, FILE *fp)
{
  SphereIO *sphere = (SphereIO *)obj->data;
  
  fprintf(fp, "sphere {\n" );
  write_objectA(obj,fp);
  fprintf(fp,"  origin %g %g %g\n", sphere->origin[0],
	  sphere->origin[1], sphere->origin[2]); 
  fprintf(fp,"  radius %g\n", sphere->radius);
  fprintf(fp,"  xaxis %g %g %g\n", sphere->xaxis[0],
	  sphere->xaxis[1], sphere->xaxis[2]); 
  fprintf(fp,"  xlength %g\n", sphere->xlength);
  fprintf(fp,"  yaxis %g %g %g\n", sphere->yaxis[0],
	  sphere->yaxis[1], sphere->yaxis[2]); 
  fprintf(fp,"  ylength %g\n", sphere->ylength);
  fprintf(fp,"  zaxis %g %g %g\n", sphere->zaxis[0],
	  sphere->zaxis[1], sphere->zaxis[2]); 
  fprintf(fp,"  zlength %g\n", sphere->zlength);
  fprintf(fp,"}\n");
}


static void
read_sphereA(SceneIO *scene, FILE *fp)
{
  ObjIO *obj = append_object(&scene->objects);
  SphereIO *sphere = (SphereIO *)calloc(1,sizeof(SphereIO));

  obj->type = SPHERE_OBJ;
  obj->data = sphere;
  
  fscanf(fp, " {" );
  read_objectA(obj,fp);
  fscanf(fp," origin %g %g %g", &sphere->origin[0],
	  &sphere->origin[1], &sphere->origin[2]); 
  fscanf(fp," radius %g", &sphere->radius);
  fscanf(fp," xaxis %g %g %g", &sphere->xaxis[0],
	  &sphere->xaxis[1], &sphere->xaxis[2]); 
  fscanf(fp," xlength %g", &sphere->xlength);
  fscanf(fp," yaxis %g %g %g", &sphere->yaxis[0],
	  &sphere->yaxis[1], &sphere->yaxis[2]); 
  fscanf(fp," ylength %g", &sphere->ylength);
  fscanf(fp," zaxis %g %g %g", &sphere->zaxis[0],
	  &sphere->zaxis[1], &sphere->zaxis[2]); 
  fscanf(fp," zlength %g", &sphere->zlength);
  fscanf(fp," }");
}


static void
write_sphereB(ObjIO *obj, FILE *fp)
{
  SphereIO *sphere = (SphereIO *)obj->data;
  
  write_objectB(obj,fp);
  fwrite(&sphere->origin, sizeof(Point), 1, fp);
  fwrite(&sphere->radius, sizeof(Flt), 1, fp);
  fwrite(&sphere->xaxis, sizeof(Vec), 1, fp);
  fwrite(&sphere->xlength, sizeof(Flt), 1, fp);
  fwrite(&sphere->yaxis, sizeof(Vec), 1, fp);
  fwrite(&sphere->ylength, sizeof(Flt), 1, fp);
  fwrite(&sphere->zaxis, sizeof(Vec), 1, fp);
  fwrite(&sphere->zlength, sizeof(Flt), 1, fp);
}


static void
read_sphereB(ObjIO *obj, FILE *fp)
{
  SphereIO *sphere;

  sphere = (SphereIO *)calloc(1,sizeof(SphereIO));
  obj->data = sphere;

  fread(&sphere->origin, sizeof(Point), 1, fp);
  fread(&sphere->radius, sizeof(Flt), 1, fp);
  fread(&sphere->xaxis, sizeof(Vec), 1, fp);
  fread(&sphere->xlength, sizeof(Flt), 1, fp);
  fread(&sphere->yaxis, sizeof(Vec), 1, fp);
  fread(&sphere->ylength, sizeof(Flt), 1, fp);
  fread(&sphere->zaxis, sizeof(Vec), 1, fp);
  fread(&sphere->zlength, sizeof(Flt), 1, fp);
}


static void
delete_sphere(SphereIO *sphere)
{
  free(sphere);
}


static void
write_poly_setA(ObjIO *obj, FILE *fp)
{
  PolySetIO *pset = (PolySetIO *)obj->data;
  PolygonIO *poly;
  VertexIO *vert;
  int i, j;

  fprintf(fp, "poly_set {\n" );
  write_objectA(obj,fp);
  switch (pset->type) {
  case POLYSET_TRI_MESH:
    fprintf(fp,"  type POLYSET_TRI_MESH\n" );
    break;
  case POLYSET_FACE_SET:
    fprintf(fp,"  type POLYSET_FACE_SET\n" );
    break;
  case POLYSET_QUAD_MESH:
    fprintf(fp,"  type POLYSET_QUAD_MESH\n" );
    break;
  default:
    printf( "Unknown PolySetIO type\n" );
    return;
  }
  switch (pset->normType) {
  case PER_VERTEX_NORMAL:
    fprintf(fp,"  normType PER_VERTEX_NORMAL\n");
    break;
  case PER_FACE_NORMAL:
    fprintf(fp,"  normType PER_FACE_NORMAL\n");
    break;
  default:
    printf( "Unknown PolySetIO normType\n" );
    return;
  }
  switch (pset->materialBinding) {
  case PER_OBJECT_MATERIAL:
    fprintf(fp,"  materialBinding PER_OBJECT_MATERIAL\n");
    break;
  case PER_VERTEX_MATERIAL:
    fprintf(fp,"  materialBinding PER_VERTEX_MATERIAL\n");
    break;
  default:
    printf( "Unknown material binding\n" );
    return;
  }
  if (pset->hasTextureCoords) {
    fprintf(fp,"  hasTextureCoords TRUE\n");
  } else {
    fprintf(fp,"  hasTextureCoords FALSE\n");
  }
  fprintf(fp,"  rowSize %ld\n", pset->rowSize);
  fprintf(fp,"  numPolys %ld\n", pset->numPolys);

  poly = pset->poly;
  for (i = 0; i < pset->numPolys; i++, poly++) {
    fprintf(fp,"  poly {\n");
    fprintf(fp,"    numVertices %ld\n", poly->numVertices);
    vert = poly->vert;
    for (j = 0; j<poly->numVertices; j++, vert++) {
      fprintf(fp,"    pos %g %g %g\n",
	      vert->pos[0], vert->pos[1], vert->pos[2]); 
      if (pset->normType == PER_VERTEX_NORMAL) {
	fprintf(fp,"    norm %g %g %g\n",
		vert->norm[0], vert->norm[1], vert->norm[2]);
      }
      if (pset->materialBinding == PER_VERTEX_MATERIAL) {
	fprintf(fp,"    materialIndex %ld\n", vert->materialIndex);
      }
      if (pset->hasTextureCoords) {
	fprintf(fp,"    s %g  t %g\n", vert->s, vert->t);
      }
    }
    fprintf(fp,"  }\n");
  }
  fprintf(fp,"}\n");
}


static void
read_poly_setA(SceneIO *scene, FILE *fp)
{
  ObjIO *obj = append_object(&scene->objects);
  PolySetIO *pset = (PolySetIO *)calloc(1,sizeof(PolySetIO));
  char word[MAX_NAME];
  PolygonIO *poly;
  VertexIO *vert;
  int i, j;

  obj->type = POLYSET_OBJ;
  obj->data = pset;
  
  fscanf(fp, " {" );
  read_objectA(obj,fp);
  CHECK(1,fscanf(fp," type %s",word));
  if (strcmp(word,"POLYSET_TRI_MESH")==0 ) {
    pset->type = POLYSET_TRI_MESH;
  } else if (strcmp(word,"POLYSET_FACE_SET")==0 ) {
    pset->type = POLYSET_FACE_SET;
  } else if (strcmp(word,"POLYSET_QUAD_MESH")==0 ) {
    pset->type = POLYSET_QUAD_MESH;
  } else {
    printf( "Error: unknown polyset type\n" );
  }
  CHECK(1,fscanf(fp," normType %s",word));
  if (strcmp(word,"PER_VERTEX_NORMAL")==0 ) {
    pset->normType = PER_VERTEX_NORMAL;
  } else if (strcmp(word,"PER_FACE_NORMAL")==0 ) {
    pset->normType = PER_FACE_NORMAL;
  } else {
    printf( "Error: unknown polyset normType\n" );
  }
  CHECK(1,fscanf(fp," materialBinding %s",word));
  if (strcmp(word,"PER_OBJECT_MATERIAL")==0 ) {
    pset->materialBinding = PER_OBJECT_MATERIAL;
  } else if (strcmp(word,"PER_VERTEX_MATERIAL")==0 ) {
    pset->materialBinding = PER_VERTEX_MATERIAL;
  } else {
    printf( "Error: unknown material binding\n" );
  }
  CHECK(1,fscanf(fp," hasTextureCoords %s",word));
  if (strcmp(word,"TRUE")==0 ) {
    pset->hasTextureCoords = TRUE;
  } else if (strcmp(word,"FALSE")==0 ) {
    pset->hasTextureCoords = FALSE;
  } else {
    printf( "Error: unknown hasTextureCoords field\n" );
  }
  CHECK(1,fscanf(fp," rowSize %ld", &pset->rowSize));
  CHECK(1,fscanf(fp," numPolys %ld", &pset->numPolys));

  pset->poly = (PolygonIO *)calloc(pset->numPolys,sizeof(PolygonIO));
  poly = pset->poly;
  for (i = 0; i < pset->numPolys; i++, poly++) {
    fscanf(fp," poly {");
    CHECK(1,fscanf(fp," numVertices %ld", &poly->numVertices));
    poly->vert = (VertexIO *)calloc(poly->numVertices,sizeof(VertexIO));
    vert = poly->vert;
    for (j = 0; j<poly->numVertices; j++, vert++) {
      CHECK(3,fscanf(fp," pos %g %g %g",
	      &vert->pos[0], &vert->pos[1], &vert->pos[2])); 
      if (pset->normType == PER_VERTEX_NORMAL) {
	CHECK(3,fscanf(fp," norm %g %g %g",
		&vert->norm[0], &vert->norm[1], &vert->norm[2]));
      }
      if (pset->materialBinding == PER_VERTEX_MATERIAL) {
	CHECK(1,fscanf(fp," materialIndex %ld", &vert->materialIndex));
      }
      if (pset->hasTextureCoords) {
	CHECK(2,fscanf(fp," s %g t %g", &vert->s, &vert->t));
      }
    }
    fscanf(fp," }");
  }
  fscanf(fp," }");
}


static void
write_poly_setB(ObjIO *obj, FILE *fp)
{
  PolySetIO *pset = (PolySetIO *)obj->data;
  PolygonIO *poly;
  VertexIO *vert;
  int i, j;
  
  write_objectB(obj,fp);
  fwrite(&pset->type, sizeof(int), 1, fp);
  fwrite(&pset->normType, sizeof(long), 1, fp);
  fwrite(&pset->materialBinding, sizeof(int), 1, fp);
  fwrite(&pset->hasTextureCoords, sizeof(int), 1, fp);
  fwrite(&pset->rowSize, sizeof(long), 1, fp);
  fwrite(&pset->numPolys, sizeof(long), 1, fp);

  poly = pset->poly;
  for (i = 0; i < pset->numPolys; i++, poly++) {
    fwrite(&poly->numVertices, sizeof(long), 1, fp);
    vert = poly->vert;
    for (j = 0; j<poly->numVertices; j++, vert++) {
      fwrite(&vert->pos, sizeof(Point), 1, fp);
      if (pset->normType == PER_VERTEX_NORMAL) {
	fwrite(&vert->norm, sizeof(Vec), 1, fp);
      }
      if (pset->materialBinding == PER_VERTEX_MATERIAL) {
	fwrite(&vert->materialIndex, sizeof(long), 1, fp);
      }
      if (pset->hasTextureCoords) {
	fwrite(&vert->s, sizeof(Flt), 1, fp);
	fwrite(&vert->t, sizeof(Flt), 1, fp);
      }
    }
  } 
}


static void
read_poly_setB(ObjIO *obj, FILE *fp)
{
  PolySetIO *pset;
  PolygonIO *poly;
  VertexIO *vert;
  int i, j;

  pset = (PolySetIO *)calloc(1,sizeof(PolySetIO));
  obj->data = pset;

  fread(&pset->type, sizeof(int), 1, fp);
  fread(&pset->normType, sizeof(long), 1, fp);
  if (Version <= 2.0) {
    pset->materialBinding = PER_OBJECT_MATERIAL;
    pset->hasTextureCoords = FALSE;
  } else {
    fread(&pset->materialBinding, sizeof(int), 1, fp);
    fread(&pset->hasTextureCoords, sizeof(int), 1, fp);
  }
  fread(&pset->rowSize, sizeof(long), 1, fp);
  fread(&pset->numPolys, sizeof(long), 1, fp);
  pset->poly = (PolygonIO *)calloc(pset->numPolys,sizeof(PolygonIO));
  poly = pset->poly;

  for (i = 0; i < pset->numPolys; i++, poly++) {
    fread(&poly->numVertices, sizeof(long), 1, fp);
    poly->vert = (VertexIO *)calloc(poly->numVertices,sizeof(VertexIO));
    vert = poly->vert;
    for (j = 0; j<poly->numVertices; j++, vert++) {
      fread(&vert->pos, sizeof(Point), 1, fp);
      if (pset->normType == PER_VERTEX_NORMAL) {
	fread(&vert->norm, sizeof(Vec), 1, fp);
      }
      if (pset->materialBinding == PER_VERTEX_MATERIAL) {
	fread(&vert->materialIndex, sizeof(long), 1, fp);
      }
      if (pset->hasTextureCoords) {
	fread(&vert->s, sizeof(Flt), 1, fp);
	fread(&vert->t, sizeof(Flt), 1, fp);
      }
    }
  }
}


static void
delete_poly_set(PolySetIO *pset) {
	PolygonIO *poly;
	int i;
	
	poly = pset->poly;
	for (i = 0; i < pset->numPolys; i++, poly++) {
		free(poly->vert);
	}

	free(pset->poly);
	free(pset);
}

