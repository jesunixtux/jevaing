#include <Jevaing/Jevaing.h>

#include "Version.h"

namespace Jevaing
{
    const char* GetVersion()
    {
        return Internal::VersionString;
    }

    const char* GetCodename()
    {
        return Internal::Codename;
    }
}
