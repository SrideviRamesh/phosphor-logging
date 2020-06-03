/**
 * Copyright © 2020 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "device_callouts.hpp"

#include "paths.hpp"

#include <fstream>
#include <phosphor-logging/log.hpp>
#include <regex>

namespace openpower::pels::device_callouts
{

constexpr auto debugFilePath = "/etc/phosphor-logging/";
constexpr auto calloutFileSuffix = "_dev_callouts.json";

namespace fs = std::filesystem;
using namespace phosphor::logging;

namespace util
{

fs::path getJSONFilename(const std::vector<std::string>& compatibleList)
{
    auto basePath = getPELReadOnlyDataPath();
    fs::path fullPath;

    // Find an entry in the list of compatible system names that
    // matches a filename we have.

    for (const auto& name : compatibleList)
    {
        fs::path filename = name + calloutFileSuffix;

        // Check the debug path first
        fs::path path{fs::path{debugFilePath} / filename};

        if (fs::exists(path))
        {
            log<level::INFO>("Found device callout debug file");
            fullPath = path;
            break;
        }

        path = basePath / filename;

        if (fs::exists(path))
        {
            fullPath = path;
            break;
        }
    }

    if (fullPath.empty())
    {
        throw std::invalid_argument(
            "No JSON dev path callout file for this system");
    }

    return fullPath;
}

/**
 * @brief Reads the callout JSON into an object based on the
 *        compatible system names list.
 *
 * @param[in] compatibleList - The list of compatible names for this
 *                             system.
 *
 * @return nlohmann::json - The JSON object
 */
nlohmann::json loadJSON(const std::vector<std::string>& compatibleList)
{
    auto filename = getJSONFilename(compatibleList);
    std::ifstream file{filename};
    return nlohmann::json::parse(file);
}

std::vector<device_callouts::Callout>
    calloutI2C(size_t i2cBus, uint8_t i2cAddress,
               const nlohmann::json& calloutJSON)
{
    // TODO
    return {};
}

std::vector<device_callouts::Callout> findCallouts(const std::string& devPath,
                                                   const nlohmann::json& json)
{
    std::vector<Callout> callouts;
    fs::path path;

    // Gives the /sys/devices/platform/ path
    try
    {
        path = fs::canonical(devPath);
    }
    catch (const fs::filesystem_error& e)
    {
        // Path not there, still try to do the callout
        path = devPath;
    }

    switch (util::getCalloutType(path))
    {
        case util::CalloutType::i2c:
            // callouts = calloutI2CUsingPath(errnoValue, path, json);
            break;
        case util::CalloutType::fsi:
            // callouts = calloutFSI(errnoValue, path, json);
            break;
        case util::CalloutType::fsii2c:
            // callouts = calloutFSII2C(errnoValue, path, json);
            break;
        case util::CalloutType::fsispi:
            // callouts = calloutFSISPI(errnoValue, path, json);
            break;
        default:
            std::string msg =
                "Could not get callout type from device path: " + path.string();
            throw std::invalid_argument{msg.c_str()};
            break;
    }

    return callouts;
}

CalloutType getCalloutType(const std::string& devPath)
{
    if ((devPath.find("fsi-master") != std::string::npos) &&
        (devPath.find("i2c-") != std::string::npos))
    {
        return CalloutType::fsii2c;
    }

    if ((devPath.find("fsi-master") != std::string::npos) &&
        (devPath.find("spi") != std::string::npos))
    {
        return CalloutType::fsispi;
    }

    // Treat anything else FSI related as plain FSI
    if (devPath.find("fsi-master") != std::string::npos)
    {
        return CalloutType::fsi;
    }

    if (devPath.find("i2c-bus/i2c-") != std::string::npos)
    {
        return CalloutType::i2c;
    }

    return CalloutType::unknown;
}

} // namespace util

std::vector<Callout> getCallouts(const std::string& devPath,
                                 const std::vector<std::string>& compatibleList)
{
    auto json = util::loadJSON(compatibleList);
    return util::findCallouts(devPath, json);
}

std::vector<Callout>
    getI2CCallouts(size_t i2cBus, uint8_t i2cAddress,
                   const std::vector<std::string>& compatibleList)
{
    auto json = util::loadJSON(compatibleList);
    return util::calloutI2C(i2cBus, i2cAddress, json);
}

} // namespace openpower::pels::device_callouts
