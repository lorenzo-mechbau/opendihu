#include <iostream>
#include <cstdlib>

#include "opendihu.h"

int main(int argc, char *argv[])
{
  // 3D multidomain: implicit Euler, FEM
  
  // initialize everything, handle arguments and parse settings from input file
  DihuContext settings(argc, argv);
  
  typedef Mesh::StructuredDeformableOfDimension<3> MeshType;

  TimeSteppingScheme::MultidomainSolver<              // multidomain
    SpatialDiscretization::FiniteElementMethod<       //FEM for initial potential flow, fibre directions
      MeshType,
      BasisFunction::LagrangeOfOrder<1>,
      Quadrature::Gauss<3>,
      Equation::Static::Laplace
    >,
    CellmlAdapter<                // Hodgkin-Huxley
      4,
      FunctionSpace::FunctionSpace<MeshType,BasisFunction::LagrangeOfOrder<1>>
    >,
    SpatialDiscretization::FiniteElementMethod<   // anisotropic diffusion
      MeshType,
      BasisFunction::LagrangeOfOrder<1>,
      Quadrature::Gauss<3>,
      Equation::Dynamic::DirectionalDiffusion
    >
  > problem(settings);
  
  problem.run();
  
  return EXIT_SUCCESS;
}
