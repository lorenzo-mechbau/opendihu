#include "data_management/finite_element_method/diffusion_tensor_constant.h"

namespace Data
{

template<typename FunctionSpaceType>
DiffusionTensorConstant<FunctionSpaceType>::
DiffusionTensorConstant(PythonConfig settings) :
  DiffusionTensorBase<FunctionSpaceType>::DiffusionTensorBase(settings)
{
  LOG(DEBUG) << "construct Diffusion tensor";
  if (VLOG_IS_ON(1))
  {
    PythonUtility::printDict(settings.pyObject());
  }

  this->diffusionTensor_ = this->parseDiffusionTensor("diffusionTensor");
}

template<typename FunctionSpaceType>
void DiffusionTensorConstant<FunctionSpaceType>::
initialize()
{
  LOG(DEBUG) << "DiffusionTensorConstant::initialize";
}

template<typename FunctionSpaceType>
const MathUtility::Matrix<FunctionSpaceType::dim(),FunctionSpaceType::dim()> &DiffusionTensorConstant<FunctionSpaceType>::
diffusionTensor(element_no_t elementNoLocal, const std::array<double,FunctionSpaceType::dim()> xi) const
{
  return this->diffusionTensor_;
}

}  // namespace
