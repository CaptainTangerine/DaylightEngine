#include "pch.h"
#include "Application.h"

using namespace Dlight;

int main()
{
    Application App;

    if (App.Initialize())
    {
        App.Run();
    }

    App.Shutdown();

    return 0;
}
