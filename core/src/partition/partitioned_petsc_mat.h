#pragma once

#include <memory>

#include "control/types.h"
#include "partition/rank_subset.h"
#include "partition/partitioned_petsc_mat_base.h"
#include "partition/01_mesh_partition.h"

/** Global numbering: such that each rank has its own contiguous subset of the total range.
 *  Local numbering: ghost elements
 */
template<typename BasisOnMeshType, typename = typename BasisOnMeshType::Mesh>
class PartitionedPetscMat
{
};

/** partial specialization for structured meshes */
template<typename MeshType, typename BasisFunctionType>
class PartitionedPetscMat<BasisOnMesh::BasisOnMesh<MeshType,BasisFunctionType>,Mesh::isStructured<MeshType>> : 
  public PartitionedPetscMatBase
{
public:
  //! constructor
  PartitionedPetscMat(std::shared_ptr<Partition::MeshPartition<BasisOnMesh::BasisOnMesh<MeshType,BasisFunctionType>>> meshPartition, 
                      int nComponents, int diagonalNonZeros, int offdiagonalNonZeros);
 
  //! constructor to simply wrap an existing Mat, as needed in nonlinear solver callback functions for jacobians
  PartitionedPetscMat(Mat &matrix);
 
protected:
  
  //! create a distributed Petsc matrix, according to the given partition
  void createMatrix(int diagonalNonZeros, int offdiagonalNonZeros);
  
  std::shared_ptr<Partition::MeshPartition<BasisOnMesh::BasisOnMesh<MeshType,BasisFunctionType>>> meshPartition_;  ///< the mesh partition object which stores how the mesh is decomposed and what is the local portion
  int nComponents_;  ///< number of components of the field variable
  DM dm_;    ///< PETSc DMDA object (data management distributed array) that stores topology information and everything needed for communication of ghost values, knows the number of components
};

/** partial specialization for unstructured meshes 
 *  use petsc IS
 *  not implemented yet
 */
template<int D, typename BasisFunctionType>
class PartitionedPetscMat<BasisOnMesh::BasisOnMesh<Mesh::UnstructuredDeformableOfDimension<D>, BasisFunctionType>, Mesh::UnstructuredDeformableOfDimension<D>> : 
  public PartitionedPetscMatBase
{
public:
  //! constructor
  PartitionedPetscMat(std::shared_ptr<Partition::MeshPartition<BasisOnMesh::BasisOnMesh<Mesh::UnstructuredDeformableOfDimension<D>,BasisFunctionType>>> meshPartition, 
                      int nComponents, int diagonalNonZeros, int offdiagonalNonZeros);
 
  //! constructor to simply wrap an existing Mat, as needed in nonlinear solver callback functions for jacobians
  PartitionedPetscMat(Mat &matrix);
 
protected:
  
  //! create a distributed Petsc matrix, according to the given partition
  void createMatrix(int diagonalNonZeros, int offdiagonalNonZeros);
  
  std::shared_ptr<Partition::MeshPartition<BasisOnMesh::BasisOnMesh<Mesh::UnstructuredDeformableOfDimension<D>, BasisFunctionType>>> meshPartition_;  ///< the mesh partition object which stores how the mesh is decomposed and what is the local portion
  int nComponents_;  ///< number of components of the field variable
  
};

#include "partition/partitioned_petsc_mat_structured.tpp"
#include "partition/partitioned_petsc_mat_unstructured.tpp"