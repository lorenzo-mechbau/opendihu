#include "control/load_balancing/load_balancing.h"

#include <omp.h>
#include <sstream>
#include "data_management/solution_vector_mapping.h"

namespace Control
{

template<class TimeSteppingScheme>
LoadBalancing<TimeSteppingScheme>::
LoadBalancing(DihuContext context) :
  Runnable(), Splitable(), context_(context["LoadBalancing"]), timeSteppingScheme_(context_)
{
  // initialize the python object "specificSettings" which holds the python dict under "LoadBalancing"
  specificSettings_ = context_.getPythonConfig();
}

template<class TimeSteppingScheme>
void LoadBalancing<TimeSteppingScheme>::
advanceTimeSpan()
{
  // This method advances the simulation by the specified time span
  timeSteppingScheme_.advanceTimeSpan();
}

template<class TimeSteppingScheme>
void LoadBalancing<TimeSteppingScheme>::
setTimeSpan(double startTime, double endTime)
{
  timeSteppingScheme_.setTimeSpan(startTime, endTime);
}

template<class TimeSteppingScheme>
void LoadBalancing<TimeSteppingScheme>::
initialize()
{
  LOG(TRACE) << "LoadBalancing::initialize()";

  timeSteppingScheme_.initialize();
}

template<class TimeSteppingScheme>
void LoadBalancing<TimeSteppingScheme>::
run()
{
  timeSteppingScheme_.run();
}

template<class TimeSteppingScheme>
void LoadBalancing<TimeSteppingScheme>::
reset()
{
  timeSteppingScheme_.reset();
}

template<class TimeSteppingScheme>
bool LoadBalancing<TimeSteppingScheme>::
knowsMeshType()
{
  return timeSteppingScheme_.knowsMeshType();
}

template<class TimeSteppingScheme>
typename LoadBalancing<TimeSteppingScheme>::Data &LoadBalancing<TimeSteppingScheme>::
data()
{
  return timeSteppingScheme_.data();
}
/*
template<class TimeSteppingScheme>
SolutionVectorMapping &LoadBalancing<TimeSteppingScheme>::
solutionVectorMapping()
{
  return timeSteppingScheme_.solutionVectorMapping();
}*/

template<class TimeSteppingScheme>
std::shared_ptr<typename LoadBalancing<TimeSteppingScheme>::Data::FieldVariableType> LoadBalancing<TimeSteppingScheme>::
solution()
{
  return timeSteppingScheme_.solution();
}

};  // namespace
