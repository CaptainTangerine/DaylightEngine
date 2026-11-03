#include "pch.h"
#include "Editor/EditorApplication.h"

int main()
{
    try 
    {
        Dlight::EditorApplication application;
        application.Run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
