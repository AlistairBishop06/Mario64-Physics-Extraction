#include "SandboxApp.h"

#include "util/Log.h"

int main(int, char**)
{
    sm64ps::SandboxApp app;
    if (!app.initialize()) {
        return 1;
    }

    const int result = app.run();
    app.shutdown();
    sm64ps::util::logInfo("shutdown complete");
    return result;
}

