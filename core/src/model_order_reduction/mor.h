#pragma once

#include <petscmat.h>

#include "control/dihu_context.h"
#include "data_management/data.h"
#include "data_management/model_order_reduction.h"

namespace ModelOrderReduction
{

/** A class for model order reduction techniques.
 */
template<typename FullFunctionSpace>
class MORBase
{
public:
  typedef Data::ModelOrderReduction<FullFunctionSpace> Data; //type of Data object
  
  //! constructor
  MORBase(DihuContext context);
  
  virtual ~MORBase();
  
  //! Set the basis V as Petsc Mat
  void setBasis();
  
  //! data object for model order reduction
  Data &data();
  
  virtual void initialize();
   
protected:
  //! Set the reduced system matrix, A_R=V^T A V
  virtual void setRedSysMatrix(Mat &A, Mat &A_R);
  
  std::shared_ptr<Data> data_;
  int nReducedBases_;             ///< dimension of the reduced space
  bool initialized_;
};

}  // namespace


#include "model_order_reduction/mor.tpp"
