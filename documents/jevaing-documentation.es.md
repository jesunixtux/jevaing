# Documentacion de Jevaing

Version: `0.0.11`
Estado: prototipo temprano, no listo para produccion

## 1. Vision General

Jevaing es un motor experimental de videojuegos escrito en C++17 y construido con CMake. Su objetivo actual es entregar una base pequena pero usable para juegos independientes, samples externos, herramientas de editor, render 2D/3D, fisica y preparacion temprana de targets de plataforma.

Jevaing 0.0.11 agrega la primera pasada usable del editor y mantiene la base de fisica de 0.0.10. El motor ahora puede compilar un ejecutable externo de juego mediante una plantilla de proyecto, y el editor puede abrir, editar, previsualizar, reproducir, pausar, avanzar frame a frame, guardar y compilar un proyecto.

El proyecto mantiene intencionalmente APIs neutrales. El codigo de juego usa conceptos de Jevaing como `Scene`, componentes, `Graphics2D`, `Graphics3D`, `PhysicsWorld2D`, `PhysicsWorld3D` e `Input`. Detalles de plataforma o backend como Win32, DirectX, XInput, Box2D, Jolt, ImGui, UWP o GDK no se exponen en la API publica.

## 2. Estructura Del Repositorio

```text
Jevaing/
|-- Api/                 Headers publicos del motor para codigo de juego.
|-- Engine/              Implementacion runtime, renderer, assets, fisica y build tooling.
|-- Editor/              Codigo fuente de JevaingEditor.
|-- Sandbox/             Host de pruebas y samples internos.
|-- Samples/             Samples consumidores externos.
|-- geometry/            Assets locales de ejemplo.
|-- Library/             Area de libreria runtime.
|-- Documentation/       Area de documentacion existente.
|-- documents/           Documentacion bilingue generada para 0.0.11.
|-- CMakeLists.txt       Build raiz de CMake.
|-- README.md            Resumen principal del proyecto.
|-- THIRD_PARTY.md       Notas de dependencias de terceros.
`-- LICENSE
```

## 3. Requisitos De Build

Configuracion recomendada en Windows Desktop:

- Windows 10 o superior.
- CMake 3.20 o superior.
- Toolchain Visual Studio / MSVC.
- Git, porque CMake descarga dependencias cuando no estan disponibles localmente.

Dependencias principales usadas o descargadas por el build:

- Assimp para carga de modelos.
- Box2D 3.1.1 para fisica 2D cuando esta habilitada.
- Jolt 5.6.0 para fisica 3D cuando esta habilitada.
- Dear ImGui 1.92.1 docking para `JevaingEditor` cuando el editor esta habilitado.

## 4. Compilar Desde Codigo Fuente

Configurar y compilar Debug:

```powershell
cmake -S . -B build
cmake --build build --config Debug
```

Salidas principales:

```text
bin/Debug/JevaingSandbox.exe
bin/Debug/JevaingEditor.exe
build/Samples/XboxSmokeTest/bin/Debug/XboxSmokeTest.exe
```

Opciones utiles de CMake:

```powershell
cmake -S . -B build-runtime -DJEVAING_BUILD_EDITOR=OFF
cmake --build build-runtime --config Debug

cmake -S . -B build-nophysics `
  -DJEVAING_ENABLE_PHYSICS_2D=OFF `
  -DJEVAING_ENABLE_PHYSICS_3D=OFF `
  -DJEVAING_BUILD_EDITOR=OFF `
  -DJEVAING_BUILD_XBOX_SMOKE_TEST=OFF
cmake --build build-nophysics --config Debug
```

## 5. JevaingEditor

Ejecutar el editor:

```powershell
.\bin\Debug\JevaingEditor.exe
```

Mostrar informacion del editor sin abrir la UI:

```powershell
.\bin\Debug\JevaingEditor.exe --editor-info
```

Paneles actuales del editor:

- `Project Browser`: crear o abrir proyectos.
- `Hierarchy`: crear/seleccionar entidades y ver relaciones padre-hijo.
- `Scene`: vista de editor para seleccion, grid y edicion de transform con mouse.
- `Game`: preview del encuadre de la camara del juego.
- `Inspector`: editar nombres, transforms y componentes principales.
- `Project`: navegador estilo Unity para `Assets`, `Scenes` y `Source`.
- `Console`: muestra logs del motor/editor.
- `Build Settings`: elegir target/configuracion y compilar el ejecutable Windows Desktop.

Menu superior:

- `File`: guardar, cerrar proyecto, detener Play Mode, salir.
- `Edit`, `GameObject`, `Assets`, `Build`, `Help`: secciones reservadas para herramientas futuras.
- `Windows`: mostrar u ocultar paneles del editor.

Controles de Play:

- `Play`: crea un snapshot runtime de la escena de edicion.
- `Pause`: detiene actualizaciones de simulacion sin salir de Play Mode.
- `Resume`: reanuda la simulacion despues de pausa.
- `Step`: avanza un frame de simulacion estando en pausa.
- `Stop`: descarta la escena runtime y restaura el snapshot de edicion.

El editor muestra una advertencia por cambios sin guardar antes de salir, cerrar un proyecto o cargar otro proyecto/escena.

## 6. Vistas Scene Y Game

`Scene` y `Game` son ventanas separadas del editor.

La vista `Scene` es una preview de editor. Dibuja un grid y bounds simples de entidades, permite seleccionar entidades con el mouse y arrastrar entidades seleccionadas en el plano X/Y mientras no se esta en Play Mode.

La vista `Game` es una preview de camara. Dibuja un marco 16:9 usando la camara de la escena como referencia de offset. Si existe una camara primaria, se prefiere esa; si no, se usa la primera entidad con `CameraComponent`. Esta vista todavia es una preview MVP dibujada por el editor y no el render target D3D11 final embebido del runtime.

## 7. Flujo De Proyecto

Un proyecto generado contiene:

```text
MyGame/
|-- jevaing.project
|-- Assets/
|   |-- Models/
|   `-- Textures/
|-- Scenes/
|   `-- main.scene
|-- Source/
|   |-- Game.cpp
|   |-- GameCode.c
|   `-- GameCode.h
`-- CMakeLists.txt
```

El CMake generado espera `JEVAING_ENGINE_ROOT` al configurar y no guarda rutas absolutas personales. Compila todos los archivos `Source/*.cpp` y `Source/*.c` mediante `GAME_SOURCES`, por lo que un proyecto puede mezclar entrada de juego en C++ con modulos auxiliares en C.

El panel Project del editor permite crear:

- scripts C++.
- archivos C con header.
- headers.
- carpetas.

Los archivos C y C++ se crean bajo `Source` para que participen en el build generado.

## 8. API Publica De Juego

El codigo de juego incluye:

```cpp
#include <Jevaing/Jevaing.h>
```

Conceptos publicos principales:

- `Game` y `GameConfig`.
- `Run(...)`.
- `Project` y `ProjectConfig`.
- `Scene` y `SceneEntity`.
- `EntityId`.
- `TransformComponent`.
- `CameraComponent`.
- `MeshRendererComponent`.
- `SpriteRenderer2DComponent`.
- cuerpos rigidos y colliders 2D/3D.
- `Graphics2D` y `Graphics3D`.
- teclado, mouse, gamepad e input maps.
- `PhysicsWorld2D` y `PhysicsWorld3D`.

Politica de API publica:

- Sin tipos ImGui.
- Sin handles Win32.
- Sin handles DirectX.
- Sin handles XInput.
- Sin tipos Box2D.
- Sin tipos Jolt.
- Sin tipos UWP/GDK.

## 9. Rendering

Trabajo actual de rendering:

- renderer DirectX 11 desktop.
- renderer Null para validacion headless/core.
- APIs de dibujo 2D y 3D.
- carga de modelos mediante Assimp.
- carga de texturas y tests orientados a materiales.
- smoke tests visuales para triangulos, sprites texturizados, cubos, iluminacion, modelos y mezcla 2D/3D.

La preview del editor actualmente dibuja representaciones simplificadas con ImGui. Sirve para editar y encuadrar camaras, pero aun no es un renderer runtime offscreen final embebido dentro del editor.

## 10. Fisica

Jevaing 0.0.10/0.0.11 mantiene la fisica detras de APIs publicas neutrales.

Fisica 2D:

- API publica: `PhysicsWorld2D`, `RigidBody2DComponent`, `BoxCollider2DComponent`, `CircleCollider2DComponent`, raycasts/eventos 2D.
- Backend: Box2D 3.1.1.

Fisica 3D:

- API publica: `PhysicsWorld3D`, `RigidBody3DComponent`, `BoxCollider3DComponent`, `SphereCollider3DComponent`, `CapsuleCollider3DComponent`, raycasts/eventos 3D.
- Backend: Jolt 5.6.0.

Scene trabaja con componentes Jevaing y mundos de fisica neutrales. La traduccion hacia cada backend pertenece a la implementacion del backend.

## 11. Input Y Gamepad

Jevaing expone APIs neutrales para teclado, mouse y gamepad.

Ejemplo de gamepad:

```cpp
if (Jevaing::Input::IsGamepadConnected(0))
{
    Jevaing::GamepadState state = Jevaing::Input::GetGamepadState(0);
    if (Jevaing::Input::IsGamepadButtonPressed(0, Jevaing::GamepadButton::A))
    {
        // Manejar boton A.
    }
}
```

Windows Desktop actualmente usa XInput detras de la API publica.

## 12. Build Targets

Estado actual de targets:

- Windows Desktop x64: target funcional.
- Xbox Dev Mode / UWP x64: smoke target experimental dependiente del entorno.
- Xbox One / GDK x64: target futuro/preparacion, no disponible sin entorno GDK autorizado.
- Xbox Series X|S / GDK x64: target futuro/preparacion, no disponible sin entorno GDK autorizado.

El editor puede compilar proyectos Windows Desktop x64 en esta pasada. Los otros targets se muestran para visibilidad y deteccion de entorno.

Xbox no esta verificado hasta que un package arranque en hardware Xbox real y el usuario lo confirme.

## 13. Xbox Smoke Test

`Samples/XboxSmokeTest` es el primer smoke target minimo orientado a Xbox:

- arranque de app.
- ruta runtime DirectX donde sea soportada.
- render de triangulo/cubo.
- gamepad mediante la API neutral de Jevaing.

Smoke desktop:

```powershell
cmake --build build --config Debug --target XboxSmokeTest
.\build\Samples\XboxSmokeTest\bin\Debug\XboxSmokeTest.exe --frames 120
```

Forma experimental de configurar UWP:

```powershell
cmake -S Samples\XboxSmokeTest -B build-xbox-smoke-uwp `
  -DCMAKE_SYSTEM_NAME=WindowsStore `
  -DCMAKE_SYSTEM_VERSION=10.0 `
  -DCMAKE_GENERATOR_PLATFORM=x64 `
  -DJEVAING_ENGINE_ROOT=<path-to-jevaing>
cmake --build build-xbox-smoke-uwp --config Debug
```

No se guarda en el repositorio ningun certificado, clave privada ni archivo privado de SDK Microsoft.

## 14. Samples

Sample externo Minimal3D:

```powershell
cmake -S Samples\Minimal3D -B build-minimal3d
cmake --build build-minimal3d --config Debug
.\build-minimal3d\bin\Debug\Minimal3D.exe --frames 120
```

Sample externo Physics3D:

```powershell
cmake -S Samples\Physics3D -B build-physics3d
cmake --build build-physics3d --config Debug
.\build-physics3d\bin\Debug\Physics3D.exe --frames 120
```

Estos samples son importantes porque consumen Jevaing como dependencia externa del motor, no mediante flags internos de Sandbox.

## 15. Validacion Por Linea De Comandos

Core:

```powershell
.\bin\Debug\JevaingSandbox.exe --self-test
.\bin\Debug\JevaingSandbox.exe --runtime-test
.\bin\Debug\JevaingSandbox.exe --scene-serialization-test
.\bin\Debug\JevaingSandbox.exe --hierarchy-test
```

Rendering/assets:

```powershell
.\bin\Debug\JevaingSandbox.exe --renderer-info
.\bin\Debug\JevaingSandbox.exe --graphics-test
.\bin\Debug\JevaingSandbox.exe --graphics-test-3d
.\bin\Debug\JevaingSandbox.exe --texture-test
.\bin\Debug\JevaingSandbox.exe --material-test
.\bin\Debug\JevaingSandbox.exe --lighting-test
.\bin\Debug\JevaingSandbox.exe --multi-model-test
.\bin\Debug\JevaingSandbox.exe --asset-cache-test
.\bin\Debug\JevaingSandbox.exe --asset-error-test
```

Fisica:

```powershell
.\bin\Debug\JevaingSandbox.exe --physics-info
.\bin\Debug\JevaingSandbox.exe --physics-fixed-step-test
.\bin\Debug\JevaingSandbox.exe --physics-3d-test
.\bin\Debug\JevaingSandbox.exe --physics-3d-stack-test
.\bin\Debug\JevaingSandbox.exe --physics-3d-sphere-test
.\bin\Debug\JevaingSandbox.exe --physics-3d-trigger-test
.\bin\Debug\JevaingSandbox.exe --physics-3d-raycast-test
.\bin\Debug\JevaingSandbox.exe --physics-2d-test
.\bin\Debug\JevaingSandbox.exe --physics-2d-circle-test
.\bin\Debug\JevaingSandbox.exe --physics-2d-trigger-test
.\bin\Debug\JevaingSandbox.exe --physics-2d-raycast-test
.\bin\Debug\JevaingSandbox.exe --physics-scene-serialization-test
.\bin\Debug\JevaingSandbox.exe --physics-destroy-test
.\bin\Debug\JevaingSandbox.exe --physics-hierarchy-test
```

Editor/build/plataforma:

```powershell
.\bin\Debug\JevaingSandbox.exe --build-target-info
.\bin\Debug\JevaingSandbox.exe --gamepad-test
.\bin\Debug\JevaingSandbox.exe --project-template-test
.\bin\Debug\JevaingSandbox.exe --editor-scene-roundtrip-test
.\bin\Debug\JevaingSandbox.exe --playmode-restore-test
.\bin\Debug\JevaingSandbox.exe --windows-build-test
.\bin\Debug\JevaingSandbox.exe --xbox-build-environment-test
.\bin\Debug\JevaingEditor.exe --editor-info
```

## 16. Checklist Manual Del Editor

1. Abrir `JevaingEditor.exe`.
2. Crear un proyecto nuevo.
3. Abrir `Scenes/main.scene`.
4. Seleccionar entidades en `Hierarchy`.
5. Seleccionar y arrastrar entidades en `Scene`.
6. Verificar el preview encuadrado por camara en `Game`.
7. Agregar/remover componentes en `Inspector`.
8. Crear archivos C y C++ desde el panel `Project`.
9. Presionar Play.
10. Presionar Pause.
11. Presionar Step.
12. Presionar Resume.
13. Presionar Stop y verificar que el estado de edicion se restaura.
14. Modificar la escena e intentar cerrar/cargar otra escena para verificar el modal de cambios sin guardar.
15. Guardar.
16. Compilar Windows Desktop x64 desde `Build Settings`.
17. Ejecutar el juego generado.

## 17. Limitaciones Conocidas

- El editor sigue siendo MVP y no un editor de produccion.
- `Scene` y `Game` son previews simplificadas, no render targets runtime finales embebidos.
- La edicion con mouse actualmente soporta arrastre basico X/Y, no un gizmo completo de transform.
- El panel `Project` crea archivos de codigo, pero no incluye editor de codigo integrado.
- Xbox Dev Mode/UWP no esta verificado en hardware.
- Los targets GDK requieren tooling Microsoft autorizado y no son targets completos de shipping.
- La validacion visual todavia requiere una persona.

## 18. Troubleshooting

Si el editor no abre:

```powershell
cmake --build build --config Debug
.\bin\Debug\JevaingEditor.exe --editor-info
```

Si los comandos de fisica reportan backends no disponibles, confirma que la fisica no este deshabilitada en el build directory activo.

Si un proyecto externo no configura, pasa `JEVAING_ENGINE_ROOT`:

```powershell
cmake -S <project> -B <project>\.jevaing-build\Windows -DJEVAING_ENGINE_ROOT=<path-to-jevaing>
```

Si archivos C/C++ generados no compilan, asegúrate de que esten bajo el directorio `Source` del proyecto.

## 19. Notas De Desarrollo

Mantener los cambios de API publica neutrales respecto al backend. La implementacion especifica de backend pertenece a internals del motor. Las dependencias exclusivas del editor deben permanecer linkeadas solo a `JevaingEditor`, no a ejecutables externos de juego.

Al agregar soporte de plataforma, preferir smoke tests pequenos antes de portar todo el motor. Xbox debe seguir marcado como no verificado hasta que la validacion en hardware real sea completada por el usuario.
