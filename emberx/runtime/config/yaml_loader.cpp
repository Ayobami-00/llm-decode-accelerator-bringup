#include "emberx/runtime/config/config.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <charconv>
#include <fstream>
#include <initializer_list>
#include <set>
#include <sstream>
#include <system_error>

namespace emberx
{
    namespace
    {

        [[noreturn]] void invalid(std::string field, std::string message)
        {
            // Private parsing helpers use this to stop at the first error.
            throw Error{EMBERX_INVALID_CONFIGURATION, field, message};
        }

        std::string child(const std::string &parent, const std::string &key)
        {
            return parent.empty() ? key : parent + "." + key;
        }

        void mapping(const YAML::Node &node, const std::string &path,
                     std::initializer_list<std::string_view> allowed)
        {
            if (!node.IsMap())
                invalid(path.empty() ? "$" : path, "expected a mapping");

            std::set<std::string> seen;
            for (const auto &entry : node)
            {
                if (!entry.first.IsScalar())
                    invalid(path.empty() ? "$" : path, "mapping keys must be strings");
                const auto key = entry.first.Scalar();
                if (!seen.insert(key).second)
                    invalid(child(path, key), "duplicate field");
                if (std::find(allowed.begin(), allowed.end(), key) == allowed.end())
                    invalid(child(path, key), "unknown field");
            }
            for (const auto key : allowed)
            {
                if (!seen.contains(std::string(key)))
                    invalid(child(path, std::string(key)), "missing required field");
            }
        }

        void sequence(const YAML::Node &node, const std::string &path)
        {
            if (!node.IsSequence())
                invalid(path, "expected a sequence");
        }

        std::string scalar(const YAML::Node &node, const std::string &path)
        {
            if (!node.IsScalar())
                invalid(path, "expected a scalar");
            return node.Scalar();
        }

        template <typename UInt>
        UInt unsigned_integer(const YAML::Node &node, const std::string &path)
        {
            const auto text = scalar(node, path);
            // In the pinned yaml-cpp version, "?" identifies an untagged plain scalar.
            if (node.Tag() != "?" || text.empty() ||
                !std::all_of(text.begin(), text.end(),
                             [](char c)
                             { return c >= '0' && c <= '9'; }))
                invalid(path, "expected an unquoted, untagged nonnegative decimal integer");

            UInt value{};
            const auto parsed = std::from_chars(
                text.data(), text.data() + text.size(), value, 10);
            if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size())
                invalid(path, "integer is outside the target type's range");
            return value;
        }

    } // namespace

    Result<SystemConfig> load_config_text(std::string_view text)
    {
        try
        {
            const auto documents = YAML::LoadAll(std::string(text));
            if (documents.size() != 1)
                invalid("$", "expected exactly one YAML document");
            const auto &root = documents.front();
            mapping(root, "", {"schema_version", "architecture", "devices", "fabric"});

            SystemConfig config{};
            config.schema_version =
                unsigned_integer<std::uint32_t>(root["schema_version"], "schema_version");
            config.architecture = scalar(root["architecture"], "architecture");

            const auto devices = root["devices"];
            sequence(devices, "devices");
            if (devices.size() == 0 || devices.size() > kMaxDevices)
                invalid("devices", "expected between 1 and 256 devices");
            config.devices.reserve(devices.size());

            for (std::size_t i = 0; i < devices.size(); ++i)
            {
                const auto node = devices[i];
                const std::string path = "devices[" + std::to_string(i) + "]";
                mapping(node, path, {"id", "memory", "supported_dtypes", "engines", "topology"});
                DeviceConfig device{};
                device.id = unsigned_integer<EmberxDeviceId>(node["id"], path + ".id");

                const auto memory = node["memory"];
                mapping(memory, path + ".memory",
                        {"sram_bytes", "allocation_alignment_bytes"});
                device.memory.sram_bytes = unsigned_integer<std::uint64_t>(
                    memory["sram_bytes"], path + ".memory.sram_bytes");
                device.memory.allocation_alignment_bytes = unsigned_integer<std::uint64_t>(
                    memory["allocation_alignment_bytes"],
                    path + ".memory.allocation_alignment_bytes");

                const auto dtypes = node["supported_dtypes"];
                sequence(dtypes, path + ".supported_dtypes");
                if (dtypes.size() != 1)
                    invalid(path + ".supported_dtypes", "expected only float32");
                if (scalar(dtypes[0], path + ".supported_dtypes[0]") != "float32")
                    invalid(path + ".supported_dtypes[0]", "unsupported dtype");
                device.supported_dtypes.push_back(EMBERX_DTYPE_FLOAT32);

                const auto engines = node["engines"];
                mapping(engines, path + ".engines", {"matrix", "vector", "dma"});
                device.engines.matrix = unsigned_integer<std::uint32_t>(
                    engines["matrix"], path + ".engines.matrix");
                device.engines.vector = unsigned_integer<std::uint32_t>(
                    engines["vector"], path + ".engines.vector");
                device.engines.dma = unsigned_integer<std::uint32_t>(
                    engines["dma"], path + ".engines.dma");

                const auto topology = node["topology"];
                mapping(topology, path + ".topology", {"coordinates"});
                const auto coordinates = topology["coordinates"];
                sequence(coordinates, path + ".topology.coordinates");
                if (coordinates.size() != 2)
                    invalid(path + ".topology.coordinates", "expected [x, y]");
                for (std::size_t axis = 0; axis < 2; ++axis)
                    device.topology.coordinates[axis] = unsigned_integer<std::uint32_t>(
                        coordinates[axis],
                        path + ".topology.coordinates[" + std::to_string(axis) + "]");
                config.devices.push_back(device);
            }

            const auto fabric = root["fabric"];
            mapping(fabric, "fabric", {"links"});
            sequence(fabric["links"], "fabric.links");
            if (fabric["links"].size() != 0)
                invalid("fabric.links", "nonempty fabric links are unsupported");

            if (const auto error = validate_config(config))
                return *error;
            return config;
        }
        catch (const Error &error)
        {
            return error;
        }
        catch (const YAML::Exception &error)
        {
            return Error{EMBERX_YAML_SYNTAX_ERROR, "$", error.what()};
        }
    }

    Result<SystemConfig> load_config(const std::filesystem::path &path)
    {
        std::ifstream input(path);
        if (!input)
            return Error{EMBERX_IO_ERROR, path.string(), "could not open configuration"};
        std::ostringstream contents;
        contents << input.rdbuf();
        if (input.bad() || contents.bad())
            return Error{EMBERX_IO_ERROR, path.string(), "could not read configuration"};
        return load_config_text(contents.str());
    }

} // namespace emberx