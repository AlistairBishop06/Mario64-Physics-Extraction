#include "assets/RomExtractor.h"

#include "debug/TweakVars.h"
#include "util/Log.h"
#include "util/Sha1.h"

#include <fstream>

#include <nlohmann/json.hpp>

namespace sm64ps::assets {

RomVerification RomExtractor::verify(const std::filesystem::path& romPath) const
{
    RomVerification verification;
    const auto sha1 = util::Sha1::fileHexDigest(romPath);
    if (!sha1) {
        return verification;
    }

    verification.readable = true;
    verification.sha1 = *sha1;
    for (const KnownRom& known : knownRoms_) {
        if (known.sha1 == *sha1) {
            verification.recognized = true;
            verification.knownRom = known;
            return verification;
        }
    }

    return verification;
}

bool RomExtractor::extractMovementData(const std::filesystem::path& romPath, const std::filesystem::path& outputDirectory) const
{
    const RomVerification verification = verify(romPath);
    if (!verification.readable) {
        util::logError("ROM is not readable: ", romPath.string());
        return false;
    }

    if (!verification.recognized || !verification.knownRom || !verification.knownRom->supported) {
        util::logError("ROM hash is not recognized as a supported retail SM64 ROM. SHA-1: ", verification.sha1);
        return false;
    }

    std::filesystem::create_directories(outputDirectory);

    nlohmann::json metadata;
    metadata["source_rom_sha1"] = verification.sha1;
    metadata["source_rom_label"] = verification.knownRom->label;
    metadata["source_rom_region"] = verification.knownRom->region;
    metadata["redistribution_allowed"] = false;
    metadata["note"] = "Generated locally from a user-supplied ROM. Do not commit this output.";

    {
        std::ofstream file(outputDirectory / "rom_metadata.json");
        file << metadata.dump(2) << '\n';
    }

    nlohmann::json movementSeed;
    movementSeed["schema"] = "sm64ps.movement_seed.v1";
    movementSeed["source_rom_sha1"] = verification.sha1;
    movementSeed["tweak_vars"] = debug::makeDefaultMovementTweaks().toJson();
    movementSeed["extraction_status"] = "starter pipeline: verified ROM and emitted editable movement seed config";

    {
        std::ofstream file(outputDirectory / "movement_tuning_seed.json");
        file << movementSeed.dump(2) << '\n';
    }

    util::logInfo("Wrote local extraction metadata to ", outputDirectory.string());
    return true;
}

} // namespace sm64ps::assets

