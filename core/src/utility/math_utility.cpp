#include "utility/math_utility.h"

#include <cmath>

namespace MathUtility
{

double sqr(double v)
{
  return v*v;
}

int sqr(int v)
{
  return v*v;
}

double length(const Vec3 node)
{
  return sqrt(sqr(node[0]) 
    + sqr(node[1])
    + sqr(node[2]));
}

double norm(const Vec3 node)
{
  return length(node);
}
  
double distance(const Vec3 node1, const Vec3 node2)
{
  return sqrt(sqr(node1[0]-node2[0]) 
    + sqr(node1[1]-node2[1])
    + sqr(node1[2]-node2[2]));
}

std::array<double,9> computeTransformationMatrixAndDeterminant(const std::array<Vec3,3> &jacobian, double &determinant)
{
  // rename input values
  const double m11 = jacobian[0][0];
  const double m21 = jacobian[0][1];
  const double m31 = jacobian[0][2];
  const double m12 = jacobian[1][0];
  const double m22 = jacobian[1][1];
  const double m32 = jacobian[1][2];
  const double m13 = jacobian[2][0];
  const double m23 = jacobian[2][1];
  const double m33 = jacobian[2][2];
  
  // the following code is copy&pasted from the output of ./invert_mapping.py 
  determinant = m11*m22*m33 - m11*m23*m32 - m12*m21*m33 + m12*m23*m31 + m13*m21*m32 - m13*m22*m31;
  double prefactor = 1./sqr(determinant);
  
  // 0 1 2
  // 3 4 5
  // 6 7 8
  
  // compute matrix entries of symmetric matrix
  std::array<double, 9> result;
  result[0] = prefactor * (sqr(m12*m23 - m13*m22) + sqr(m12*m33 - m13*m32) + sqr(m22*m33 - m23*m32));
  result[1] = prefactor * (-(m11*m23 - m13*m21)*(m12*m23 - m13*m22) - (m11*m33 - m13*m31)*(m12*m33 - m13*m32) - (m21*m33 - m23*m31)*(m22*m33 - m23*m32));
  result[2] = prefactor * ((m11*m22 - m12*m21)*(m12*m23 - m13*m22) + (m11*m32 - m12*m31)*(m12*m33 - m13*m32) + (m21*m32 - m22*m31)*(m22*m33 - m23*m32));
  result[3] = result[1];
  result[4] = prefactor * (sqr(m11*m23 - m13*m21) + sqr(m11*m33 - m13*m31) + sqr(m21*m33 - m23*m31));
  result[5] = prefactor * (-(m11*m22 - m12*m21)*(m11*m23 - m13*m21) - (m11*m32 - m12*m31)*(m11*m33 - m13*m31) - (m21*m32 - m22*m31)*(m21*m33 - m23*m31));
  result[6] = result[2];
  result[7] = result[5];
  result[8] = prefactor * (sqr(m11*m22 - m12*m21) + sqr(m11*m32 - m12*m31) + sqr(m21*m32 - m22*m31));
  
  return result;
}

// 1D integration factor
template<>
double computeIntegrationFactor<1>(const std::array<Vec3,1> &jacobian)
{
  return length(jacobian[0]);
}

// 2D integration factor
template<>
double computeIntegrationFactor<2>(const std::array<Vec3,2> &jacobian)
{
  // rename input values
  const double m11 = jacobian[0][0];
  const double m21 = jacobian[0][1];
  const double m31 = jacobian[0][2];
  const double m12 = jacobian[1][0];
  const double m22 = jacobian[1][1];
  const double m32 = jacobian[1][2];
  return sqrt(sqr(m11*m22) + sqr(m11*m32) - 2*m11*m12*m21*m22 - 2*m11*m12*m31*m32 
    + sqr(m12*m21) + sqr(m12*m31) + sqr(m21*m32) - 2*m21*m22*m31*m32 + sqr(m22*m31));
}

// 3D integration factor
template<>
double computeIntegrationFactor<3>(const std::array<Vec3,3> &jacobian)
{
  // rename input values
  const double m11 = jacobian[0][0];
  const double m21 = jacobian[0][1];
  const double m31 = jacobian[0][2];
  const double m12 = jacobian[1][0];
  const double m22 = jacobian[1][1];
  const double m32 = jacobian[1][2];
  const double m13 = jacobian[2][0];
  const double m23 = jacobian[2][1];
  const double m33 = jacobian[2][2];
  
  double determinant = m11*m22*m33 - m11*m23*m32 - m12*m21*m33 + m12*m23*m31 + m13*m21*m32 - m13*m22*m31;
  
  return fabs(determinant);
}

double applyTransformation(const std::array<double,9> &transformationMatrix, const Vec3 &vector1, const Vec3 &vector2)
{
  double result;
  
  // rename input values
  const double v11 = vector1[0];
  const double v12 = vector1[1];
  const double v13 = vector1[2];
  const double v21 = vector2[0];
  const double v22 = vector2[1];
  const double v23 = vector2[2];
  // 0 1 2
  // 3 4 5
  // 6 7 8
  const double m11 = transformationMatrix[0];
  const double m12 = transformationMatrix[1];
  const double m13 = transformationMatrix[2];
  const double m22 = transformationMatrix[4];
  const double m23 = transformationMatrix[5];
  const double m33 = transformationMatrix[8];
  
  // compute result
  result = v21*(m11*v11 + m12*v12 + m13*v13) + v22*(m12*v11 + m22*v12 + m23*v13) + v23*(m13*v11 + m23*v12 + m33*v13);
  return result;
}

double applyTransformation(const std::array<double,4> &transformationMatrix, const Vec2 &vector1, const Vec2 &vector2)
{
  double result;
  
  // rename input values
  const double v11 = vector1[0];
  const double v12 = vector1[1];
  const double v21 = vector2[0];
  const double v22 = vector2[1];
  // 0 1
  // 2 3
  const double m11 = transformationMatrix[0];
  const double m12 = transformationMatrix[1];
  const double m22 = transformationMatrix[3];
  
  // compute result
  result = v21*(m11*v11 + m12*v12) + v22*(m12*v11 + m22*v12);
  return result;
}

double computeDeterminant(const std::array<Vec3,3> &jacobian)
{
  // rename input values
  const double m11 = jacobian[0][0];
  const double m21 = jacobian[0][1];
  const double m31 = jacobian[0][2];
  const double m12 = jacobian[1][0];
  const double m22 = jacobian[1][1];
  const double m32 = jacobian[1][2];
  const double m13 = jacobian[2][0];
  const double m23 = jacobian[2][1];
  const double m33 = jacobian[2][2];
  
  return m11*m22*m33 - m11*m23*m32 - m12*m21*m33 + m12*m23*m31 + m13*m21*m32 - m13*m22*m31;
}

std::array<Vec3,3> computeSymmetricInverse(const std::array<Vec3,3> &matrix, double &determinant)
{
  // rename input values
  const double m11 = matrix[0][0];
  const double m21 = matrix[0][1];
  const double m31 = matrix[0][2];
  const double m12 = matrix[1][0];
  const double m22 = matrix[1][1];
  const double m32 = matrix[1][2];
  const double m33 = matrix[2][2];
  
  determinant = m11*m22*m33 - m11*sqr(m32) - sqr(m21)*m33 + 2*m21*m31*m32 - m22*sqr(m31);
  double invDet = 1./determinant;
  
  std::array<Vec3,3> result;
  
  result[0][0] = invDet*(m22*m33 - sqr(m32));  // entry m11
  result[1][0] = invDet*(-m21*m33 + m31*m32);  // entry m12
  result[2][0] = invDet*(m21*m32 - m22*m31);   // entry m13
  result[0][1] = invDet*(-m21*m33 + m31*m32);  // entry m21
  result[1][1] = invDet*(m11*m33 - sqr(m31));  // entry m22
  result[2][1] = invDet*(-m11*m32 + m21*m31);  // entry m23
  result[0][2] = invDet*(m21*m32 - m22*m31);   // entry m31
  result[1][2] = invDet*(-m11*m32 + m21*m31);  // entry m32
  result[2][2] = invDet*(m11*m22 - sqr(m21));  // entry m33
  
  return result;
}

bool isSubsequenceOf(std::vector<int> a, std::vector<int> b, size_t &subsequenceAStartPos)
{
  if (b.empty())
    return true;
  
  // find the matching entry in vector a
  bool matchFound = false;
  size_t aIndex=0;
  for (; aIndex<a.size(); aIndex++)
  {
    if (a[aIndex] == b[0])
    {
      subsequenceAStartPos = aIndex;
      matchFound = true;
      break;
    }
  }

  if (!matchFound)
    return false;
  
  for (size_t bIndex=1; bIndex<b.size(); bIndex++)
  {
    aIndex++;
    if (aIndex >= a.size())
      return false;
    
    if (a[aIndex] != b[bIndex])
      return false;
  }
  return true;
}

int permutation(int i, int j, int k)
{
  if ((i==1 && j==2 && k==3) || (i==2 && j==3 && k==1) || (i==3 && j==1 && k==2))
    return 1;
  else if ((i==3 && j==2 && k==1) || (i==1 && j==3 && k==2) || (i==2 && j==1 && k==3))
    return -1;
  return 0;
}

}; // namespace