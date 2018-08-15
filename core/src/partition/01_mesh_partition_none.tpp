#include "partition/01_mesh_partition.h"

#include <petscsys.h>

#include "mesh/mesh.h"
#include "utility/mpi_utility.h"

namespace Partition
{

template<typename T>
void MeshPartition<Mesh::None>::
extractLocalNodes(std::vector<T> &vector)
{
  // copy the entries at [beginGlobal_, beginGlobal_+localSize_] to [0,localSize_]
  for (int i = 0; i < localSize_; i++)
  {
    vector[i] = vector[i+beginGlobal_];
  }
  
  // crop the vector at localSize_
  vector.resize(localSize_);
}

}  // namespace