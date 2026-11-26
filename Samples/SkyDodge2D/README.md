# SkyDodge2D

Minijuego 2D hecho con Jevaing. Recoge **8 cuadrados dorados** antes de perder tus **3 vidas** por tocar los rojos. La pantalla muestra el objetivo, los controles, vidas y puntuación.

- **Enter:** iniciar una partida o volver a jugar tras ganar/perder.
- **A/D** o **flechas izquierda/derecha:** mover la nave.
- **Espacio:** pausar y continuar.

## Jugar en otra PC

Descarga o copia `Builds/SkyDodge2D-Windows-x64-Release.zip`, extrae **todo** el ZIP y abre `SkyDodge2D.exe`. El juego incluye el código de Jevaing y el runtime de C/C++ enlazados dentro del ejecutable. No hay que instalar Jevaing ni el Visual C++ Redistributable. Requiere Windows de 64 bits con los componentes de sistema DirectX 11, D3DCompiler 47 y XInput 1.4; no se ha probado todavía en un equipo Windows limpio.

Conserva `jevaing.project` y `Scenes/main.scene` junto al EXE dentro de la carpeta extraída. No necesita imágenes, fuentes ni bibliotecas de terceros por separado. La colisión simple de este minijuego está implementada en el propio juego; no prueba Box2D.

## Compilar y verificar

Desde la raíz del repositorio, con CMake y MSVC instalados **solo en el equipo de compilación**:

```powershell
cmake -S Samples/SkyDodge2D -B build-skydodge-portable
cmake --build build-skydodge-portable --config Release --target SkyDodge2D
.\Samples\SkyDodge2D\VerifyPortable.ps1
```

El CMake del prototipo usa `/MT` para enlazar el runtime de Visual C++ de forma estática. `VerifyPortable.ps1` comprueba la tabla de importaciones, la lógica del juego, la ejecución de dos fotogramas y el ZIP extraído en una carpeta temporal fuera del repositorio. `Package.ps1` genera únicamente el ZIP si ya compilaste el EXE portable.
