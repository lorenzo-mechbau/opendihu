#pragma once

#include "utility/type_utility.h"

#include <cstdlib>

/** The functions in this file model a loop over the elements of a tuple, as it occurs as OutputFieldVariablesType in all data_management classes.
 *  (Because the types inside the tuple are static and fixed at compile-time, a simple for loop c not work here.)
 *  The two functions starting with loop recursively emulate the loop. One method is the break condition and does nothing, the other method does the work and calls the method without loop in the name.
 *  OutputFieldVariablesType is assumed to be of type std::tuple<...>> where the types can be (mixed) std::shared_ptr<FieldVariable> or std::vector<std::shared_ptr<FieldVariable>>.
 * 
 *  Call ParaviewWriter::output on the mesh with meshName. This outputs all field variables of the mesh to a paraview readable file.
 */

namespace OutputWriter
{

namespace MegaMolLoopOverTuple
{
 
 /** Static recursive loop from 0 to number of entries in the tuple
 *  Stopping criterion
 */
template<typename OutputFieldVariablesType, typename AllOutputFieldVariablesType, int i=0>
inline typename std::enable_if<i == std::tuple_size<OutputFieldVariablesType>::value, void>::type
loopOutput(const OutputFieldVariablesType &fieldVariables, const AllOutputFieldVariablesType &allFieldVariables, std::string meshName,
           PythonConfig specificSettings
)
{}

 /** Static recursive loop from 0 to number of entries in the tuple
 * Loop body
 */
template<typename OutputFieldVariablesType, typename AllOutputFieldVariablesType, int i=0>
inline typename std::enable_if<i < std::tuple_size<OutputFieldVariablesType>::value, void>::type
loopOutput(const OutputFieldVariablesType &fieldVariables, const AllOutputFieldVariablesType &allFieldVariables, std::string meshName, 
           PythonConfig specificSettings);


/** Loop body for a vector element
 */
template<typename VectorType, typename OutputFieldVariablesType>
typename std::enable_if<TypeUtility::isVector<VectorType>::value, bool>::type
output(VectorType currentFieldVariableVector, const OutputFieldVariablesType &fieldVariables, std::string meshName, 
       PythonConfig specificSettings);

/** Loop body for a tuple element
 */
template<typename VectorType, typename OutputFieldVariablesType>
typename std::enable_if<TypeUtility::isTuple<VectorType>::value, bool>::type
output(VectorType currentFieldVariableVector, const OutputFieldVariablesType &fieldVariables, std::string meshName, 
       PythonConfig specificSettings);

 /**  Loop body for a pointer element
 */
template<typename CurrentFieldVariableType, typename OutputFieldVariablesType>
typename std::enable_if<!TypeUtility::isTuple<CurrentFieldVariableType>::value && !TypeUtility::isVector<CurrentFieldVariableType>::value, bool>::type
output(CurrentFieldVariableType currentFieldVariable, const OutputFieldVariablesType &fieldVariables, std::string meshName, 
       PythonConfig specificSettings);

}  // namespace MegaMolLoopOverTuple

}  // namespace OutputWriter

#include "output_writer/megamol/loop_output.tpp"
