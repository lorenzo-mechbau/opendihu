#include "basis_on_mesh/basis_on_mesh.h"

#include <cmath>
#include <array>
#include <sstream>

#include "easylogging++.h"

namespace BasisOnMesh
{

template<typename MeshType, typename BasisFunctionType>
std::array<dof_no_t,BasisOnMeshFunction<MeshType,BasisFunctionType>::nDofsPerElement()> BasisOnMesh<MeshType,BasisFunctionType>::
getElementDofNos(element_no_t elementNo) const
{
  std::array<dof_no_t,BasisOnMeshFunction<MeshType,BasisFunctionType>::nDofsPerElement()> dof;
  for (int dofIndex = 0; dofIndex < BasisOnMeshFunction<MeshType,BasisFunctionType>::nDofsPerElement(); dofIndex++)
  {
    dof[dofIndex] = this->getDofNo(elementNo, dofIndex);
  }
  return dof;
}

template<typename MeshType, typename BasisFunctionType>
void BasisOnMesh<MeshType,BasisFunctionType>::
getElementDofNos(element_no_t elementNo, std::vector<dof_no_t> &globalDofNos) const
{
  globalDofNos.resize(BasisOnMeshFunction<MeshType,BasisFunctionType>::nDofsPerElement());
  for (int dofIndex = 0; dofIndex < BasisOnMeshFunction<MeshType,BasisFunctionType>::nDofsPerElement(); dofIndex++)
  {
    globalDofNos[dofIndex] = this->getDofNo(elementNo, dofIndex);
  }
}


};  // namespace