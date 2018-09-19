#pragma once

#include <Python.h>  // has to be the first included header
#include <petscmat.h>
#include <memory>

#include "control/types.h"
#include "mesh/mesh.h"
#include "data_management/data.h"
#include "data_management/time_stepping.h"
#include "field_variable/field_variable.h"

namespace Data
{

/**  The datastructures used for multidomain solver.
  */
template<typename FunctionSpaceType>
class Multidomain : public Data<FunctionSpaceType>
{
public:

  typedef FieldVariable::FieldVariable<FunctionSpaceType,1> FieldVariableType;
  typedef FieldVariable::FieldVariable<FunctionSpaceType,3> GradientFieldVariableType;

  //! constructor
  Multidomain(DihuContext context, int nCompartments);

  //! return a reference to the rhs summand vector which is needed to apply the boundary conditions, the PETSc Vec can be obtained via fieldVariable->valuesGlobal()
  std::shared_ptr<GradientFieldVariableType> fibreDirection();

  //! return the field variable of the potential for the Laplace potential flow problem
  std::shared_ptr<FieldVariableType> flowPotential();

  //! return the extra-cellular potential field variable
  std::shared_ptr<FieldVariableType> extraCellularPotential();

  //! return the trans-membrane potential field variable for MU compartmentNo
  std::shared_ptr<FieldVariableType> transmembranePotential(int compartmentNo);

  //! print all stored data to stdout
  void print() override;

  //! field variables that will be output by outputWriters
  typedef std::tuple<
    GradientFieldVariableType,     // fibreDirection
    FieldVariableType,              // extra-cellular potential
    std::vector<FunctionSpaceType>    // transmembrane potentials
  > OutputFieldVariables;

  //! get pointers to all field variables that can be written by output writers
  OutputFieldVariables getOutputFieldVariables();

private:

  //! initializes the vectors with size
  void createPetscObjects() override;

  int nCompartments_;     ///< number of compartments i.e. motor units
  std::shared_ptr<FieldVariableType> flowPotential_; ///< the direction of fibres
  std::shared_ptr<GradientFieldVariableType> fibreDirection_; ///< the direction of fibres
  std::vector<std::shared_ptr<FieldVariableType>> transmembranePotential_;  ///< the Vm value for the compartments
  std::vector<std::shared_ptr<FieldVariableType>> extraCellularPotential_;  ///< the phi_e value which is the extra-cellular potential
};

} // namespace Data

#include "data_management/multidomain.tpp"
