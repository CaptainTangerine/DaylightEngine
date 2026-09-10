#include "pch.h"
#include "Application.h"

int main()
{
    Dlight::Application App;

    if (App.Initialize())
    {
        App.Run();
    }

    App.Shutdown();

    return 0;
}
