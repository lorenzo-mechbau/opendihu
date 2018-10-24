#pragma once

#include <Python.h>  // has to be the first included header
#include <petscmat.h>
#include <memory>

#include "data_management/data.h"
#include "control/types.h"
#include "mesh/mesh.h"

/**
 * The Data classes contain each a vector that stores the solution. Often, the values need to be accessed to
 * continue computation using them, e.g. in operator splittings. Then possibly not all the values need to be accessed
 * but only some of them while the others are only needed for the own computation. An example is a cellml model that
 * contains lots of internal states and only a single state variable is needed in the diffusion equation.
 *
 * Because the data may be stored in different forms, e.g. as raw Vec, as FieldVariable with several components where only one component is the solution variable
 * or as nested vector, the transfer function that transfers the data from one object to another is implemented using partial specializations of this class.
 *
 * The data is also scaled with a prefactor, that can be given. This is needed for the results of the cellml class.
 */
template<typename TransferableSolutionDataType1, typename TransferableSolutionDataType2>
class SolutionVectorMapping
{
};

/** Transfer between two field variables with given component number
 */
template<typename FunctionSpaceType1, int nComponents1, typename FunctionSpaceType2, int nComponents2>
class SolutionVectorMapping<
  std::tuple<std::shared_ptr<FieldVariable::FieldVariable<FunctionSpaceType1,nComponents1>>, int, double>,   // <fieldVariableType,componentNo,prefactor>
  std::tuple<std::shared_ptr<FieldVariable::FieldVariable<FunctionSpaceType2,nComponents2>>, int, double>
>
{
public:
  ///! transfer the data from transferableSolutionData1 to transferableSolutionData2, as efficient as possible
  static void transfer(std::tuple<std::shared_ptr<FieldVariable::FieldVariable<FunctionSpaceType1,nComponents1>>,int,double> transferableSolutionData1,
                       std::tuple<std::shared_ptr<FieldVariable::FieldVariable<FunctionSpaceType2,nComponents2>>,int,double> transferableSolutionData2);
};

/** Transfer between one scalar field variable (e.g. from finite element method) and one field variables with given component number and prefactor (e.g. from cellml)
 */
template<typename FunctionSpaceType1, typename FunctionSpaceType2, int nComponents2>
class SolutionVectorMapping<
  std::shared_ptr<FieldVariable::FieldVariable<FunctionSpaceType1,1>>,   // <fieldVariableType,componentNo>
  std::tuple<std::shared_ptr<FieldVariable::FieldVariable<FunctionSpaceType2,nComponents2>>,int,double>
> :
  public SolutionVectorMapping<
    std::tuple<std::shared_ptr<FieldVariable::FieldVariable<FunctionSpaceType1,1>>,int,double>,
    std::tuple<std::shared_ptr<FieldVariable::FieldVariable<FunctionSpaceType2,nComponents2>>,int,double>
  >
{
public:
  ///! transfer the data from transferableSolutionData1 to transferableSolutionData2, as efficient as possible
  static void transfer(std::shared_ptr<FieldVariable::FieldVariable<FunctionSpaceType1,1>> transferableSolutionData1,
                       std::tuple<std::shared_ptr<FieldVariable::FieldVariable<FunctionSpaceType2,nComponents2>>,int,double> transferableSolutionData2);

};


#include "operator_splitting/solution_vector_mapping.tpp"