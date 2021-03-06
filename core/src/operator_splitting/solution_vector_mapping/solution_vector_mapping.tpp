#include "operator_splitting/solution_vector_mapping/solution_vector_mapping.h"

#include <vector>
#include <tuple>
#include "easylogging++.h"

/** Transfer between two field variables with given component number
 */
template<typename FunctionSpaceType1, int nComponents1, typename FunctionSpaceType2, int nComponents2>
void SolutionVectorMapping<
  std::tuple<std::shared_ptr<FieldVariable::FieldVariable<FunctionSpaceType1,nComponents1>>, int, double>,   // <fieldVariableType,componentNo,prefactor>
  std::tuple<std::shared_ptr<FieldVariable::FieldVariable<FunctionSpaceType2,nComponents2>>, int, double>
>::transfer(const std::tuple<std::shared_ptr<FieldVariable::FieldVariable<FunctionSpaceType1,nComponents1>>, int, double> &transferableSolutionData1,
            const std::tuple<std::shared_ptr<FieldVariable::FieldVariable<FunctionSpaceType2,nComponents2>>, int, double> &transferableSolutionData2)
{
  // rename input data
  std::shared_ptr<FieldVariable::FieldVariable<FunctionSpaceType1,nComponents1>> fieldVariable1 = std::get<0>(transferableSolutionData1);
  std::shared_ptr<FieldVariable::FieldVariable<FunctionSpaceType2,nComponents2>> fieldVariable2 = std::get<0>(transferableSolutionData2);

  // disable checking for nans and infs because it take a lot of time
  //fieldVariable1->checkNansInfs();

  int componentNo1 = std::get<1>(transferableSolutionData1);
  int componentNo2 = std::get<1>(transferableSolutionData2);

  double prefactor1 = std::get<2>(transferableSolutionData1);
  double prefactor2 = std::get<2>(transferableSolutionData2);

  VLOG(1) << "solution vector mapping, transfer from component "
    << componentNo1 << " (" << fieldVariable1->nDofsLocalWithoutGhosts() << " dofs), prefactor " << prefactor1
    << " to " << componentNo2 << " (" << fieldVariable2->nDofsLocalWithoutGhosts() << " dofs), prefactor " << prefactor2 << " (not considered here)";

  assert(fieldVariable1->nDofsLocalWithoutGhosts() == fieldVariable2->nDofsLocalWithoutGhosts());
  assert(fieldVariable1->nDofsGlobal() == fieldVariable2->nDofsGlobal());

  // if representation of fieldVariable1 is invalid, this means that it has been extracted to another field variable
  if (fieldVariable2->partitionedPetscVec()->currentRepresentation() == Partition::values_representation_t::representationInvalid)
  {
    LOG(DEBUG) << "SolutionVectorMapping restoreExtractedComponent (1)";
    // tranfer from finite elements back to cellml
    fieldVariable2->restoreExtractedComponent(fieldVariable1->partitionedPetscVec());
  }
  else
  {
    LOG(DEBUG) << "SolutionVectorMapping copy (1)";
    PetscErrorCode ierr;
    ierr = VecCopy(fieldVariable1->valuesGlobal(componentNo1), fieldVariable2->valuesGlobal(componentNo2)); CHKERRV(ierr);
  }

  // scale result with prefactor1
  if (prefactor1 != 1.0)
  {
    PetscErrorCode ierr;
    ierr = VecScale(fieldVariable2->valuesGlobal(componentNo2), prefactor1); CHKERRV(ierr);
  }
}

/** Transfer between two field variables, the first is scalar, the second is vector-values, store to given component number
 */
template<typename FunctionSpaceType1, typename FunctionSpaceType2, int nComponents2>
void SolutionVectorMapping<
  std::tuple<std::shared_ptr<FieldVariable::FieldVariable<FunctionSpaceType1,1>>, int, double>,   // <fieldVariableType,componentNo,prefactor>
  std::tuple<std::shared_ptr<FieldVariable::FieldVariable<FunctionSpaceType2,nComponents2>>, int, double>
>::transfer(const std::tuple<std::shared_ptr<FieldVariable::FieldVariable<FunctionSpaceType1,1>>, int, double> &transferableSolutionData1,
            const std::tuple<std::shared_ptr<FieldVariable::FieldVariable<FunctionSpaceType2,nComponents2>>, int, double> &transferableSolutionData2)
{
  // rename input data
  std::shared_ptr<FieldVariable::FieldVariable<FunctionSpaceType1,1>> fieldVariable1 = std::get<0>(transferableSolutionData1);
  std::shared_ptr<FieldVariable::FieldVariable<FunctionSpaceType2,nComponents2>> fieldVariable2 = std::get<0>(transferableSolutionData2);

  // disable checking for nans and infs because it take a lot of time
  //fieldVariable1->checkNansInfs();

  int componentNo1 = 0;
  int componentNo2 = std::get<1>(transferableSolutionData2);

  double prefactor1 = std::get<2>(transferableSolutionData1);
  double prefactor2 = std::get<2>(transferableSolutionData2);

  VLOG(1) << "solution vector mapping, transfer from component "
    << componentNo1 << " (" << fieldVariable1->nDofsLocalWithoutGhosts() << " dofs), prefactor " << prefactor1
    << " to " << componentNo2 << " (" << fieldVariable2->nDofsLocalWithoutGhosts() << " dofs), prefactor " << prefactor2 << " (not considered here)";

  assert(fieldVariable1->nDofsLocalWithoutGhosts() == fieldVariable2->nDofsLocalWithoutGhosts());
  assert(fieldVariable1->nDofsGlobal() == fieldVariable2->nDofsGlobal());

  // if representation of fieldVariable1 is invalid, this means that it has been extracted to another field variable
  if (fieldVariable2->partitionedPetscVec()->currentRepresentation() == Partition::values_representation_t::representationInvalid)
  {
    LOG(DEBUG) << "SolutionVectorMapping restoreExtractedComponent (2)";
    // tranfer from finite elements back to cellml
    fieldVariable2->restoreExtractedComponent(fieldVariable1->partitionedPetscVec());
  }
  else
  {
    LOG(DEBUG) << "SolutionVectorMapping copy (2) "
      << Partition::valuesRepresentationString[fieldVariable1->partitionedPetscVec()->currentRepresentation()]
      << " -> "
      << Partition::valuesRepresentationString[fieldVariable2->partitionedPetscVec()->currentRepresentation()]
      << "";

    PetscErrorCode ierr;
    ierr = VecCopy(fieldVariable1->valuesGlobal(componentNo1), fieldVariable2->valuesGlobal(componentNo2)); CHKERRV(ierr);
  }

  // scale result with prefactor1
  if (prefactor1 != 1.0)
  {
    PetscErrorCode ierr;
    ierr = VecScale(fieldVariable2->valuesGlobal(componentNo2), prefactor1); CHKERRV(ierr);
  }
}

/** Transfer between two field variables, the first is vector-valued, use given component number, store in second, which is scalar
 */
template<typename FunctionSpaceType1, int nComponents1, typename FunctionSpaceType2>
void SolutionVectorMapping<
  std::tuple<std::shared_ptr<FieldVariable::FieldVariable<FunctionSpaceType1,nComponents1>>, int, double>,   // <fieldVariableType,componentNo,prefactor>
  std::tuple<std::shared_ptr<FieldVariable::FieldVariable<FunctionSpaceType2,1>>, int, double>
>::transfer(const std::tuple<std::shared_ptr<FieldVariable::FieldVariable<FunctionSpaceType1,nComponents1>>, int, double> &transferableSolutionData1,
            const std::tuple<std::shared_ptr<FieldVariable::FieldVariable<FunctionSpaceType2,1>>, int, double> &transferableSolutionData2)
{
  // rename input data
  std::shared_ptr<FieldVariable::FieldVariable<FunctionSpaceType1,nComponents1>> fieldVariable1 = std::get<0>(transferableSolutionData1);
  std::shared_ptr<FieldVariable::FieldVariable<FunctionSpaceType2,1>> fieldVariable2 = std::get<0>(transferableSolutionData2);

  // disable checking for nans and infs because it take a lot of time
  //fieldVariable1->checkNansInfs();

  int componentNo1 = std::get<1>(transferableSolutionData1);
  int componentNo2 = 0;

  double prefactor1 = std::get<2>(transferableSolutionData1);
  double prefactor2 = std::get<2>(transferableSolutionData2);

  VLOG(1) << "solution vector mapping, transfer from component "
    << componentNo1 << " (" << fieldVariable1->nDofsLocalWithoutGhosts() << " dofs), prefactor " << prefactor1
    << " to " << componentNo2 << " (" << fieldVariable2->nDofsLocalWithoutGhosts() << " dofs), prefactor " << prefactor2 << " (not considered here)";

  assert(fieldVariable1->nDofsLocalWithoutGhosts() == fieldVariable2->nDofsLocalWithoutGhosts());
  assert(fieldVariable1->nDofsGlobal() == fieldVariable2->nDofsGlobal());

  // if representation of fieldVariable1 is invalid, this means that it has been extracted to another field variable
  if (fieldVariable2->partitionedPetscVec()->currentRepresentation() == Partition::values_representation_t::representationInvalid)
  {
    LOG(FATAL) << "fieldVariable2 has invalid representation (should not happen). Maybe check if both field variables have nComponents = 1 (this is not implemented).";
  }
  else
  {
    LOG(DEBUG) << "SolutionVectorMapping extractComponentShared (3)";
    // this retrieves the raw memory pointer from the Petsc vector in fieldVariable1 and reuses it for fieldVariable2
    // that means that fieldVariable cannot be used anymore, only after restoreExtractedComponent was called on fieldVariable1. This is done in the other solution_vector_mapping transfer call.
    fieldVariable1->extractComponentShared(componentNo1, fieldVariable2);
  }

  // scale result with prefactor1
  if (prefactor1 != 1.0)
  {
    PetscErrorCode ierr;
    ierr = VecScale(fieldVariable2->valuesGlobal(componentNo2), prefactor1); CHKERRV(ierr);
  }
}
