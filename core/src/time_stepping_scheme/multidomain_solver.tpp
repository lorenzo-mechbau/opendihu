#include "time_stepping_scheme/multidomain_solver.h"

#include <Python.h>  // has to be the first included header

#include "utility/python_utility.h"
#include "utility/petsc_utility.h"
#include "data_management/time_stepping/multidomain.h"

//#define MONODOMAIN

namespace TimeSteppingScheme
{

template<typename FiniteElementMethodPotentialFlow,typename FiniteElementMethodDiffusion>
MultidomainSolver<FiniteElementMethodPotentialFlow,FiniteElementMethodDiffusion>::
MultidomainSolver(DihuContext context) :
  TimeSteppingScheme(context["MultidomainSolver"]),
  dataMultidomain_(this->context_), finiteElementMethodPotentialFlow_(this->context_["PotentialFlow"]),
  finiteElementMethodDiffusion_(this->context_["Activation"]), finiteElementMethodDiffusionTotal_(this->context_["Activation"]),
  rankSubset_(std::make_shared<Partition::RankSubset>())
{
  // get python config
  this->specificSettings_ = this->context_.getPythonConfig();

  // initialize output writers
  this->outputWriterManager_.initialize(this->context_, this->specificSettings_);

  // parse number of motor units
  nCompartments_ = this->specificSettings_.getOptionInt("nCompartments", 1, PythonUtility::NonNegative);

  // create finiteElement objects for diffusion in compartments
  finiteElementMethodDiffusionCompartment_.reserve(nCompartments_);
  for (int k = 0; k < nCompartments_; k++)
  {
    finiteElementMethodDiffusionCompartment_.emplace_back(this->context_["Activation"]);
  }
}

template<typename FiniteElementMethodPotentialFlow,typename FiniteElementMethodDiffusion>
void MultidomainSolver<FiniteElementMethodPotentialFlow,FiniteElementMethodDiffusion>::
advanceTimeSpan()
{
  // start duration measurement, the name of the output variable can be set by "durationLogKey" in the config
  if (this->durationLogKey_ != "")
    Control::PerformanceMeasurement::start(this->durationLogKey_);

  // compute timestep width
  double timeSpan = this->endTime_ - this->startTime_;

  LOG(DEBUG) << "MultidomainSolver::advanceTimeSpan, timeSpan=" << timeSpan<< ", timeStepWidth=" << this->timeStepWidth_
    << " n steps: " << this->numberTimeSteps_;

  // loop over time steps
  double currentTime = this->startTime_;

  // loop over time steps
  for (int timeStepNo = 0; timeStepNo < this->numberTimeSteps_;)
  {
    if (timeStepNo % this->timeStepOutputInterval_ == 0 && timeStepNo > 0)
    {
      LOG(INFO) << "Multidomain diffusion, timestep " << timeStepNo << "/" << this->numberTimeSteps_<< ", t=" << currentTime
        << " (linear solver iterations: " << lastNumberOfIterations_ << ")";
    }

    // advance simulation time
    timeStepNo++;
    currentTime = this->startTime_ + double(timeStepNo) / this->numberTimeSteps_ * timeSpan;

    LOG(DEBUG) << " Vm: ";
    //dataMultidomain_.subcellularStates(0)->extractComponent(0, dataMultidomain_.transmembranePotential(0));
    LOG(DEBUG) << *dataMultidomain_.transmembranePotential(0);

    // advance diffusion
    VLOG(1) << "---- diffusion term";

    // fill nested right hand side vector with subvectors
    /*std::vector<PetscInt> indices(nCompartments_+1);
    std::iota(indices.begin(), indices.end(), 0);
    ierr = VecNestSetSubVecs(rightHandSide_, nCompartments_+1, indices.data(), subvectorsRightHandSide_.data()); CHKERRV(ierr);
*/
    // solve A*u^{t+1} = u^{t} for u^{t+1} where A is the system matrix, solveLinearSystem(b,x)
    this->solveLinearSystem();
    //ierr = VecCopy(rightHandSide_, solution_); CHKERRV(ierr);  // for debugging just copy rhs (Vm_k^{*}) to solution (Vm_k^{i+1})

    // write the solution from the nested vector back to data
    // Note, thte subvectors are actually the global vectors for component 0 of dataMultidomain_.subcellularStates(k).
    // dataMultidomain_.subcellularStates(k) is currently stored in contiguous representation. The setValues(0, subVectors[k]) copies the data to the contiguous vector.

    /*int nSubVectors = 0;
    Vec *subVectors;
    ierr = VecNestGetSubVecs(solution_, &nSubVectors, &subVectors); CHKERRV(ierr);
    assert(nSubVectors == nCompartments_+1);
*/
    //VLOG(2) << "copy phi_e";

    // get phi_e
    //dataMultidomain_.extraCellularPotential()->setValues(subVectors[nCompartments_]);

    LOG(DEBUG) << " extraCellularPotential: " << PetscUtility::getStringVector(subvectorsSolution_[nCompartments_]);
    LOG(DEBUG) << *dataMultidomain_.extraCellularPotential();

    // stop duration measurement
    if (this->durationLogKey_ != "")
      Control::PerformanceMeasurement::stop(this->durationLogKey_);

    // write current output values
    this->outputWriterManager_.writeOutput(this->dataMultidomain_, timeStepNo, currentTime);

    // start duration measurement
    if (this->durationLogKey_ != "")
      Control::PerformanceMeasurement::start(this->durationLogKey_);
  }

  // stop duration measurement
  if (this->durationLogKey_ != "")
    Control::PerformanceMeasurement::stop(this->durationLogKey_);
}

template<typename FiniteElementMethodPotentialFlow,typename FiniteElementMethodDiffusion>
void MultidomainSolver<FiniteElementMethodPotentialFlow,FiniteElementMethodDiffusion>::
run()
{
  // initialize everything
  initialize();

  this->advanceTimeSpan();
}

template<typename FiniteElementMethodPotentialFlow,typename FiniteElementMethodDiffusion>
void MultidomainSolver<FiniteElementMethodPotentialFlow,FiniteElementMethodDiffusion>::
initialize()
{
  if (this->initialized_)
    return;

  TimeSteppingScheme::initialize();

  LOG(DEBUG) << "initialize multidomain_solver, " << nCompartments_ << " compartments";
  assert(this->specificSettings_.pyObject());

  // initialize the potential flow finite element method, this also creates the function space
  finiteElementMethodPotentialFlow_.initialize();

  // initialize the data object
  dataMultidomain_.setFunctionSpace(finiteElementMethodPotentialFlow_.functionSpace());
  dataMultidomain_.initialize(nCompartments_);

  LOG(INFO) << "Run potential flow simulation for fiber directions.";

  // solve potential flow Laplace problem
  finiteElementMethodPotentialFlow_.run();

  LOG(DEBUG) << "compute gradient field";

  // compute a gradient field from the solution of the potential flow
  dataMultidomain_.flowPotential()->setValues(*finiteElementMethodPotentialFlow_.data().solution());
  dataMultidomain_.flowPotential()->computeGradientField(dataMultidomain_.fiberDirection());

  LOG(DEBUG) << "flow potential: " << *dataMultidomain_.flowPotential();
  LOG(DEBUG) << "fiber direction: " << *dataMultidomain_.fiberDirection();

  initializeCompartmentRelativeFactors();

  // initialize the finite element class, from which only the stiffness matrix is needed
  // diffusion object without prefactor, for normal diffusion (2nd multidomain eq.)
  finiteElementMethodDiffusion_.initialize(dataMultidomain_.fiberDirection(), nullptr);
  finiteElementMethodDiffusion_.initializeForImplicitTimeStepping(); // this performs extra initialization for implicit timestepping methods, i.e. it sets the inverse lumped mass matrix

  // diffusion objects with spatially varying prefactors (f_r), needed for the bottom row of the matrix eq. or the 1st multidomain eq.
  for (int k = 0; k < nCompartments_; k++)
  {
    finiteElementMethodDiffusionCompartment_[k].initialize(dataMultidomain_.fiberDirection(), dataMultidomain_.compartmentRelativeFactor(k));
    finiteElementMethodDiffusionCompartment_[k].initializeForImplicitTimeStepping(); // this performs extra initialization for implicit timestepping methods, i.e. it sets the inverse lumped mass matrix
  }

  finiteElementMethodDiffusionTotal_.initialize(dataMultidomain_.fiberDirection(), dataMultidomain_.relativeFactorTotal(), true);

  // parse parameters
  this->specificSettings_.getOptionVector("am", nCompartments_, am_);
  this->specificSettings_.getOptionVector("cm", nCompartments_, cm_);
  LOG(DEBUG) << "Am: " << am_ << ", Cm: " << cm_;

  // initialize system matrix
  setSystemMatrix(this->timeStepWidth_);

  LOG(DEBUG) << "initialize linear solver";

  // initialize linear solver
  if (linearSolver_ == nullptr)
  {
    // retrieve linear solver
    linearSolver_ = this->context_.solverManager()->template solver<Solver::Linear>(
      this->specificSettings_, this->rankSubset_->mpiCommunicator());
  }

  LOG(DEBUG) << "set system matrix to linear solver";

  // set matrix used for linear solver and preconditioner to ksp context
  assert(this->linearSolver_->ksp());
  PetscErrorCode ierr;
  ierr = KSPSetOperators(*this->linearSolver_->ksp(), systemMatrix_, systemMatrix_); CHKERRV(ierr);

  // initialize rhs and solution vector
  subvectorsRightHandSide_.resize(nCompartments_+1);
  subvectorsSolution_.resize(nCompartments_+1);

  // set values for Vm in compartments
  for (int k = 0; k < nCompartments_; k++)
  {
    subvectorsRightHandSide_[k] = dataMultidomain_.transmembranePotential(k)->valuesGlobal();
    subvectorsSolution_[k] = dataMultidomain_.transmembranePotentialSolution(k)->valuesGlobal(0);
  }

  // set values for phi_e
  subvectorsRightHandSide_[nCompartments_] = dataMultidomain_.zero()->valuesGlobal();
  subvectorsSolution_[nCompartments_] = dataMultidomain_.extraCellularPotential()->valuesGlobal();
  ierr = VecZeroEntries(subvectorsSolution_[nCompartments_]); CHKERRV(ierr);

  // create the nested vectors
  LOG(DEBUG) << "create nested vector";
  ierr = VecCreateNest(MPI_COMM_WORLD, nCompartments_+1, NULL, subvectorsRightHandSide_.data(), &rightHandSide_); CHKERRV(ierr);
  ierr = VecCreateNest(MPI_COMM_WORLD, nCompartments_+1, NULL, subvectorsSolution_.data(), &solution_); CHKERRV(ierr);

  LOG(DEBUG) << "initialization done";
  this->initialized_ = true;
}

template<typename FiniteElementMethodPotentialFlow,typename FiniteElementMethodDiffusion>
void MultidomainSolver<FiniteElementMethodPotentialFlow,FiniteElementMethodDiffusion>::
initializeCompartmentRelativeFactors()
{
  // parse relative factors f_r for compartments
  bool inputIsGlobal = this->specificSettings_.getOptionBool("inputIsGlobal", true);

  std::vector<PythonConfig> compartmentFields;
  this->specificSettings_.getOptionVector("compartmentRelativeFactors", compartmentFields);
  if (compartmentFields.size() < nCompartments_)
  {
    LOG(FATAL) << "Only " << compartmentFields.size() << " relative factors specified under \"compartmentRelativeFactors\". "
      << "Number of compartments is " << nCompartments_ << ".";
  }
  for (int k = 0; k < nCompartments_; k++)
  {
    std::vector<double> values = PythonUtility::convertFromPython<std::vector<double>>::get(compartmentFields[k].pyObject());

    // if parsed node positions in vector localNodePositions_ actually contains global node positions, extract local positions
    if (inputIsGlobal)
    {
      dataMultidomain_.functionSpace()->meshPartition()->extractLocalDofsWithoutGhosts(values);
    }

    if (values.size() < dataMultidomain_.compartmentRelativeFactor(k)->nDofsLocalWithoutGhosts())
    {
      LOG(FATAL) << "\"compartmentRelativeFactors\" for compartment " << k << " contains only " << values.size() << " entries, "
        << dataMultidomain_.compartmentRelativeFactor(k)->nDofsLocalWithoutGhosts() << " are needed.";
    }

    dataMultidomain_.compartmentRelativeFactor(k)->setValuesWithoutGhosts(values);
  }

  // compute relative Factor total as sum f_r
  dataMultidomain_.relativeFactorTotal()->zeroEntries();
  PetscErrorCode ierr;
  for (int k = 0; k < nCompartments_; k++)
  {
    ierr = VecAXPY(dataMultidomain_.relativeFactorTotal()->valuesGlobal(), 1.0, dataMultidomain_.compartmentRelativeFactor(k)->valuesGlobal()); CHKERRV(ierr);
  }


  for (int k = 0; k < nCompartments_; k++)
  {
    LOG(DEBUG) << "compartmentRelativeFactor(k=" << k << "): " << *dataMultidomain_.compartmentRelativeFactor(k);
  }
  LOG(DEBUG) << "relativeFactorTotal: " << *dataMultidomain_.relativeFactorTotal();

}

template<typename FiniteElementMethodPotentialFlow,typename FiniteElementMethodDiffusion>
void MultidomainSolver<FiniteElementMethodPotentialFlow,FiniteElementMethodDiffusion>::
setSystemMatrix(double timeStepWidth)
{
  std::vector<Mat> submatrices(MathUtility::sqr(nCompartments_+1),NULL);

  LOG(TRACE) << "setSystemMatrix";

  // fill submatrices, empty submatrices may be NULL
  // stiffnessMatrix and inverse lumped mass matrix without prefactor
  Mat stiffnessMatrix = finiteElementMethodDiffusion_.data().stiffnessMatrix()->valuesGlobal();
  Mat inverseLumpedMassMatrix = finiteElementMethodDiffusion_.data().inverseLumpedMassMatrix()->valuesGlobal();

  PetscErrorCode ierr;
  // set all submatrices
  for (int k = 0; k < nCompartments_; k++)
  {

    // right column matrix
    double prefactor = -this->timeStepWidth_ / (am_[k]*cm_[k]);

    VLOG(2) << "k=" << k << ", am: " << am_[k] << ", cm: " << cm_[k] << ", prefactor: " << prefactor;

    // create matrix as M^{-1}*K
    Mat matrixOnRightColumn;
    ierr = MatMatMult(inverseLumpedMassMatrix, stiffnessMatrix, MAT_INITIAL_MATRIX, PETSC_DEFAULT, &matrixOnRightColumn);

    // scale matrix on right column with prefactor
    ierr = MatScale(matrixOnRightColumn, prefactor); CHKERRV(ierr);

    // copy right block matrix also to diagonal matrix
    Mat matrixOnDiagonalBlock;
    ierr = MatConvert(matrixOnRightColumn, MATSAME, MAT_INITIAL_MATRIX, &matrixOnDiagonalBlock); CHKERRV(ierr);

    // for debugging zero all entries
#ifdef MONODOMAIN
/**/    //ierr = MatZeroEntries(matrixOnRightColumn); CHKERRV(ierr);
#endif

    // set on right column of the system matrix
    submatrices[k*(nCompartments_+1) + (nCompartments_+1) - 1] = matrixOnRightColumn;

    if (VLOG_IS_ON(2))
    {
      VLOG(2) << "matrixOnRightColumn: " << PetscUtility::getStringMatrix(matrixOnRightColumn);
      VLOG(2) << "set at index " << k*(nCompartments_+1) + (nCompartments_+1) - 1;
    }

    // ---
    // diagonal matrix
    // add identity
    ierr = MatShift(matrixOnDiagonalBlock, 1); CHKERRV(ierr);

    // set on diagonal
    submatrices[k*(nCompartments_+1) + k] = matrixOnDiagonalBlock;

    if (VLOG_IS_ON(2))
    {
      VLOG(2) << "matrixOnDiagonalBlock:" << PetscUtility::getStringMatrix(matrixOnDiagonalBlock);
      VLOG(2) << "set at index " << k*(nCompartments_+1) + k;
    }

    // ---
    // bottom row matrices
    // create matrix as copy of stiffnessMatrix
    Mat stiffnessMatrixWithPrefactor = finiteElementMethodDiffusionCompartment_[k].data().stiffnessMatrix()->valuesGlobal();

    Mat matrixOnBottomRow;
    ierr = MatConvert(stiffnessMatrixWithPrefactor, MATSAME, MAT_INITIAL_MATRIX, &matrixOnBottomRow); CHKERRV(ierr);

#if 0
    // debugging test, gives slightly different results due to approximation of test
    Mat test;
    ierr = MatConvert(stiffnessMatrix, MATSAME, MAT_INITIAL_MATRIX, &test); CHKERRV(ierr);

    // get the relative factor of the compartment
    Vec compartmentRelativeFactor = dataMultidomain_.compartmentRelativeFactor(k)->valuesGlobal();

    int i = 0;double v = 0;
    MatGetValues(test,1,&i,1,&i,&v);
    LOG(DEBUG) << "compartmentRelativeFactor: " << PetscUtility::getStringVector(compartmentRelativeFactor);
    LOG(DEBUG) << "test[0][0] = " << v;

    // MatDiagonalScale(A,l,NULL) computes A = diag(l)*A, this scales the rows of A with the values in l (each row with one entry of l)
    ierr = MatDiagonalScale(test, compartmentRelativeFactor, NULL); CHKERRV(ierr);

    MatGetValues(test,1,&i,1,&i,&v);
    LOG(DEBUG) << "MatdiagonalScale";
    LOG(DEBUG) << "test[0][0] = " << v;

    // test if "test" and matrixOnBottomRow yield the same matrices
    PetscInt nRows, nColumns;
    ierr = MatGetSize(matrixOnBottomRow,&nRows,&nColumns); CHKERRV(ierr);
    double max_diff = 0;
    for (int column = 0; column < nColumns; column++)
    {
      for (int row = 0; row < nRows; row++)
      {
        double value1, value2;
        ierr = MatGetValues(matrixOnBottomRow,1,&row,1,&column,&value1); CHKERRV(ierr);
        ierr = MatGetValues(test,1,&row,1,&column,&value2); CHKERRV(ierr);
        double difference = fabs(value1-value2);
        max_diff = std::max(max_diff, difference);
        if (difference > 1e-5)
        {
          LOG(DEBUG) << "bottom matrix entry row=" << row << ", column=" << column << " is different: " << value1 << "," << value2 << ", diff: " << difference;

        }
      }
    }
    LOG(FATAL) << "max_diff:" << max_diff;
#endif
    if (VLOG_IS_ON(2))
    {
      VLOG(2) << "matrixOnBottomRow: " << PetscUtility::getStringMatrix(matrixOnBottomRow);
    }

    // for debugging zero all entries
#ifdef MONODOMAIN
/**/    ierr = MatZeroEntries(matrixOnBottomRow); CHKERRV(ierr);
#endif

    // set on bottom row of the system matrix
    submatrices[((nCompartments_+1) - 1)*(nCompartments_+1) + k] = matrixOnBottomRow;

    VLOG(2) << "set at index " << ((nCompartments_+1) - 1)*(nCompartments_+1) + k;
  }

  // set bottom right matrix
  Mat stiffnessMatrixBottomRight = finiteElementMethodDiffusionTotal_.data().stiffnessMatrix()->valuesGlobal();

  if (VLOG_IS_ON(2))
  {
    VLOG(2) << "stiffnessMatrixBottomRight:" << PetscUtility::getStringMatrix(stiffnessMatrixBottomRight);
  }

  // for debugging set to identity
#ifdef MONODOMAIN
/**/  ierr = MatZeroEntries(stiffnessMatrixBottomRight); CHKERRV(ierr);
/**/  ierr = MatShift(stiffnessMatrixBottomRight,1); CHKERRV(ierr);
#endif

  // set on bottom right
  submatrices[(nCompartments_+1)*(nCompartments_+1)-1] = stiffnessMatrixBottomRight;
  VLOG(2) << "set at index " << (nCompartments_+1)*(nCompartments_+1)-1;

  // create nested matrix
  ierr = MatCreateNest(this->dataMultidomain_.functionSpace()->meshPartition()->mpiCommunicator(),
                       nCompartments_+1, NULL, nCompartments_+1, NULL, submatrices.data(), &this->systemMatrix_); CHKERRV(ierr);
}

template<typename FiniteElementMethodPotentialFlow,typename FiniteElementMethodDiffusion>
void MultidomainSolver<FiniteElementMethodPotentialFlow,FiniteElementMethodDiffusion>::
solveLinearSystem()
{
  PetscErrorCode ierr;


  VLOG(1) << "in solveLinearSystem";

  // configure that the initial value for the iterative solver is the value in solution, not zero
  ierr = KSPSetInitialGuessNonzero(*this->linearSolver_->ksp(), PETSC_TRUE); CHKERRV(ierr);

  // solve the system, KSPSolve(ksp,b,x)
  ierr = KSPSolve(*this->linearSolver_->ksp(), rightHandSide_, solution_); CHKERRV(ierr);

  int numberOfIterations = 0;
  PetscReal residualNorm = 0.0;
  ierr = KSPGetIterationNumber(*this->linearSolver_->ksp(), &numberOfIterations); CHKERRV(ierr);
  ierr = KSPGetResidualNorm(*this->linearSolver_->ksp(), &residualNorm); CHKERRV(ierr);

  KSPConvergedReason convergedReason;
  ierr = KSPGetConvergedReason(*this->linearSolver_->ksp(), &convergedReason); CHKERRV(ierr);

  lastNumberOfIterations_ = numberOfIterations;

  LOG(DEBUG) << "Linear system of multidomain problem solved in " << numberOfIterations << " iterations, residual norm " << residualNorm
    << ": " << PetscUtility::getStringLinearConvergedReason(convergedReason);
}

//! return whether the underlying discretizableInTime object has a specified mesh type and is not independent of the mesh type
template<typename FiniteElementMethodPotentialFlow,typename FiniteElementMethodDiffusion>
bool MultidomainSolver<FiniteElementMethodPotentialFlow,FiniteElementMethodDiffusion>::
knowsMeshType()
{
  return true;
}

template<typename FiniteElementMethodPotentialFlow,typename FiniteElementMethodDiffusion>
typename MultidomainSolver<FiniteElementMethodPotentialFlow,FiniteElementMethodDiffusion>::Data &MultidomainSolver<FiniteElementMethodPotentialFlow,FiniteElementMethodDiffusion>::
data()
{
  return dataMultidomain_;
}

//! get the data that will be transferred in the operator splitting to the other term of the splitting
//! the transfer is done by the solution_vector_mapping class
template<typename FiniteElementMethodPotentialFlow,typename FiniteElementMethodDiffusion>
typename MultidomainSolver<FiniteElementMethodPotentialFlow,FiniteElementMethodDiffusion>::TransferableSolutionDataType
MultidomainSolver<FiniteElementMethodPotentialFlow,FiniteElementMethodDiffusion>::
getSolutionForTransferInOperatorSplitting()
{
  LOG(DEBUG) << "getSolutionForTransferInOperatorSplitting, size of Vm vector: " << this->dataMultidomain_.transmembranePotential().size();

  return std::pair<std::vector<Vec>,std::vector<std::shared_ptr<FieldVariableType>>>(
    this->subvectorsSolution_, this->dataMultidomain_.transmembranePotential());
}

} // namespace TimeSteppingScheme
