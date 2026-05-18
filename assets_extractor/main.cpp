#include "assets/RomExtractor.h"
#include "util/Log.h"

#include <filesystem>
#include <iostream>

int main(int argc, char** argv)
{
    if (argc != 3) {
        std::cerr << "usage: sm64ps_asset_extractor <path-to-local-rom> <output-directory>\n";
        std::cerr << "The output is local generated data only. Do not redistribute extracted assets.\n";
        return 2;
    }

    const std::filesystem::path romPath = argv[1];
    const std::filesystem::path outputDirectory = argv[2];

    sm64ps::assets::RomExtractor extractor;
    const auto verification = extractor.verify(romPath);
    if (!verification.readable) {
        sm64ps::util::logError("Unable to read ROM: ", romPath.string());
        return 1;
    }

    sm64ps::util::logInfo("ROM SHA-1: ", verification.sha1);
    if (verification.knownRom) {
        sm64ps::util::logInfo("Recognized: ", verification.knownRom->label);
    }

    return extractor.extractMovementData(romPath, outputDirectory) ? 0 : 1;
}

