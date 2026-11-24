#include "HT.hpp"

#include <iostream>

int main()
{
    try
    {
        std::cout << "Create memory helper functions which move data staged & which create special buffers!" << std::endl;
        HelloTriangle app;
        app.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}