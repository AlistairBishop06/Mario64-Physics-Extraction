#include "replay/Replay.h"

#include <cassert>
#include <filesystem>
#include <iostream>

int main()
{
    sm64ps::replay::ReplayTrack track;
    sm64ps::mario::MarioInput input;
    input.stick = { 1.0f, 0.0f };
    input.jumpPressed = true;

    sm64ps::mario::MarioBody body;
    body.position = { 1.0f, 2.0f, 3.0f };
    body.velocity = { 4.0f, 5.0f, 6.0f };
    body.action = sm64ps::mario::Action::Jump;

    track.record(7, input, body);
    assert(track.sampleAtFrame(7).has_value());
    assert(!track.sampleAtFrame(6).has_value());

    const auto path = std::filesystem::temp_directory_path() / "sm64ps_replay_test.json";
    assert(track.save(path));

    sm64ps::replay::ReplayTrack loaded;
    assert(loaded.load(path));
    assert(loaded.samples().size() == 1);
    assert(loaded.samples().front().frame == 7);
    assert(loaded.samples().front().body.position.x == 1.0f);
    std::filesystem::remove(path);

    std::cout << "replay tests passed\n";
    return 0;
}

