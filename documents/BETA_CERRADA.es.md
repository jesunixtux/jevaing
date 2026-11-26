# Jevaing: preparación de beta cerrada (Windows)

**Estado:** preparación; NO se declara beta aprobada hasta que la compilación, las pruebas y la revisión manual sean satisfactorias.

## Alcance de la primera beta

- Windows Desktop x64 con MSVC, CMake y Git.
- Editor Dear ImGui: crear/abrir proyectos, editar escenas y usar Play/Stop.
- Juego independiente compilado a EXE sin depender del editor.
- Física 2D (Box2D), física 3D (Jolt), importación básica de modelos y texturas.
- Código de juegos en C++17 y módulos auxiliares en C.

No se anuncia como compatible con Xbox comercial, macOS, Linux, Vulkan, Metal, Lua, mods, plugins nativos, recuperación cifrada de fuentes ni servicios externos. Esas funciones siguen en el plan, no en la beta actual.

## Cambios para recuperar una compilación reproducible

- El patrón anterior "build/" ignoraba también el directorio de código "Engine/Build".
- Se incorporó Engine/Build/BuildSystem.h y .cpp para que CMake pueda encontrar la unidad de compilación que ya referenciaba, y para cubrir las interfaces usadas por Editor/Application/EditorMain.cpp y Engine/Core/Application.cpp.
- Los archivos temporales de CMake se excluyen del repositorio.
- La plantilla crea un proyecto C/C++ portable: la ruta del motor se especifica al compilar con JEVAING_ENGINE_ROOT.
- El exportador de Windows copia EXE, Assets y las escenas seleccionadas en Build Settings; escribe metadatos con la escena de inicio seleccionada. Requiere marcar confianza en los scripts CMake del proyecto.

## Validación obligatoria

1. Ejecutar la acción de GitHub "Windows beta build and smoke tests" en main y comprobar resultado verde.
2. Compilar localmente en un Windows limpio o VM sin directorios de desarrollo previos.
3. Abrir JevaingEditor.exe y crear un proyecto nuevo.
4. Añadir objetos, física 2D/3D, guardar y reabrir la escena.
5. Usar Play, Pause, Step, Stop y comprobar que Stop restaura la escena editable.
6. Usar Build y Build & Run. Verificar que el juego abre desde el directorio de salida, que contiene las escenas seleccionadas y los recursos.
7. Copiar el resultado a otra máquina sin Jevaing y comprobar arranque, controles y cierre.
8. Revisar avisos, dependencias y licencias de redistribución.
9. Repetir pruebas tras cada cambio de API o dependencias.
10. Probar con al menos un juego pequeño real antes de anunciar la beta.

## Prioridades posteriores

1. Runtime Lua con API de escenas, entidades y eventos, con controles de acceso.
2. Ampliar la API C v1 experimental a escenas, entidades, física y recursos; comprobar su ABI por plataforma.
3. Mods como función opcional: **apagada por defecto** y excluida de la compilación si no se usa.
4. Adaptadores para SDK/API externas, también opcionales; los SDK propietarios nunca se almacenan en el repositorio público.
5. Paquetes de contenido y sistema de recuperación cifrada de proyectos como funciones opt-in separadas.
6. Mejoras del editor y audio/animaciones antes de juegos completos.

## Limitaciones de esta preparación

No se ha verificado en hardware real Xbox ni se ha probado visualmente el editor mediante esta actualización. El nuevo pipeline de CI y los builds locales deben proporcionar las evidencias antes de cambiar el estado.
