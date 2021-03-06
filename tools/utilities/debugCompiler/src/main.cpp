////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Learning Library (ELL)
//  File:     main.cpp (compare)
//  Authors:  Chris Lovett
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "CompareArguments.h"
#include "LoadImage.h"
#include "ModelComparison.h"

// common
#include "LoadModel.h"
#include "MapCompilerArguments.h"
#include "ModelLoadArguments.h"

// math
#include "Tensor.h"

// utilities
#include "CommandLineParser.h"
#include "Exception.h"
#include "Files.h"
#include "OutputStreamImpostor.h"
#include "RandomEngines.h"
#include "TypeTraits.h"

// stl
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace ell;

template <typename InputType, utilities::IsIntegral<InputType> = true>
std::vector<InputType> GetInputVector(const math::TensorShape& inputShape, float scale)
{
    auto inputSize = inputShape.Size();
    std::vector<InputType> result(inputSize);
    auto engine = utilities::GetRandomEngine("123");
    std::uniform_int_distribution<InputType> dist(0, 255);
    for (auto index = 0; index < inputSize; ++index)
    {
        result[index] = dist(engine) * scale;
    }
    return result;
}

template <typename InputType, utilities::IsFloatingPoint<InputType> = true>
std::vector<InputType> GetInputVector(const math::TensorShape& inputShape, float scale)
{
    auto inputSize = inputShape.Size();
    std::vector<InputType> result(inputSize);
    auto engine = utilities::GetRandomEngine("123");
    std::uniform_real_distribution<InputType> dist(0, std::nextafter(255, std::numeric_limits<InputType>::max()));
    for (size_t index = 0; index < inputSize; ++index)
    {
        result[index] = dist(engine) * scale;
    }
    return result;
}

template <typename InputType>
std::vector<InputType> GetInputImage(std::string filename, const math::TensorShape& inputShape, float inputScale, bool bgr2rgb)
{
    return LoadImage<InputType>(filename, inputShape.NumColumns(), inputShape.NumRows(), inputScale, bgr2rgb ? PixelOrder::RGB : PixelOrder::BGR);
}

template <typename InputType>
std::vector<InputType> GetInputData(model::DynamicMap& map, const CompareArguments& compareArguments)
{
    auto inputShape = map.GetInputShape();
    if (compareArguments.inputTestFile.empty())
    {
        return GetInputVector<InputType>(inputShape, compareArguments.inputScale);
    }
    else
    {
        return GetInputImage<InputType>(compareArguments.inputTestFile, inputShape, compareArguments.inputScale, !compareArguments.bgr);
    }
}

int main(int argc, char* argv[])
{
    using TestDataType = float;
    try
    {
        // create a command line parser
        utilities::CommandLineParser commandLineParser(argc, argv);

        // add arguments to the command line parser
        ParsedCompareArguments compareArguments;
        commandLineParser.AddOptionSet(compareArguments);
        common::ParsedMapCompilerArguments compileArguments;
        commandLineParser.AddDocumentationString("Code generation options");
        commandLineParser.AddOptionSet(compileArguments);
        commandLineParser.Parse();

        if (compareArguments.inputMapFile.empty())
        {
            std::cout << "Model file not specified\n\n";
            std::cout << commandLineParser.GetHelpString() << std::endl;
            return 1;
        }

        if (!ell::utilities::FileExists(compareArguments.inputMapFile))
        {

            std::cout << "Model file not found: " << compareArguments.inputMapFile << std::endl;
            std::cout << commandLineParser.GetHelpString() << std::endl;
            return 1;
        }

        // load map file
        std::cout << "loading map..." << std::endl;
        model::DynamicMap map = common::LoadMap(compareArguments.inputMapFile);

        if (compareArguments.outputDirectory != "")
        {
            ell::utilities::EnsureDirectoryExists(compareArguments.outputDirectory);
        }

        auto input = GetInputData<TestDataType>(map, compareArguments);
        ModelComparison comparison(compareArguments.outputDirectory);

        model::MapCompilerParameters settings = compileArguments.GetMapCompilerParameters("");
        comparison.Compare(input, map, settings);

        // Write summary report
        if (compareArguments.writeReport)
        {
            std::string reportFileName = utilities::JoinPaths(compareArguments.outputDirectory, "report.md");
            std::ofstream reportStream(reportFileName);
            comparison.WriteReport(reportStream, compareArguments.inputMapFile, compareArguments.inputTestFile, compareArguments.writePrediction);
        }

        // Write an annotated graph showing where differences occurred in the model
        // between compiled and reference implementation.
        if (compareArguments.writeGraph)
        {
            std::string graphFileName = utilities::JoinPaths(compareArguments.outputDirectory, "graph.dgml");
            std::ofstream graphStream(graphFileName);
            comparison.SaveDgml(graphStream);

            std::string dotFileName = utilities::JoinPaths(compareArguments.outputDirectory, "graph.dot");
            std::ofstream dotStream(dotFileName);
            comparison.SaveDot(dotStream);
        }
    }
    catch (const utilities::CommandLineParserPrintHelpException& exception)
    {
        std::cout << exception.GetHelpText() << std::endl;
        return 0;
    }
    catch (const utilities::CommandLineParserErrorException& exception)
    {
        std::cerr << "Command line parse error:" << std::endl;
        for (const auto& error : exception.GetParseErrors())
        {
            std::cerr << error.GetMessage() << std::endl;
        }
        return 1;
    }
    catch (utilities::LogicException exception)
    {
        std::cerr << "runtime error: " << exception.GetMessage() << std::endl;
        return 1;
    }

    // the end
    return 0;
}
