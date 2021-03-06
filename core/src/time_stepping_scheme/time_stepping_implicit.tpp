#include "time_stepping_scheme/time_stepping_implicit.h"

#include <Python.h>  // has to be the first included header

#include "utility/python_utility.h"
#include "utility/petsc_utility.h"
#include <petscksp.h>
#include "solver/solver_manager.h"
#include "solver/linear.h"
#include "data_management/time_stepping/time_stepping_implicit.h"

namespace TimeSteppingScheme
{

template<typename DiscretizableInTimeType>
TimeSteppingImplicit<DiscretizableInTimeType>::TimeSteppingImplicit(DihuContext context, std::string name) :
TimeSteppingSchemeOde<DiscretizableInTimeType>(context, name)
{
  this->data_ = std::make_shared<Data::TimeSteppingImplicit<typename DiscretizableInTimeType::FunctionSpace, DiscretizableInTimeType::nComponents()>>(context); // create data object for implicit euler
  this->dataImplicit_ = std::static_pointer_cast<Data::TimeSteppingImplicit<typename DiscretizableInTimeType::FunctionSpace, DiscretizableInTimeType::nComponents()>>(this->data_);
}

template<typename DiscretizableInTimeType>
void TimeSteppingImplicit<DiscretizableInTimeType>::
initialize()
{
  if (this->initialized_)
    return;
  
  TimeSteppingSchemeOde<DiscretizableInTimeType>::initialize();
  LOG(TRACE) << "TimeSteppingImplicit::initialize";

  // compute the system matrix
  this->setSystemMatrix(this->timeStepWidth_);

  // set the boundary conditions to system matrix, i.e. zero rows and columns of Dirichlet BC dofs and set diagonal to 1
  this->dirichletBoundaryConditions_->applyInSystemMatrix(this->dataImplicit_->systemMatrix(), this->dataImplicit_->boundaryConditionsRightHandSideSummand());

  // initialize the linear solver that is used for solving the implicit system
  initializeLinearSolver();
  
  // set matrix used for linear system and preconditioner to ksp context
  Mat &systemMatrix = this->dataImplicit_->systemMatrix()->valuesGlobal();
  assert(this->ksp_);
  PetscErrorCode ierr;
  ierr = KSPSetOperators(*ksp_, systemMatrix, systemMatrix); CHKERRV(ierr);
  
  this->initialized_ = true;
}

template<typename DiscretizableInTimeType>
void TimeSteppingImplicit<DiscretizableInTimeType>::
solveLinearSystem(Vec &input, Vec &output)
{
  // solve systemMatrix*output = input for output
  Mat &systemMatrix = this->dataImplicit_->systemMatrix()->valuesGlobal();
  
  PetscErrorCode ierr;
  PetscUtility::checkDimensionsMatrixVector(systemMatrix, input);
  
  // solve the system, KSPSolve(ksp,b,x)
  ierr = KSPSolve(*ksp_, input, output); CHKERRV(ierr);
  
  int numberOfIterations = 0;
  PetscReal residualNorm = 0.0;
  ierr = KSPGetIterationNumber(*ksp_, &numberOfIterations); CHKERRV(ierr);
  ierr = KSPGetResidualNorm(*ksp_, &residualNorm); CHKERRV(ierr);
  
  KSPConvergedReason convergedReason;
  ierr = KSPGetConvergedReason(*ksp_, &convergedReason); CHKERRV(ierr);
  
  VLOG(1) << "Linear system of implicit time stepping solved in " << numberOfIterations << " iterations, residual norm " << residualNorm
    << ": " << PetscUtility::getStringLinearConvergedReason(convergedReason);
}

template<typename DiscretizableInTimeType>
void TimeSteppingImplicit<DiscretizableInTimeType>::
initializeLinearSolver()
{ 
  if (linearSolver_ == nullptr)
  {
    LOG(DEBUG) << "Implicit time stepping: initialize linearSolver";
    
    // retrieve linear solver
    linearSolver_ = this->context_.solverManager()->template solver<Solver::Linear>(
      this->specificSettings_, this->data_->functionSpace()->meshPartition()->mpiCommunicator());
    ksp_ = linearSolver_->ksp();
  }
  else 
  {
    VLOG(2) << ": linearSolver_ already set";
  }
}



} // namespace TimeSteppingScheme
