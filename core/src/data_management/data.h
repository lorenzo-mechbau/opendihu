#pragma once

#include <Python.h>  // has to be the first included header
#include <petscmat.h>
#include <memory>
#include <vector>

#include "control/types.h"
#include "mesh/mesh.h"
#include "field_variable/field_variable.h"
#include "control/dihu_context.h"
#include "partition/rank_subset.h"

namespace Data
{

template<typename FunctionSpaceType>
class Data
{
public:

  typedef FunctionSpaceType FunctionSpace;

  //! constructor
  Data(DihuContext context);

  //! destructor
  virtual ~Data();

  //! initialize the mesh with e.g. number of dimensions
  virtual void setFunctionSpace(std::shared_ptr<FunctionSpaceType> fuctionSpace);

  //! initialize, generate petsc objects, this has to be called after setFunctionSpace
  virtual void initialize();

  //! initialize, generate petsc objects, this has to be called after setFunctionSpace
  virtual void reset();

  //! set the subset of ranks that will compute the work
  virtual void setRankSubset(Partition::RankSubset rankSubset);
  
  //! get the stored functionSpace
  const std::shared_ptr<FunctionSpaceType> functionSpace() const;

  //! return the total number of unknowns in the local partition, i.e. degrees of freedom x number of components, this can be a multiple of the number of nodes of the mesh
  virtual dof_no_t nUnknownsLocalWithGhosts();

  //! return the total number of unknowns in the local partition, i.e. degrees of freedom x number of components, this can be a multiple of the number of nodes of the mesh
  virtual dof_no_t nUnknownsLocalWithoutGhosts();

  //! return the total number of unknowns in the global domain, i.e. degrees of freedom x number of components, this can be a multiple of the number of nodes of the mesh
  virtual global_no_t nUnknownsGlobal();

protected:

  //! initializes the vectors and stiffness matrix with size
  virtual void createPetscObjects() = 0;

  const DihuContext context_;     ///< the context object with python config of the class that uses this data object
  std::shared_ptr<FunctionSpaceType> functionSpace_; ///< the mesh/function space on which the data in this object is defined

  std::shared_ptr<Partition::RankSubset> rankSubset_;  ///< a subset of MPI ranks that will operate on the data of this object
  
  bool initialized_ = false;
};

} // namespace

#include "data_management/data.tpp"
