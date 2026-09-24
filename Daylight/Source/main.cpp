#include "pch.h"
#include "Core/Engine.h"

int main()
{
    Dlight::Engine engine;

    if (engine.Initialize())
    {
        engine.Run();
    }

    engine.Shutdown();

    return 0;
}
