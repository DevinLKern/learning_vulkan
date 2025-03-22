#include <sandbox/application.h>

int main()
{
    Application application = Application_Create();
    if (application.component_count == 0)
    {
        return 1;
    }

    Application_Run(&application);

    Application_Cleanup(&application);

    return 0;
}