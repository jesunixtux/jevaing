#pragma once

namespace Jevaing
{
    // Devuelve la versión actual del motor.
    const char* GetVersion();

    // Devuelve el nombre clave interno.
    const char* GetCodename();

    // Inicia Jevaing.
    int Run();
}
