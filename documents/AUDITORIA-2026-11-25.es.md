# Auditoría estática — 2026-11-25
Base: main df8877ec59a646a5dcb38798c80ba6d03b661183. CI registrado: https://github.com/jesunixtux/jevaing/actions/runs/44876697119 (success). No compilación local ni inspección visual.

## Hallazgos
- P0: BuildSystem.cpp:388-403 construye comandos std::system e interpola project.Name. Project.cpp solo valida presencia del nombre. Usar proceso con argumentos separados, validar nombres y exigir confianza explícita antes de compilar CMake de terceros.
- P0: Project.cpp permite rutas absolutas y relativas no confinadas; BuildSystem.cpp:427-428 copia raíces configurables. Validar rutas canónicas, enlaces simbólicos y destino de staging.
- P1: Application.cpp:802-835 solo verifica existencia del EXE, no su ejecución ni recursos en máquina limpia.
- P1: EditorMain.cpp:573-625 carga copia de escena, no ejecuta código del juego en Play Mode.
- P1: Scene.cpp:826-962 y Project.cpp:127-150 guardan directamente sin verificar estado final del stream. Guardado transaccional, backup y pruebas de fallo.
- P1: snapshots temporales con nombres fijos; usar directorios únicos y limpieza RAII.
- P1: Windows es única plataforma gráfica real. Vulkan/Metal no implementados.
- P2: CI solo Debug/main/manual. Esta rama agrega PR y smoke de ABI C; falta Release, pruebas de runtime y ejecución exportada.
- P2: BuildSettings declara StartupScene y ScenesInBuild, pero exportador copia raíces completas y metadata original.
- P2: Lua, mods y plugins dinámicos pendientes.

## Cambios en esta rama
API C v1 experimental, muestra C11 pura y prueba de ABI en CI. Se conserva C++17. No se afirma compilación aprobada hasta CI. No se exponen todavía escenas, física ni assets en C.

## Roadmap
1. Seguridad de build, rutas y guardado.
2. Validar C/C++ Debug/Release, runtime null y DX11 en Windows real.
3. API C de escenas, entidades, física y ownership.
4. Play Mode ejecutando código real de proyectos en proceso separado.
5. SDK instalable, Lua opt-in, módulos externos versionados.
6. Precuela de JumpFall como juego 2D independiente de prueba; requiere audio, input, colisiones, escenas y guardado.

## Avance posterior a la auditoría

- P0 mitigado: el exportador dejó de usar `std::system`, valida nombre/rutas y exige confianza explícita antes de ejecutar CMake del proyecto.
- P1 parcialmente mitigado: Project y Scene guardan con archivo temporal y copia `.bak`; se recupera cuando falta el archivo principal. Falta probar cortes reales de energía y sincronización durable.
- P1 parcialmente mitigado: el test de exportación ejecuta dos frames del juego generado. Falta validación en Windows limpio sin código fuente ni herramientas de desarrollo.
- P1 pendiente: Play del editor todavía no ejecuta el código real del juego.
- P2 en progreso: Debug y Release compilaron y pasaron las pruebas locales, incluida la exportación de juego con arranque de dos frames; CI solicita ambas configuraciones y sus resultados remotos aún deben comprobarse. La ABI C v1 continúa experimental.
- El cargador nativo experimental de Windows admite manifiestos, dependencias y servicios versionados y pasó pruebas locales. El montaje opt-in de contenido de mods pasó pruebas de recursos/mapas y conserva los archivos originales; no está integrado automáticamente al runtime. Lua, audio y recuperación cifrada siguen pendientes.
