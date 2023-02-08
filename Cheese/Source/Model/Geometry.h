#ifndef MODEL_GEOMETRY_H
#define MODEL_GEOMETRY_H
#include "Mesh.h"

class Geometry
{
 public:
  static IMesh* GenerateBox(uint32 width, uint32 height, uint32 depth);
  static IMesh* GeneratePlane(float width, float depth, uint32 m = 2, uint32 n = 2);
};
#endif  //  MODEL_GEOMETRY_H
