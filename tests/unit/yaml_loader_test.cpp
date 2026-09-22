#include "emberx/runtime/config/config.h"
#include "tests/support/checks.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

int main(int argc, char **argv)
{
    Checks checks;
    if (argc != 3)
    {
        std::cerr << "Expected valid and invalid fixture paths\n";
        return EXIT_FAILURE;
    }
    const std::filesystem::path sample = argv[1];
    auto loaded = emberx::load_config(sample);
    const auto *config = std::get_if<emberx::SystemConfig>(&loaded);
    checks.expect(config && config->devices.size() == 1, "load valid file");
    if (config && !config->devices.empty())
    {
        checks.expect(config->devices[0].memory.sram_bytes == 1'048'576,
                      "parsed SRAM capacity");
        checks.expect(config->devices[0].memory.allocation_alignment_bytes == 64,
                      "parsed alignment");
    }

    std::ifstream input(sample);
    std::ostringstream stream;
    stream << input.rdbuf();
    const auto original = stream.str();
    auto replace = [&](std::string_view from, std::string_view to)
    {
        auto text = original;
        const auto pos = text.find(from);
        if (pos == std::string::npos)
            throw std::runtime_error("test fixture text was not found");
        text.replace(pos, from.size(), to);
        return text;
    };
    auto rejects = [&](const std::string &text, EmberxStatus code,
                       std::string_view field)
    {
        auto result = emberx::load_config_text(text);
        const auto *error = std::get_if<emberx::Error>(&result);
        checks.expect(error && error->code == code && error->field == field, field);
    };
    const auto invalid = EMBERX_INVALID_CONFIGURATION;
    rejects("devices: [", EMBERX_YAML_SYNTAX_ERROR, "$");
    rejects(original + "\n---\n" + original, invalid, "$");
    rejects("[]", invalid, "$");
    rejects(replace("schema_version: 1", ""), invalid, "schema_version");
    rejects(replace("schema_version: 1", "schema_version: 2"), invalid, "schema_version");
    rejects(original + "\nextra: 7\n", invalid, "extra");
    rejects(original + "\nschema_version: 1\n", invalid, "schema_version");
    rejects(replace("matrix: 1", "matrix: 1\n      matrix: 1"),
            invalid, "devices[0].engines.matrix");
    rejects(replace("matrix: 1", "matrix: null"), invalid, "devices[0].engines.matrix");
    rejects(replace("matrix: 1", "matrix: 2"), invalid, "devices[0].engines.matrix");
    rejects(replace("matrix: 1", "matrix: 4294967296"),
            invalid, "devices[0].engines.matrix");
    rejects(replace("float32", "float16"), invalid, "devices[0].supported_dtypes[0]");
    rejects(replace("[0, 0]", "[0]"), invalid, "devices[0].topology.coordinates");
    rejects(replace("links: []", "links: [{}]"), invalid, "fabric.links");

    for (std::string_view value : {"-1", "1.5", "1e6", "true", "'1048576'",
                                   "\"1048576\"", "!!int 1048576",
                                   "18446744073709551616"})
    {
        rejects(replace("sram_bytes: 1048576", "sram_bytes: " + std::string(value)),
                invalid, "devices[0].memory.sram_bytes");
    }

    auto bad_file = emberx::load_config(argv[2]);
    const auto *bad_error = std::get_if<emberx::Error>(&bad_file);
    checks.expect(bad_error && bad_error->code == invalid &&
                      bad_error->field == "devices[0].memory.allocation_alignment_bytes",
                  "invalid file reports field path");

    auto missing = emberx::load_config(sample.parent_path() / "missing-m0-fixture.yaml");
    const auto *missing_error = std::get_if<emberx::Error>(&missing);
    checks.expect(missing_error && missing_error->code == EMBERX_IO_ERROR,
                  "missing file is an IO error");
    return checks.result();
}