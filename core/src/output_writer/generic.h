#pragma once

#include <Python.h>  // has to be the first included header
#include <fstream>

#include "control/types.h"
#include "data_management/data.h"
#include "output_writer/loop_collect_mesh_names.h"

namespace OutputWriter
{

class Generic
{
public:
  //! ctor
  Generic(DihuContext context, PythonConfig specificSettings);

  //! virtual destructor to allow dynamic_pointer_cast
  virtual ~Generic();

  //! open file given by filename, create directory if necessary
  static std::ofstream openFile(std::string filename, bool append=false);

  //! append rank no in the format ".001" to str
  static void appendRankNo(std::stringstream &str, int nRanks, int ownRankNo);

protected:

  //! check if output should be written in this timestep and prepare filename, i.e. set filename_ from config
  template<typename DataType>
  bool prepareWrite(DataType &data, int timeStepNo = -1, double currentTime = 0.0);

  DihuContext context_;         ///< the context object

  std::string filenameBaseWithNo_;   ///< beginning of the file with "_<fileNo>" appended
  std::string filenameBase_;    ///< beginning of the file name for output file
  std::string filename_;        ///< file name with time step number
  std::string formatString_;    ///< the format option, the string as given in config, e.g. "Paraview"
  int writeCallCount_ = 0;      ///< counter of calls to write
  int outputFileNo_ = 0;        ///< counter of calls to write when actually a file was written
  int outputInterval_ = 0;      ///< the interval in which calls to write actually write data


  std::shared_ptr<Partition::RankSubset> rankSubset_; ///< the ranks that collectively call Paraview::write

  int timeStepNo_;              ///< the current time step no.
  double currentTime_;          ///< the current simulation time

  PythonConfig specificSettings_;    ///< the python dict containing settings relevant to this object
};

} // namespace

#include "output_writer/generic.tpp"
