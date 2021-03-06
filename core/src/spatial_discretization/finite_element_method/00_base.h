#pragma once

#include "data_management/finite_element_method/finite_elements.h"
#include "partition/rank_subset.h"

//#define QUADRATURE_TEST    ///< if evaluation of quadrature accuracy takes place
//#define EXACT_QUADRATURE Quadrature::Gauss<20>

namespace SpatialDiscretization
{

/**
 * Base class containing basic finite element functionality such as initializing and solving.
 * Further classes derive from this base class and add special functionality such as setting stiffness matrix, rhs and timestepping
 */
template<typename FunctionSpaceType,typename QuadratureType,typename Term>
class FiniteElementMethodBase : public SpatialDiscretization, public Runnable
{
public:
  //! constructor, if function space is not given, create new one according to settings
  //! if the function space is given as parameter, is has to be already initialize()d
  FiniteElementMethodBase(DihuContext context, std::shared_ptr<FunctionSpaceType> functionSpace = nullptr);

  typedef ::Data::FiniteElements<FunctionSpaceType,Term> Data;
  typedef FunctionSpaceType FunctionSpace;
  typedef typename Data::TransferableSolutionDataType TransferableSolutionDataType;

  // perform computation
  void run();

  //! initialize for use as laplace or poisson equation, not for timestepping
  virtual void initialize();

  //! reset to pre-initialized state, this deallocates all data and sets initialized_ to false such that a new call to initialize() is necessary
  void reset();

  //! set the subset of ranks that will compute the work
  void setRankSubset(Partition::RankSubset rankSubset);
  
  //! get the stored functionSpace
  std::shared_ptr<FunctionSpaceType> functionSpace();

  //! get the data object
  Data &data();

  friend class StiffnessMatrixTester;    ///< a class used for testing
protected:

  //! read in rhs values from config and creates a FE rhs vector out of it
  virtual void setRightHandSide() = 0;

  //! setup stiffness matrix
  virtual void setStiffnessMatrix() = 0;
  
  //! setup mass matrix
  virtual void setMassMatrix() = 0;

  //! solve finite element linear system
  virtual void solve();

  //! after rhs is transferred to weak form this method is called and can be overriden later
  virtual void manipulateWeakRhs(){}

  //! modify the rhs to incorporate dirichlet boundary conditions
  virtual void applyBoundaryConditions() = 0;

  DihuContext context_;    ///< object that contains the python config for the current context and the global singletons meshManager and solverManager
  Data data_;     ///< data object that holds all PETSc vectors and matrices
  PythonConfig specificSettings_;    ///< python object containing the value of the python config dict with corresponding key
  OutputWriter::Manager outputWriterManager_; ///< manager object holding all output writer
  bool initialized_;     ///< if initialize was already called on this object, then further calls to initialize() have no effect
};

/** class that provides extra initialize methods, depending on Term
 */
template<typename FunctionSpaceType,typename QuadratureType,typename Term>
class FiniteElementMethodInitializeData :
  public FiniteElementMethodBase<FunctionSpaceType,QuadratureType,Term>
{
public:
  //! use constructor of base class
  using FiniteElementMethodBase<FunctionSpaceType,QuadratureType,Term>::FiniteElementMethodBase;
};

/** special initialize for DiffusionTensorDirectional, for Term Equation::Dynamic::DirectionalDiffusion
 */
template<typename FunctionSpaceType,typename QuadratureType>
class FiniteElementMethodInitializeData<FunctionSpaceType,QuadratureType,Equation::Dynamic::DirectionalDiffusion> :
  public FiniteElementMethodBase<FunctionSpaceType,QuadratureType,Equation::Dynamic::DirectionalDiffusion>
{
public:
  //! use constructor of base class
  using FiniteElementMethodBase<FunctionSpaceType,QuadratureType,Equation::Dynamic::DirectionalDiffusion>::FiniteElementMethodBase;

  //! initialize with direction field for DiffusionTensorDirectional, this replaces the initialize() method
  virtual void initialize(std::shared_ptr<FieldVariable::FieldVariable<FunctionSpaceType,3>> direction,
                          std::shared_ptr<FieldVariable::FieldVariable<FunctionSpaceType,1>> spatiallyVaryingPrefactor,
                          bool useAdditionalDiffusionTensor = false);
};


} // namespace

#include "spatial_discretization/finite_element_method/00_base.tpp"
