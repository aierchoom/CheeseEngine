#ifndef MODEL_GEOMETRY_H
#define MODEL_GEOMETRY_H
#include "Mesh.h"

class Geometry
{
 public:
  static IMesh* GenerateBox(uint32 width, uint32 height, uint32 depth);
};
#endif  //  MODEL_GEOMETRY_H
