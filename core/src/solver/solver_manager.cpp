#include "solver/solver_manager.h"

namespace Solver
{
  
Manager::Manager(const DihuContext &context) : 
  context_(context), numberAnonymousSolvers_(0)
{
  specificSettings_ = this->context_.getPythonConfig();
  storePreconfiguredSolvers();
}
  
bool Manager::hasSolver(std::string solverName)
{
  return solvers_.find(solverName) != solvers_.end();
}
  
void Manager::storePreconfiguredSolvers()
{
  LOG(TRACE) << "SolverManager::storePreconfiguredSolvers";
  if (specificSettings_)
  {
    std::string keyString("Solvers");
    std::pair<std::string, PyObject *> dictItem 
      = PythonUtility::getOptionDictBegin<std::string, PyObject *>(specificSettings_, keyString);
    
    for (; !PythonUtility::getOptionDictEnd(specificSettings_, keyString); 
        PythonUtility::getOptionDictNext<std::string, PyObject *>(specificSettings_, keyString, dictItem))
    {
      std::string key = dictItem.first;
      PyObject *value = dictItem.second;
          
      if (value == NULL)
      {
        LOG(WARNING) << "Could not extract dict for solver \""<<key<<"\".";
      }
      else if(!PyDict_Check(value))
      {
        LOG(WARNING) << "Value for solver with name \""<<key<<"\" should be a dict.";
      }
      else
      {
        LOG(DEBUG) << "store solver configuration with key \""<<key<<"\".";
        solverConfiguration_[key] = value;
      }
    }
  }
}
  
};   // namespace