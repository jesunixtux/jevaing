# SkyDodge2D

Juego 2D de prueba hecho con Jevaing. Mueve la nave con **A/D** o las flechas, recoge los cuadrados dorados y esquiva los rojos. **Espacio** reinicia tras perder tres vidas. La barra superior muestra puntos y los tres cuadros de la esquina muestran vidas.

Desde la raíz del repositorio:

```powershell
cmake -S Samples/SkyDodge2D -B build-skydodge
cmake --build build-skydodge --config Debug --target SkyDodge2D
build-skydodge/bin/Debug/SkyDodge2D.exe --logic-test
build-skydodge/bin/Debug/SkyDodge2D.exe --renderer null --frames 2
build-skydodge/bin/Debug/SkyDodge2D.exe
```

Para crear el ZIP Release después de compilar `SkyDodge2D` en `build-codex`:

```powershell
.\Samples\SkyDodge2D\Package.ps1
```

El ejecutable lee `jevaing.project` y la escena junto a sí mismo. El juego usa dibujo 2D, teclado, ciclo de vida, proyecto y carga de escena del motor. No requiere imágenes externas. La colisión sencilla del minijuego está en su propio código y no prueba Box2D.

La copia Release para probar está en `Builds/Windows-x64-Release/SkyDodge2D.exe` y el ZIP en `Builds/SkyDodge2D-Windows-x64-Release.zip` después de compilar y empaquetar localmente. Extrae el ZIP completo antes de abrir el ejecutable; necesita `Scenes/main.scene` y `jevaing.project` en la misma carpeta base. El ZIP se verificó en este equipo, no en una instalación limpia de Windows.
