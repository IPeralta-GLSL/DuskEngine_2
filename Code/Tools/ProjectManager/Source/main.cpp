#include <AzQtComponents/Utilities/QtPluginPaths.h>
#include <Application.h>

int main(int argc, char* argv[])
{
    const AZ::Debug::Trace tracer;

    AzQtComponents::PrepareQtPaths();

    O3DE::ProjectManager::Application application(&argc, &argv);

    if (!application.Init())
    {
        AZ_Error("ProjectManager", false, "Failed to initialize");
        return 1;
    }

    return application.Run() ? 0 : 1;
}
