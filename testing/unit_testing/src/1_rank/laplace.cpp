#include <Python.h>  // this has to be the first included header

#include <iostream>
#include <cstdlib>
#include <fstream>

#include "gtest/gtest.h"
#include "opendihu.h"
#include "arg.h"
#include "stiffness_matrix_tester.h"
#include "node_positions_tester.h"

namespace SpatialDiscretization
{
  
TEST(LaplaceTest, MatrixIsCorrect1DSmall)
{
  std::string pythonConfig = R"(
# Laplace 1D
n = 5
    
# boundary conditions
bc = {}
bc[0] = 1.0
bc[n] = 0.0

config = {
  "FiniteElementMethod": {
    "nElements": n,
    "physicalExtent": 4.0,
    "dirichletBoundaryConditions": bc,
    "relativeTolerance": 1e-15,
  }
}
)";

  DihuContext settings(argc, argv, pythonConfig);
  
  FiniteElementMethod<
    Mesh::StructuredRegularFixedOfDimension<1>,
    BasisFunction::LagrangeOfOrder<>,
    Quadrature::None,
    Equation::Static::Laplace
  > equationDiscretized(settings);
  
  equationDiscretized.run();

  std::vector<double> referenceMatrix = {
    1, 0, 0, 0, 0, 0,
    0, -2.5, 1.25, 0, 0, 0,
    0, 1.25, -2.5, 1.25, 0, 0,
    0, 0, 1.25, -2.5, 1.25, 0, 
    0, 0, 0, 1.25, -2.5, 0,
    0, 0, 0, 0, 0, 1
  };
  std::vector<double> referenceRhs = {
    1, -1.25, 0, 0, 0, 0
  };
  std::map<int, double> dirichletBC = {{0, 1.0}, {5,0.0}};
  
  StiffnessMatrixTester::compareMatrix(equationDiscretized, referenceMatrix);
  StiffnessMatrixTester::compareRhs(equationDiscretized, referenceRhs);
  StiffnessMatrixTester::checkDirichletBCInSolution(equationDiscretized, dirichletBC);
}

TEST(LaplaceTest, MatrixIsCorrect1DBig)
{
  std::string pythonConfig = R"(
# Laplace 1D
n = 10
    
# boundary conditions
bc = {}
bc[0] = 1.0
bc[n] = 2.0

config = {
  "disablePrinting": False,
  "disableMatrixPrinting": False,
  "FiniteElementMethod" : {
    "nElements": n,
    "physicalExtent": 0.5,
    "dirichletBoundaryConditions": bc,
    "relativeTolerance": 1e-15,
  },
}
)";

  DihuContext settings(argc, argv, pythonConfig);
  
  FiniteElementMethod<
    Mesh::StructuredRegularFixedOfDimension<1>,
    BasisFunction::LagrangeOfOrder<>,
    Quadrature::None,
    Equation::Static::Laplace
  > equationDiscretized(settings);
  
  equationDiscretized.run();

  std::vector<double> referenceMatrix = {
    1,    0,    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, -40, 20, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 20, -40, 20, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 20, -40, 20, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 20, -40, 20, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 20, -40, 20, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 20, -40, 20, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 20, -40, 20, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 20, -40, 20, 0,
    0, 0, 0, 0, 0, 0, 0, 0,    20, -40, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
  };
  std::vector<double> referenceRhs = {
    1, -20, 0, 0, 0, 0, 0, 0, 0, -40, 2
  };
  std::map<int, double> dirichletBC = {{0, 1.0}, {10,2.0}};
  StiffnessMatrixTester::compareMatrix(equationDiscretized, referenceMatrix);
  StiffnessMatrixTester::compareRhs(equationDiscretized, referenceRhs);
  StiffnessMatrixTester::checkDirichletBCInSolution(equationDiscretized, dirichletBC);
}

TEST(LaplaceTest, MatrixIsCorrect1DStencils)
{
  std::string pythonConfig = R"(
# Laplace 1D
n = 10
    
# no boundary conditions
config = {
  "disablePrinting": False,
  "disableMatrixPrinting": False,
  "FiniteElementMethod" : {
    "nElements": n,
    "physicalExtent": n,
    "relativeTolerance": 1e-15,
  },
}
)";

  DihuContext settings(argc, argv, pythonConfig);
  
  FiniteElementMethod<
    Mesh::StructuredRegularFixedOfDimension<1>,
    BasisFunction::LagrangeOfOrder<>,
    Quadrature::None,
    Equation::Static::Laplace
  > equationDiscretized(settings);
  
  equationDiscretized.run();

  std::vector<double> referenceMatrix(121, 0.0);
  std::array<double, 2> stencil = {-1.0, 1.0};
    // stencil for -Δu in 1D: [1 _-2_ 1] (element contribution: [_-1_ 1])
  
  // fill with stencil values
  auto matrixIndex = [](int x, int y){return y*11 + x;};
  
  // loop over elements
  for (int i=0; i<10; i++)
  {
    // nodes:
    // 0--1
    int node0 = i;
    int node1 = i+1;
    
    // add contribution from node 0 to all nodes
    referenceMatrix[matrixIndex(node0, node0)] += stencil[0];
    referenceMatrix[matrixIndex(node0, node1)] += stencil[1];
    
    // add contribution from node 1 to all nodes
    referenceMatrix[matrixIndex(node1, node0)] += stencil[1];
    referenceMatrix[matrixIndex(node1, node1)] += stencil[0];
  }
  
  StiffnessMatrixTester::compareMatrix(equationDiscretized, referenceMatrix);
}

TEST(LaplaceTest, StructuredDeformableMatrixIsCorrect1DStencils)
{
  std::string pythonConfig = R"(
# Laplace 1D
n = 10
    
# no boundary conditions
config = {
  "disablePrinting": False,
  "disableMatrixPrinting": False,
  "FiniteElementMethod" : {
    "nElements": n,
    "physicalExtent": n,
    "relativeTolerance": 1e-15,
  },
}
)";

  DihuContext settings(argc, argv, pythonConfig);
  
  FiniteElementMethod<
    Mesh::StructuredDeformableOfDimension<1>,
    BasisFunction::LagrangeOfOrder<>,
    Quadrature::Gauss<2>,
    Equation::Static::Laplace
  > equationDiscretized(settings);
  
  equationDiscretized.run();

  std::vector<double> referenceMatrix(121, 0.0);
  std::array<double, 2> stencil = {-1.0, 1.0};
    // stencil for -Δu in 1D: [1 _-2_ 1] (element contribution: [_-1_ 1])
  
  // fill with stencil values
  auto matrixIndex = [](int x, int y){return y*11 + x;};
  
  // loop over elements
  for (int i=0; i<10; i++)
  {
    // nodes:
    // 0--1
    int node0 = i;
    int node1 = i+1;
    
    // add contribution from node 0 to all nodes
    referenceMatrix[matrixIndex(node0, node0)] += stencil[0];
    referenceMatrix[matrixIndex(node0, node1)] += stencil[1];
    
    // add contribution from node 1 to all nodes
    referenceMatrix[matrixIndex(node1, node0)] += stencil[1];
    referenceMatrix[matrixIndex(node1, node1)] += stencil[0];
  }
  
  StiffnessMatrixTester::compareMatrix(equationDiscretized, referenceMatrix);
}

TEST(LaplaceTest, MatrixIsCorrect2DSmall)
{
  std::string pythonConfig = R"(
# Laplace 2D,  3x4 nodes
n = 2.
m = 3.

# boundary conditions
bc = {}
for i in range(int(n+1)):
  x = i/(n+1.)
  bc[i] = x
  i2 = (n+1)*m + i
  bc[int(i2)] = x
  
  print("bc[{}] = {}, bc[{}] = {}".format(i, bc[i], i2, bc[i2]))

#bc = {0: 0, 1: 1./3, 2: 2./3,
#      9: 0, 10: 1./3, 11: 2./3}

config = {
  "disablePrinting": False,
  "disableMatrixPrinting": False,
  "FiniteElementMethod" : {
    "nElements": [n, m],
    "physicalExtent": [6.0, 9.0],
    "dirichletBoundaryConditions": bc,
    "relativeTolerance": 1e-15,
  },
}
)";

  DihuContext settings(argc, argv, pythonConfig);
  
  FiniteElementMethod<
    Mesh::StructuredRegularFixedOfDimension<2>,
    BasisFunction::LagrangeOfOrder<>,
    Quadrature::None,
    Equation::Static::Laplace
  > equationDiscretized(settings);
  
  equationDiscretized.run();

  std::vector<double> referenceMatrix = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0,-12/9.,3/9., 0,1.5/9.,3/9., 0, 0, 0, 0,
    0, 0, 0,3/9.,-24/9.,3/9.,3/9.,3/9., 3/9., 0, 0, 0,
    0, 0, 0, 0,3/9.,-12/9.,0,3/9.,1.5/9., 0, 0, 0,
    0, 0, 0,1.5/9.,3/9.,0,-12/9.,3/9., 0, 0, 0, 0,
    0, 0, 0,3/9.,3/9.,3/9.,3/9.,-24/9., 3/9., 0, 0, 0,
    0, 0, 0,0,3/9.,1.5/9.,0,3/9.,-12/9., 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
  };
  std::vector<double> referenceRhs = {
    0, 1./3, 2./3, -1./9, -3./9, -2./9, -1./9, -3/9., -2./9, 0, 1./3, 2./3
  };
  std::map<int, double> dirichletBC = {{0, 0.0}, {1,1./3}, {2,2./3}, {9,0}, {10,1./3}, {11,2./3}};
  
  StiffnessMatrixTester::compareMatrix(equationDiscretized, referenceMatrix);
  StiffnessMatrixTester::compareRhs(equationDiscretized, referenceRhs);
  StiffnessMatrixTester::checkDirichletBCInSolution(equationDiscretized, dirichletBC);
}

TEST(LaplaceTest, MatrixIsCorrect2DStencils)
{
  std::string pythonConfig = R"(
# Laplace 2D
config = {
  "disablePrinting": False,
  "disableMatrixPrinting": True,
  "FiniteElementMethod" : {
    "nElements": [4, 4],
    "physicalExtent": [4.0, 4.0],
    "relativeTolerance": 1e-15,
  },
}
)";

  DihuContext settings(argc, argv, pythonConfig);
  
  FiniteElementMethod<
    Mesh::StructuredRegularFixedOfDimension<2>,
    BasisFunction::LagrangeOfOrder<>,
    Quadrature::None,
    Equation::Static::Laplace
  > equationDiscretized(settings);
  
  equationDiscretized.run();

  std::vector<double> referenceMatrix(625, 0.0);
  
  // stencil for -Δu in 2D:     [1  1   1] (element contribution: [  1/6  1/3])
  //                        1/3*[1 _-8_ 1]                        [_-2/3_ 1/6]
  //                            [1  1   1]
  double stencil[2][2] = {{-2./3, 1./6}, {1./6, 1./3}};
  
  // fill with stencil values
  auto matrixIndex = [](int x, int y){return y*25 + x;};
  
  // loop over elements
  for (int x=0; x<4; x++)
  {
    for (int y=0; y<4; y++)
    {
      // nodes:
      // 2--3
      // 0--1
      int node0 = y*5 + x;
      int node1 = y*5 + x+1;
      int node2 = (y+1)*5 + x;
      int node3 = (y+1)*5 + x+1;
      
      // add contribution from node 0 to all nodes
      referenceMatrix[matrixIndex(node0, node0)] += stencil[0][0];
      referenceMatrix[matrixIndex(node0, node1)] += stencil[0][1];
      referenceMatrix[matrixIndex(node0, node2)] += stencil[1][0];
      referenceMatrix[matrixIndex(node0, node3)] += stencil[1][1];
      
      // add contribution from node 1 to all nodes
      referenceMatrix[matrixIndex(node1, node0)] += stencil[0][1];
      referenceMatrix[matrixIndex(node1, node1)] += stencil[0][0];
      referenceMatrix[matrixIndex(node1, node2)] += stencil[1][1];
      referenceMatrix[matrixIndex(node1, node3)] += stencil[1][0];
      
      // add contribution from node 2 to all nodes
      referenceMatrix[matrixIndex(node2, node0)] += stencil[1][0];
      referenceMatrix[matrixIndex(node2, node1)] += stencil[1][1];
      referenceMatrix[matrixIndex(node2, node2)] += stencil[0][0];
      referenceMatrix[matrixIndex(node2, node3)] += stencil[0][1];
      
      // add contribution from node 3 to all nodes
      referenceMatrix[matrixIndex(node3, node0)] += stencil[1][1];
      referenceMatrix[matrixIndex(node3, node1)] += stencil[1][0];
      referenceMatrix[matrixIndex(node3, node2)] += stencil[0][1];
      referenceMatrix[matrixIndex(node3, node3)] += stencil[0][0];
    }
  }
    
  StiffnessMatrixTester::compareMatrix(equationDiscretized, referenceMatrix);
}

TEST(LaplaceTest, StructuredDeformableMatrixIsCorrect2DStencils)
{
  std::string pythonConfig = R"(
# Laplace 2D
config = {
  "disablePrinting": False,
  "disableMatrixPrinting": True,
  "FiniteElementMethod" : {
    "nElements": [4, 4],
    "physicalExtent": [4.0, 4.0],
    "relativeTolerance": 1e-15,
  },
}
)";

  DihuContext settings(argc, argv, pythonConfig);
  
  FiniteElementMethod<
    Mesh::StructuredDeformableOfDimension<2>,
    BasisFunction::LagrangeOfOrder<>,
    Quadrature::Gauss<2>,
    Equation::Static::Laplace
  > equationDiscretized(settings);
  
  equationDiscretized.run();

  std::vector<double> referenceMatrix(625, 0.0);
  
  // stencil for -Δu in 2D:     [1  1   1] (element contribution: [  1/6  1/3])
  //                        1/3*[1 _-8_ 1]                        [_-2/3_ 1/6]
  //                            [1  1   1]
  double stencil[2][2] = {{-2./3, 1./6}, {1./6, 1./3}};
  
  // fill with stencil values
  auto matrixIndex = [](int x, int y){return y*25 + x;};
  
  // loop over elements
  for (int x=0; x<4; x++)
  {
    for (int y=0; y<4; y++)
    {
      // nodes:
      // 2--3
      // 0--1
      int node0 = y*5 + x;
      int node1 = y*5 + x+1;
      int node2 = (y+1)*5 + x;
      int node3 = (y+1)*5 + x+1;
      
      // add contribution from node 0 to all nodes
      referenceMatrix[matrixIndex(node0, node0)] += stencil[0][0];
      referenceMatrix[matrixIndex(node0, node1)] += stencil[0][1];
      referenceMatrix[matrixIndex(node0, node2)] += stencil[1][0];
      referenceMatrix[matrixIndex(node0, node3)] += stencil[1][1];
      
      // add contribution from node 1 to all nodes
      referenceMatrix[matrixIndex(node1, node0)] += stencil[0][1];
      referenceMatrix[matrixIndex(node1, node1)] += stencil[0][0];
      referenceMatrix[matrixIndex(node1, node2)] += stencil[1][1];
      referenceMatrix[matrixIndex(node1, node3)] += stencil[1][0];
      
      // add contribution from node 2 to all nodes
      referenceMatrix[matrixIndex(node2, node0)] += stencil[1][0];
      referenceMatrix[matrixIndex(node2, node1)] += stencil[1][1];
      referenceMatrix[matrixIndex(node2, node2)] += stencil[0][0];
      referenceMatrix[matrixIndex(node2, node3)] += stencil[0][1];
      
      // add contribution from node 3 to all nodes
      referenceMatrix[matrixIndex(node3, node0)] += stencil[1][1];
      referenceMatrix[matrixIndex(node3, node1)] += stencil[1][0];
      referenceMatrix[matrixIndex(node3, node2)] += stencil[0][1];
      referenceMatrix[matrixIndex(node3, node3)] += stencil[0][0];
    }
  }
    
  StiffnessMatrixTester::compareMatrix(equationDiscretized, referenceMatrix);
}

TEST(LaplaceTest, MatrixIsCorrect3DStencils)
{
  std::string pythonConfig = R"(
# Laplace 3D
config = {
  "disablePrinting": False,
  "disableMatrixPrinting": True,
  "FiniteElementMethod" : {
    "nElements": [4, 4, 4],
    "physicalExtent": [4.0, 4.0, 4.0],
    "relativeTolerance": 1e-15,
  },
}
)";

  DihuContext settings(argc, argv, pythonConfig);
  
  FiniteElementMethod<
    Mesh::StructuredRegularFixedOfDimension<3>,
    BasisFunction::LagrangeOfOrder<>,
    Quadrature::None,
    Equation::Static::Laplace
  > equationDiscretized(settings);
  
  
  equationDiscretized.run();

  std::vector<double> referenceMatrix(15625, 0.0);
  
  const double stencil[2][2][2] = {
    {{-4./12, 0./12},
    {0./12, 1./12}},    //center
    {{0./12, 1./12},
    {1./12, 1./12}},    //bottom
  };
  
  // fill with stencil values
  auto matrixIndex = [](int i, int j){return j*125+i;};
  
  // loop over elements
  for (int x=0; x<4; x++)
  {
    for (int y=0; y<4; y++)
    {
      for (int z=0; z<4; z++)
      {
        // nodes:
        // 6--7
        // 4--5
        //
        // 2--3
        // 0--1
        int node0 = z*25 + y*5 + x;
        int node1 = z*25 + y*5 + x+1;
        int node2 = z*25 + (y+1)*5 + x;
        int node3 = z*25 + (y+1)*5 + x+1;
        int node4 = (z+1)*25 + y*5 + x;
        int node5 = (z+1)*25 + y*5 + x+1;
        int node6 = (z+1)*25 + (y+1)*5 + x;
        int node7 = (z+1)*25 + (y+1)*5 + x+1;
        
        // add contribution from node 0 to all nodes          x  y  z
        referenceMatrix[matrixIndex(node0, node0)] += stencil[0][0][0];
        referenceMatrix[matrixIndex(node0, node1)] += stencil[0][1][0];
        referenceMatrix[matrixIndex(node0, node2)] += stencil[1][0][0];
        referenceMatrix[matrixIndex(node0, node3)] += stencil[1][1][0];
        referenceMatrix[matrixIndex(node0, node4)] += stencil[0][0][1];
        referenceMatrix[matrixIndex(node0, node5)] += stencil[0][1][1];
        referenceMatrix[matrixIndex(node0, node6)] += stencil[1][0][1];
        referenceMatrix[matrixIndex(node0, node7)] += stencil[1][1][1];
        
        // add contribution from node 1 to all nodes
        referenceMatrix[matrixIndex(node1, node0)] += stencil[0][1][0];
        referenceMatrix[matrixIndex(node1, node1)] += stencil[0][0][0];
        referenceMatrix[matrixIndex(node1, node2)] += stencil[1][1][0];
        referenceMatrix[matrixIndex(node1, node3)] += stencil[1][0][0];
        referenceMatrix[matrixIndex(node1, node4)] += stencil[0][1][1];
        referenceMatrix[matrixIndex(node1, node5)] += stencil[0][0][1];
        referenceMatrix[matrixIndex(node1, node6)] += stencil[1][1][1];
        referenceMatrix[matrixIndex(node1, node7)] += stencil[1][0][1];
        
        // add contribution from node 2 to all nodes
        referenceMatrix[matrixIndex(node2, node0)] += stencil[1][0][0];
        referenceMatrix[matrixIndex(node2, node1)] += stencil[1][1][0];
        referenceMatrix[matrixIndex(node2, node2)] += stencil[0][0][0];
        referenceMatrix[matrixIndex(node2, node3)] += stencil[0][1][0];
        referenceMatrix[matrixIndex(node2, node4)] += stencil[1][0][1];
        referenceMatrix[matrixIndex(node2, node5)] += stencil[1][1][1];
        referenceMatrix[matrixIndex(node2, node6)] += stencil[0][0][1];
        referenceMatrix[matrixIndex(node2, node7)] += stencil[0][1][1];
        
        // add contribution from node 3 to all nodes
        referenceMatrix[matrixIndex(node3, node0)] += stencil[1][1][0];
        referenceMatrix[matrixIndex(node3, node1)] += stencil[1][0][0];
        referenceMatrix[matrixIndex(node3, node2)] += stencil[0][1][0];
        referenceMatrix[matrixIndex(node3, node3)] += stencil[0][0][0];
        referenceMatrix[matrixIndex(node3, node4)] += stencil[1][1][1];
        referenceMatrix[matrixIndex(node3, node5)] += stencil[1][0][1];
        referenceMatrix[matrixIndex(node3, node6)] += stencil[0][1][1];
        referenceMatrix[matrixIndex(node3, node7)] += stencil[0][0][1];
        
        // add contribution from node 4 to all nodes          x  y  z
        referenceMatrix[matrixIndex(node4, node0)] += stencil[0][0][1];
        referenceMatrix[matrixIndex(node4, node1)] += stencil[0][1][1];
        referenceMatrix[matrixIndex(node4, node2)] += stencil[1][0][1];
        referenceMatrix[matrixIndex(node4, node3)] += stencil[1][1][1];
        referenceMatrix[matrixIndex(node4, node4)] += stencil[0][0][0];
        referenceMatrix[matrixIndex(node4, node5)] += stencil[0][1][0];
        referenceMatrix[matrixIndex(node4, node6)] += stencil[1][0][0];
        referenceMatrix[matrixIndex(node4, node7)] += stencil[1][1][0];
        
        // add contribution from node 5 to all nodes
        referenceMatrix[matrixIndex(node5, node0)] += stencil[0][1][1];
        referenceMatrix[matrixIndex(node5, node1)] += stencil[0][0][1];
        referenceMatrix[matrixIndex(node5, node2)] += stencil[1][1][1];
        referenceMatrix[matrixIndex(node5, node3)] += stencil[1][0][1];
        referenceMatrix[matrixIndex(node5, node4)] += stencil[0][1][0];
        referenceMatrix[matrixIndex(node5, node5)] += stencil[0][0][0];
        referenceMatrix[matrixIndex(node5, node6)] += stencil[1][1][0];
        referenceMatrix[matrixIndex(node5, node7)] += stencil[1][0][0];
        
        // add contribution from node 6 to all nodes
        referenceMatrix[matrixIndex(node6, node0)] += stencil[1][0][1];
        referenceMatrix[matrixIndex(node6, node1)] += stencil[1][1][1];
        referenceMatrix[matrixIndex(node6, node2)] += stencil[0][0][1];
        referenceMatrix[matrixIndex(node6, node3)] += stencil[0][1][1];
        referenceMatrix[matrixIndex(node6, node4)] += stencil[1][0][0];
        referenceMatrix[matrixIndex(node6, node5)] += stencil[1][1][0];
        referenceMatrix[matrixIndex(node6, node6)] += stencil[0][0][0];
        referenceMatrix[matrixIndex(node6, node7)] += stencil[0][1][0];
        
        // add contribution from node 7 to all nodes
        referenceMatrix[matrixIndex(node7, node0)] += stencil[1][1][1];
        referenceMatrix[matrixIndex(node7, node1)] += stencil[1][0][1];
        referenceMatrix[matrixIndex(node7, node2)] += stencil[0][1][1];
        referenceMatrix[matrixIndex(node7, node3)] += stencil[0][0][1];
        referenceMatrix[matrixIndex(node7, node4)] += stencil[1][1][0];
        referenceMatrix[matrixIndex(node7, node5)] += stencil[1][0][0];
        referenceMatrix[matrixIndex(node7, node6)] += stencil[0][1][0];
        referenceMatrix[matrixIndex(node7, node7)] += stencil[0][0][0];
      }
    }
  }
    
  StiffnessMatrixTester::compareMatrix(equationDiscretized, referenceMatrix);
}

TEST(LaplaceTest, StructuredDeformableMatrixIsCorrect3DStencils)
{
  std::string pythonConfig = R"(
# Laplace 3D
config = {
  "disablePrinting": False,
  "disableMatrixPrinting": True,
  "FiniteElementMethod" : {
    "nElements": [4, 4, 4],
    "physicalExtent": [4.0, 4.0, 4.0],
    "relativeTolerance": 1e-15,
  },
}
)";

  DihuContext settings(argc, argv, pythonConfig);
  
  FiniteElementMethod<
    Mesh::StructuredDeformableOfDimension<3>,
    BasisFunction::LagrangeOfOrder<>,
    Quadrature::Gauss<2>,
    Equation::Static::Laplace
  > equationDiscretized(settings);
  
  
  equationDiscretized.run();
  
  std::vector<double> referenceMatrix(15625, 0.0);
  
  const double stencil[2][2][2] = {
    {{-4./12, 0./12},
    {0./12, 1./12}},    //center
    {{0./12, 1./12},
    {1./12, 1./12}},    //bottom
  };
  
  // fill with stencil values
  auto matrixIndex = [](int i, int j){return j*125+i;};
  
  // loop over elements
  for (int x=0; x<4; x++)
  {
    for (int y=0; y<4; y++)
    {
      for (int z=0; z<4; z++)
      {
        // nodes:
        // 6--7
        // 4--5
        //
        // 2--3
        // 0--1
        int node0 = z*25 + y*5 + x;
        int node1 = z*25 + y*5 + x+1;
        int node2 = z*25 + (y+1)*5 + x;
        int node3 = z*25 + (y+1)*5 + x+1;
        int node4 = (z+1)*25 + y*5 + x;
        int node5 = (z+1)*25 + y*5 + x+1;
        int node6 = (z+1)*25 + (y+1)*5 + x;
        int node7 = (z+1)*25 + (y+1)*5 + x+1;
        
        // add contribution from node 0 to all nodes          x  y  z
        referenceMatrix[matrixIndex(node0, node0)] += stencil[0][0][0];
        referenceMatrix[matrixIndex(node0, node1)] += stencil[0][1][0];
        referenceMatrix[matrixIndex(node0, node2)] += stencil[1][0][0];
        referenceMatrix[matrixIndex(node0, node3)] += stencil[1][1][0];
        referenceMatrix[matrixIndex(node0, node4)] += stencil[0][0][1];
        referenceMatrix[matrixIndex(node0, node5)] += stencil[0][1][1];
        referenceMatrix[matrixIndex(node0, node6)] += stencil[1][0][1];
        referenceMatrix[matrixIndex(node0, node7)] += stencil[1][1][1];
        
        // add contribution from node 1 to all nodes
        referenceMatrix[matrixIndex(node1, node0)] += stencil[0][1][0];
        referenceMatrix[matrixIndex(node1, node1)] += stencil[0][0][0];
        referenceMatrix[matrixIndex(node1, node2)] += stencil[1][1][0];
        referenceMatrix[matrixIndex(node1, node3)] += stencil[1][0][0];
        referenceMatrix[matrixIndex(node1, node4)] += stencil[0][1][1];
        referenceMatrix[matrixIndex(node1, node5)] += stencil[0][0][1];
        referenceMatrix[matrixIndex(node1, node6)] += stencil[1][1][1];
        referenceMatrix[matrixIndex(node1, node7)] += stencil[1][0][1];
        
        // add contribution from node 2 to all nodes
        referenceMatrix[matrixIndex(node2, node0)] += stencil[1][0][0];
        referenceMatrix[matrixIndex(node2, node1)] += stencil[1][1][0];
        referenceMatrix[matrixIndex(node2, node2)] += stencil[0][0][0];
        referenceMatrix[matrixIndex(node2, node3)] += stencil[0][1][0];
        referenceMatrix[matrixIndex(node2, node4)] += stencil[1][0][1];
        referenceMatrix[matrixIndex(node2, node5)] += stencil[1][1][1];
        referenceMatrix[matrixIndex(node2, node6)] += stencil[0][0][1];
        referenceMatrix[matrixIndex(node2, node7)] += stencil[0][1][1];
        
        // add contribution from node 3 to all nodes
        referenceMatrix[matrixIndex(node3, node0)] += stencil[1][1][0];
        referenceMatrix[matrixIndex(node3, node1)] += stencil[1][0][0];
        referenceMatrix[matrixIndex(node3, node2)] += stencil[0][1][0];
        referenceMatrix[matrixIndex(node3, node3)] += stencil[0][0][0];
        referenceMatrix[matrixIndex(node3, node4)] += stencil[1][1][1];
        referenceMatrix[matrixIndex(node3, node5)] += stencil[1][0][1];
        referenceMatrix[matrixIndex(node3, node6)] += stencil[0][1][1];
        referenceMatrix[matrixIndex(node3, node7)] += stencil[0][0][1];
        
        // add contribution from node 4 to all nodes          x  y  z
        referenceMatrix[matrixIndex(node4, node0)] += stencil[0][0][1];
        referenceMatrix[matrixIndex(node4, node1)] += stencil[0][1][1];
        referenceMatrix[matrixIndex(node4, node2)] += stencil[1][0][1];
        referenceMatrix[matrixIndex(node4, node3)] += stencil[1][1][1];
        referenceMatrix[matrixIndex(node4, node4)] += stencil[0][0][0];
        referenceMatrix[matrixIndex(node4, node5)] += stencil[0][1][0];
        referenceMatrix[matrixIndex(node4, node6)] += stencil[1][0][0];
        referenceMatrix[matrixIndex(node4, node7)] += stencil[1][1][0];
        
        // add contribution from node 5 to all nodes
        referenceMatrix[matrixIndex(node5, node0)] += stencil[0][1][1];
        referenceMatrix[matrixIndex(node5, node1)] += stencil[0][0][1];
        referenceMatrix[matrixIndex(node5, node2)] += stencil[1][1][1];
        referenceMatrix[matrixIndex(node5, node3)] += stencil[1][0][1];
        referenceMatrix[matrixIndex(node5, node4)] += stencil[0][1][0];
        referenceMatrix[matrixIndex(node5, node5)] += stencil[0][0][0];
        referenceMatrix[matrixIndex(node5, node6)] += stencil[1][1][0];
        referenceMatrix[matrixIndex(node5, node7)] += stencil[1][0][0];
        
        // add contribution from node 6 to all nodes
        referenceMatrix[matrixIndex(node6, node0)] += stencil[1][0][1];
        referenceMatrix[matrixIndex(node6, node1)] += stencil[1][1][1];
        referenceMatrix[matrixIndex(node6, node2)] += stencil[0][0][1];
        referenceMatrix[matrixIndex(node6, node3)] += stencil[0][1][1];
        referenceMatrix[matrixIndex(node6, node4)] += stencil[1][0][0];
        referenceMatrix[matrixIndex(node6, node5)] += stencil[1][1][0];
        referenceMatrix[matrixIndex(node6, node6)] += stencil[0][0][0];
        referenceMatrix[matrixIndex(node6, node7)] += stencil[0][1][0];
        
        // add contribution from node 7 to all nodes
        referenceMatrix[matrixIndex(node7, node0)] += stencil[1][1][1];
        referenceMatrix[matrixIndex(node7, node1)] += stencil[1][0][1];
        referenceMatrix[matrixIndex(node7, node2)] += stencil[0][1][1];
        referenceMatrix[matrixIndex(node7, node3)] += stencil[0][0][1];
        referenceMatrix[matrixIndex(node7, node4)] += stencil[1][1][0];
        referenceMatrix[matrixIndex(node7, node5)] += stencil[1][0][0];
        referenceMatrix[matrixIndex(node7, node6)] += stencil[0][1][0];
        referenceMatrix[matrixIndex(node7, node7)] += stencil[0][0][0];
      }
    }
  }
    
  StiffnessMatrixTester::compareMatrix(equationDiscretized, referenceMatrix);
}

TEST(LaplaceTest, SolverManagerWorks)
{
  std::string pythonConfig = R"(
# Laplace 1D
n = 5
    
# boundary conditions
bc = {}
bc[0] = 1.0
bc[n] = 0.0

config = {
  "disablePrinting": False,
  "disableMatrixPrinting": False,
  "Solvers" : {
    "linearSolver": {
      "relativeTolerance": 1e-15
    },
  },
  "FiniteElementMethod" : {
    "nElements": n,
    "physicalExtent": 4.0,
    "dirichletBoundaryConditions": bc,
    "solverName": "linearSolver"
  }
}
)";

  DihuContext settings(argc, argv, pythonConfig);
  
  FiniteElementMethod<
    Mesh::StructuredRegularFixedOfDimension<1>,
    BasisFunction::LagrangeOfOrder<>,
    Quadrature::None,
    Equation::Static::Laplace
  > equationDiscretized(settings);
  
  
  equationDiscretized.run();

  std::vector<double> referenceMatrix = {
    1, 0, 0, 0, 0, 0,
    0, -2.5, 1.25, 0, 0, 0,
    0, 1.25, -2.5, 1.25, 0, 0,
    0, 0, 1.25, -2.5, 1.25, 0, 
    0, 0, 0, 1.25, -2.5, 0,
    0, 0, 0, 0, 0, 1
  };
  std::vector<double> referenceRhs = {
    1, -1.25, 0, 0, 0, 0
  };
  std::map<int, double> dirichletBC = {{0, 1.0}, {5,0.0}};
  
  StiffnessMatrixTester::compareMatrix(equationDiscretized, referenceMatrix);
  StiffnessMatrixTester::compareRhs(equationDiscretized, referenceRhs);
  StiffnessMatrixTester::checkDirichletBCInSolution(equationDiscretized, dirichletBC);
}

}  // namespace

