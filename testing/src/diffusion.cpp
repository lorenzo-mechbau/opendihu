#include <iostream>
#include <cstdlib>
#include <fstream>

#include "gtest/gtest.h"
#include "opendihu.h"
#include "control/petsc_utility.h"
#include "arg.h"
#include "stiffness_matrix_tester.h"
#include "equation/diffusion.h"

namespace SpatialDiscretization
{
  
TEST(DiffusionTest, Compiles)
{
  std::string pythonConfig = R"(
# Diffusion 1D
n = 5
config = {
  "disablePrinting": False,
  "disableMatrixPrinting": False,
  "FiniteElementMethod" : {
    "nElements": n,
    "physicalExtend": 4.0,
    "relativeTolerance": 1e-15,
  },
  "ExplicitEuler" : {
    "initialValues": [2,2,4,5,2,2],
    "numberTimeSteps": 5,
    "endTime": 1.0,
  },
  "OutputWriter" : [
    {"format": "Paraview", "interval": 1, "filename": "out", "binaryOutput": "false", "fixedFormat": False},
    {"format": "Python", "filename": "p", "frequency": 1}
  ]
}
)";

  DihuContext settings(argc, argv, pythonConfig);
  
  TimeSteppingScheme::ExplicitEuler<
    FiniteElementMethod<
      Mesh::RegularFixed<1>,
      BasisFunction::Lagrange,
      Equation::Dynamic::Diffusion
    >
  > equationDiscretized(settings);
  
  Computation computation(settings, equationDiscretized);
  computation.run();
}

} // namespace