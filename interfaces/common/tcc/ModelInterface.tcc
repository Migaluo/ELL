////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Learning Library (ELL)
//  File:     ModelInterface.tcc (interfaces)
//  Authors:  Chuck Jacobs, Kirk Olynyk, Lisa Ong
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SWIG

// model
#include "IRMapCompiler.h"

// nodes
#include "SinkNode.h"
#include "SourceNode.h"

namespace ELL_API
{

//
// Map
//
template <typename ElementType>
void Map::SetSourceCallback(ell::api::CallbackBase<ElementType>& callback, size_t index)
{
    auto nodes = GetModel().GetModel().GetNodesByType<ell::nodes::SourceNode<ElementType>>();
    nodes.at(index)->SetSourceFunction([&callback](auto& input)
    {
        return callback.Run(input);
    });
}

template <typename ElementType>
void Map::SetSinkCallback(ell::api::CallbackBase<ElementType>& callback, size_t index)
{
    auto nodes = GetModel().GetModel().GetNodesByType<ell::nodes::SinkNode<ElementType>>();
    nodes.at(index)->SetSinkFunction([&callback](const std::vector<ElementType>& output)
    {
        // Reason for the const_cast:
        // SWIG warns that the const overload gets shadowed, so CallbackBase only
        // provides a non-const vector overload for Run.
        callback.Run(const_cast<std::vector<ElementType>&>(output));
    });
}

template <typename ElementType>
void Map::Step(ell::api::TimeTickType timestamp)
{
    std::vector<ell::api::TimeTickType> input = { timestamp };
    _map->Compute<ElementType>(input);
}

//
// CompiledMap
//
template <typename ElementType>
void CompiledMap::Step(ell::api::TimeTickType timestamp)
{
    // Note: casting TimeTickType to match input and output port types
    std::vector<ell::api::TimeTickType> input = { timestamp };
    _map->Compute<ElementType>(input);
}

template <typename ElementType>
void CompiledMap::RegisterCallbacks(
    ell::api::CallbackBase<ElementType>& inputCallback,
    ell::api::CallbackBase<ElementType>& outputCallback)
{
    ell::api::CallbackBase<ell::api::TimeTickType> unusedLagCallback;
    CallbackForwarder<ElementType>().InitializeOnce(inputCallback, _inputShape.Size(), outputCallback, _outputShape.Size(), unusedLagCallback);
}

template <typename ElementType>
void CompiledMap::UnregisterCallbacks()
{
    CallbackForwarder<ElementType>().Uninitialize();
}

template <typename ElementType>
bool CompiledMap::InvokeSourceCallback(ElementType* input)
{
    return CallbackForwarder<ElementType>().InvokeInput(input);
}

template <typename ElementType>
void CompiledMap::InvokeSinkCallback(ElementType* output)
{
    CallbackForwarder<ElementType>().InvokeOutput(output);
}

} // end namespace

#endif // SWIG