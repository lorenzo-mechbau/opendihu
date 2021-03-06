#pragma once

#include <Python.h>  // has to be the first included header
#include <map>

#include "control/dihu_context.h"
#include "function_space/function_space.h"
#include "mesh/mapping_between_meshes.h"

namespace Partition{
class Manager;
}
namespace Mesh
{
class NodePositionsTester;

/**
 * This class creates and stores all used meshes.
 * Each mesh can be defined in the python config under "Meshes" with a name and other properties.
 * Various components of the program can later
 * request their mesh by a call to mesh(name).
 * If a mesh was not defined earlier, it is created on the fly when it is requested.
 */
class Manager
{
public:
  //! constructor
  Manager(PythonConfig specificSettings);

  //! store the pointer to the partition manager
  void setPartitionManager(std::shared_ptr<Partition::Manager> partitionManager);
  
  //! check if a MappingBetweenMeshes object need to be created and initialized
  template<typename FunctionSpace1Type, typename FunctionSpace2Type>
  void initializeMappingsBetweenMeshes(const std::shared_ptr<FunctionSpace1Type> functionSpace1, const std::shared_ptr<FunctionSpace2Type> functionSpace2);

  //! return previously created mesh or create on the fly, already call functionSpace->initialize()
  template<typename FunctionSpaceType=FunctionSpace::Generic>
  std::shared_ptr<FunctionSpaceType> functionSpace(PythonConfig settings);

  //! check if a function space with the given name and type is stored
  template<typename FunctionSpaceType>
  bool hasFunctionSpaceOfType(std::string meshName);

  //! check if a function space with the specified name is stored, the type is not checked
  bool hasFunctionSpace(std::string meshName);

  //! create a mesh not from python config but directly by calling an appropriate construtor.
  //! With this e.g. meshes from node positions can be created.
  //! Initialize the functionSpace from settings.
  template<typename FunctionSpaceType, typename ...Args>
  std::shared_ptr<FunctionSpaceType> createFunctionSpace(std::string name, Args && ...args);

  //! create a mesh not from python config but directly by calling an appropriate construtor.
  //! With this e.g. meshes from node positions can be created.
  //! Use the given meshPartition, i.e. not the number of elements from settings.
  template<typename FunctionSpaceType, typename ...Args>
  std::shared_ptr<FunctionSpaceType> createFunctionSpaceWithGivenMeshPartition(
    std::string name, std::shared_ptr<Partition::MeshPartition<FunctionSpaceType>> meshPartition, Args && ...args);

  //! Create a field variable without logical mesh representation, e.g. for MOR reduced vectors.
  //! The vector contains nEntries entries, the partitioning is done by the partition manager.
  //! \param name is the name of the Petsc Vec, used for debugging output.
  std::shared_ptr<FieldVariable::FieldVariable<FunctionSpace::Generic,1>> createGenericFieldVariable(int nEntries, std::string name);

  friend class NodePositionsTester;    ///< a class used for testing

private:
  //! store settings for all meshes that are specified in specificSettings_
  void storePreconfiguredMeshes();

  //! create MappingBetweenMeshes objects from the config and store them under mappingsBetweenMeshes_
  void storeMappingsBetweenMeshes();

  //! indicate that on the mesh with name, "initialize()" has been called and now check if mappingsBetweenMeshes_ can be initialized
  void checkInitializeMappingBetweenMeshes(std::string name);

  std::shared_ptr<Partition::Manager> partitionManager_;  ///< the partition manager object
  
  PythonConfig specificSettings_;    ///< python object containing the value of the python config dict with corresponding key, for meshManager
  int numberAnonymousMeshes_;     ///< how many meshes without a given name in the python config are contained in meshes_. These have a key "anonymous<no>"

  std::map<std::string, PythonConfig> meshConfiguration_;         ///< the python dicts for the meshes that were defined under "Meshes"
  std::map<std::string, std::shared_ptr<Mesh>> functionSpaces_;    ///< the managed function spaces with their string key
  std::map<std::string, std::map<std::string, std::shared_ptr<MappingBetweenMeshesBase>>> mappingsBetweenMeshes_;  ///<["key mesh from"]["key mesh to"] mapping between meshes
};

}  // namespace

#include "mesh/mesh_manager.tpp"
