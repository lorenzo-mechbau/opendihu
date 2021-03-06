#include "control/dihu_context.h"

#include <Python.h>  // this has to be the first included header
#include <python_home.h>  // defines PYTHON_HOME_DIRECTORY
#include "control/performance_measurement.h"

void DihuContext::initializePython(int argc, char *argv[], bool explicitConfigFileGiven)
{
  LOG(TRACE) << "initialize python";

  // set program name of python script
  char const *programName = "opendihu";

  wchar_t *programNameWChar = Py_DecodeLocale(programName, NULL);
  Py_SetProgramName(programNameWChar);  /* optional but recommended */

  // set python home and path, apparently this is not needed
  VLOG(1) << "python home directory: \"" << PYTHON_HOME_DIRECTORY << "\"";
  std::string pythonSearchPath = PYTHON_HOME_DIRECTORY;
  //std::string pythonSearchPath = std::string("/store/software/opendihu/dependencies/python/install");
  const wchar_t *pythonSearchPathWChar = Py_DecodeLocale(pythonSearchPath.c_str(), NULL);
  Py_SetPythonHome((wchar_t *)pythonSearchPathWChar);

  // initialize python
  Py_Initialize();

  PyEval_InitThreads();

  //VLOG(4) << "PyEval_ReleaseLock()";
  //PyEval_ReleaseLock();

  Py_SetStandardStreamEncoding(NULL, NULL);

  // get standard python path
  wchar_t *standardPythonPathWChar = Py_GetPath();
  std::wstring standardPythonPath(standardPythonPathWChar);

  VLOG(1) << "standard python path: " << standardPythonPath;

  // set python path
  std::stringstream pythonPath;
  //pythonPath << ".:" << PYTHON_HOME_DIRECTORY << "/lib/python3.6:" << PYTHON_HOME_DIRECTORY << "/lib/python3.6/site-packages:"
  //pythonPath << OPENDIHU_HOME << "/scripts:" << OPENDIHU_HOME << "/scripts/geometry_manipulation";
  //VLOG(1) << "python path: " << pythonPath.str();
  //const wchar_t *pythonPathWChar = Py_DecodeLocale(pythonPath.str().c_str(), NULL);
  //Py_SetPath((wchar_t *)pythonPathWChar);


  // pass on command line arguments to python config script

  // determine if the first argument (argv[1]) is *.py, then it is also discarded
  // always remove the first argument, which is the name of the executable
  int numberArgumentsToRemove = (explicitConfigFileGiven? 2: 1);

  // add the own rank no and the number of ranks at the end as command line arguments

  int nArgumentsToConfig = argc - numberArgumentsToRemove + 2;
  VLOG(4) << "nArgumentsToConfig: " << nArgumentsToConfig << ", numberArgumentsToRemove: " << numberArgumentsToRemove;

  char **argvReduced = new char *[nArgumentsToConfig];
  wchar_t **argumentToConfigWChar = new wchar_t *[nArgumentsToConfig];

  // set given command line arguments
  for (int i=0; i<nArgumentsToConfig-2; i++)
  {
    argvReduced[i] = argv[i+numberArgumentsToRemove];
    argumentToConfigWChar[i] = Py_DecodeLocale(argvReduced[i], NULL);
  }

  // add rank no and nRanks
  // get own rank no and number of ranks
  int rankNo;
  MPIUtility::handleReturnValue (MPI_Comm_rank(MPI_COMM_WORLD, &rankNo));

  Control::PerformanceMeasurement::setParameter("rankNo", rankNo);
  Control::PerformanceMeasurement::setParameter("nRanks", nRanksCommWorld_);

  // convert to wchar_t
  std::stringstream rankNoStr, nRanksStr;
  rankNoStr << rankNo;
  nRanksStr << nRanksCommWorld_;
  argumentToConfigWChar[nArgumentsToConfig-2] = Py_DecodeLocale(rankNoStr.str().c_str(), NULL);
  argumentToConfigWChar[nArgumentsToConfig-1] = Py_DecodeLocale(nRanksStr.str().c_str(), NULL);

  if (VLOG_IS_ON(1) && pythonConfig_.pyObject())
  {
    PythonUtility::printDict(pythonConfig_.pyObject());
  }

  // pass reduced list of command line arguments to python script
  PySys_SetArgvEx(nArgumentsToConfig, argumentToConfigWChar, 0);

  // check different python setting for debugging
  wchar_t *homeWChar = Py_GetPythonHome();
  char *home = Py_EncodeLocale(homeWChar, NULL);
  VLOG(2) << "python home: " << home;

  wchar_t *pathWChar = Py_GetPath();
  char *path = Py_EncodeLocale(pathWChar, NULL);
  VLOG(2) << "python path: " << path;

  wchar_t *prefixWChar = Py_GetPrefix();
  char *prefix = Py_EncodeLocale(prefixWChar, NULL);
  VLOG(2) << "python prefix: " << prefix;

  wchar_t *execPrefixWChar = Py_GetExecPrefix();
  char *execPrefix = Py_EncodeLocale(execPrefixWChar, NULL);
  VLOG(2) << "python execPrefix: " << execPrefix;

  wchar_t *programFullPathWChar = Py_GetProgramFullPath();
  char *programFullPath = Py_EncodeLocale(programFullPathWChar, NULL);
  VLOG(2) << "python programFullPath: " << programFullPath;

  const char *version = Py_GetVersion();
  VLOG(2) << "python version: " << version;

  const char *platform = Py_GetPlatform();
  VLOG(2) << "python platform: " << platform;

  const char *compiler = Py_GetCompiler();
  VLOG(2) << "python compiler: " << compiler;

  const char *buildInfo = Py_GetBuildInfo();
  VLOG(2) << "python buildInfo: " << buildInfo;

}

void DihuContext::loadPythonScriptFromFile(std::string filename)
{
  // initialize python interpreter

  std::ifstream file(filename);
  if (!file.is_open())
  {
    LOG(FATAL) << "Could not open settings file \"" <<filename << "\".";
  }
  else
  {
    // reserve memory of size of file
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    std::string fileContents(fileSize, ' ');

    // reset file pointer
    file.seekg(0, std::ios::beg);

    // read in file contents
    file.read(&fileContents[0], fileSize);

    LOG(INFO) << "File \"" <<filename << "\" loaded.";

    loadPythonScript(fileContents);
  }
}

void DihuContext::loadPythonScript(std::string text)
{
  LOG(TRACE) << "loadPythonScript(" << text.substr(0,std::min(std::size_t(80),text.length())) << ")";

  // execute python code
  int ret = 0;
  LOG(INFO) << std::string(80, '-');
  try
  {
    // check if numpy module could be loaded
    PyObject *numpyModule = PyImport_ImportModule("numpy");
    if (numpyModule == NULL)
    {
      LOG(ERROR) << "Failed to import numpy.";
    }

    // execute config script
    ret = PyRun_SimpleString(text.c_str());

    PythonUtility::checkForError();
  }
  catch(...)
  {
  }
  LOG(INFO) << std::string(80, '-');

  // if there was an error in the python code
  if (ret != 0)
  {
    if (PyErr_Occurred())
    {
      // print error message and exit
      PyErr_Print();
      LOG(FATAL) << "An error occured in the python config.";
    }

    PyErr_Print();
    LOG(FATAL) << "An error occured in the python config.";
  }

  // load main module and extract config
  PyObject *mainModule = PyImport_AddModule("__main__");
  PyObject *config = PyObject_GetAttrString(mainModule, "config");
  VLOG(4) << "create pythonConfig_ (initialize ref to 1)";


  // check if type is valid
  if (config == NULL || !PyDict_Check(config))
  {
    LOG(FATAL) << "Python config file does not contain a dict named \"config\".";
  }

  pythonConfig_.setPyObject(config);

  // parse scenario name
  std::string scenarioName = "";
  if (pythonConfig_.hasKey("scenarioName"))
  {
    scenarioName = pythonConfig_.getOptionString("scenarioName", "");
  }
  Control::PerformanceMeasurement::setParameter("scenarioName", scenarioName);
}
