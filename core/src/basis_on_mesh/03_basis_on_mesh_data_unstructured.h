#pragma once

#include <Python.h>  // has to be the first included header

#include <array>
#include "control/types.h"

#include "basis_on_mesh/02_basis_on_mesh_jacobian.h"
#include "mesh/type_traits.h"
#include "field_variable/unstructured/element_to_node_mapping.h"
#include "field_variable/00_field_variable_base.h"

// forward declaration of FieldVariable
namespace FieldVariable
{
template<typename BasisOnMeshType,int nComponents>
class FieldVariable;

template<typename BasisOnMeshType>
class FieldVariableBase;

template<typename BasisOnMeshType>
class Component;
};

namespace BasisOnMesh
{
// forward declaration of BasisOnMesh, needed for FieldVariable
template<typename MeshType,typename BasisFunctionType>
class BasisOnMesh;

/** Class that stores adjacency information for unstructured meshes and does file i/o for construction
 */
template<int D,typename BasisFunctionType>
class BasisOnMeshDataUnstructured :
  public BasisOnMeshJacobian<Mesh::UnstructuredDeformableOfDimension<D>,BasisFunctionType>
{
public:

  typedef ::FieldVariable::FieldVariableBase<BasisOnMesh<Mesh::UnstructuredDeformableOfDimension<D>,BasisFunctionType>> FieldVariableBaseType;
  typedef BasisOnMesh<Mesh::UnstructuredDeformableOfDimension<D>,BasisFunctionType> BasisOnMeshType;

  //! constructor, it is possible to create a basisOnMesh object without geometry field, e.g. for the lower order mesh of a mixed formulation
  BasisOnMeshDataUnstructured(PyObject *settings, bool noGeometryField=false);

  //! return the global dof number of element-local dof dofIndex of element elementNo, nElements is the total number of elements
  dof_no_t getDofNo(element_no_t elementNo, int dofIndex) const;

  //! return the global node number of element-local node nodeIndex of element elementNo, nElements is the total number of elements
  node_no_t getNodeNo(element_no_t elementNo, int nodeIndex) const;

  //! get all dofs of a specific node, as vector, the array version that is present for structured meshes does not make sense here, because with versions the number of dofs per node is not static.
  void getNodeDofs(node_no_t nodeGlobalNo, std::vector<dof_no_t> &dofGlobalNos) const;

  //! write exelem file representation to stream
  void outputExelemFile(std::ostream &file);

  //! write exnode file representation to stream
  void outputExnodeFile(std::ostream &file);

  //! get the total number of elements, for structured meshes this is directly implemented in the Mesh itself (not BasisOnMesh like here)
  element_no_t nElements() const;

protected:
  //! parse a given *.exelem file and prepare fieldVariable_
  void parseExelemFile(std::string exelemFilename);

  //! parse a given *.exelem file and fill fieldVariable_ with values
  void parseExnodeFile(std::string exnodeFilename);

  //! rename field variables if "remap" is specified in config
  void remapFieldVariables(PyObject *settings);

  //! multiply dof values with scale factors such that scale factor information is completely contained in dof values
  void eliminateScaleFactors();

  //! parse the element and node positions from python settings
  void parseFromSettings(PyObject *settings);

  std::map<std::string, std::shared_ptr<FieldVariableBaseType>> fieldVariable_; ///< all non-geometry field field variables that were present in exelem/exnode files
  std::shared_ptr<FieldVariable::FieldVariable<BasisOnMeshType,3>> geometryField_ = nullptr;  ///< the geometry field variable

  std::shared_ptr<FieldVariable::ElementToNodeMapping> elementToNodeMapping_;   ///< for every element the adjacent nodes and the field variable + dofs for their position
  element_no_t nElements_ = 0;    ///< number of elements in exelem file
  dof_no_t nDofs_ = 0;        ///< number of degrees of freedom. This can be different from nNodes * nDofsPerNode because of versions and shared nodes
  bool noGeometryField_;     ///< this is set if there is no geometry field stored. this is only needed for solid mechanics mixed formulation where the lower order basisOnMesh does not need its own geometry information

};

}  // namespace

#include "basis_on_mesh/03_basis_on_mesh_data_unstructured.tpp"
#include "basis_on_mesh/03_basis_on_mesh_data_unstructured_exfile_io.tpp"
