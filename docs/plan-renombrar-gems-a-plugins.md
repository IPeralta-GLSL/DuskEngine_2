# Plan: renombrar `Gems/` a `plugins/` en Dusk Engine

## 1. Alcance y advertencias

- Solo se renombra la **carpeta física** del motor que contiene las gems: `Gems/` → `plugins/`.
- No se renombra el concepto "gem" ni sus identificadores internos:
  - `gem.json`, `gem_name`, `ly_add_gem`, `AZ::Gem`, etc. permanecen iguales.
  - Carpetas de usuario (`~/DuskEngine/Gems`, `~/DuskEngine/Restricted/Gems`) permanecen iguales.
- Es posible, pero requiere actualizar todas las rutas literales `Gems/` que apunten al motor.
- **FirstPersonController se elimina**: tanto `Gems/FirstPersonController` como `plugins/FirstPersonController` se borran, y se saca la entrada de `engine.json`. Es un plugin de nodos visuales que no va a seguir en el proyecto.

## 2. Estrategia de alto nivel

1. Hacer una rama limpia.
2. Eliminar `Gems/FirstPersonController` y `plugins/FirstPersonController`, y quitar la entrada de `engine.json`.
3. Mover el resto de subdirectorios de `Gems/<nombre>` a `plugins/<nombre>`.
4. Actualizar `engine.json` para que apunte a `plugins/<nombre>`.
5. Actualizar scripts de build, empaquetado, Docker, Project Manager y config del Asset Processor.
6. Actualizar `.gitattributes` para que los archivos LFS sigan bajo control de LFS tras el movimiento.
7. Hacer una búsqueda/auditoría global de rutas literales `Gems/` y reemplazar las que correspondan.
8. Configurar CMake y compilar para verificar.

## 3. Archivos obligatorios a modificar

| Archivo | Líneas | Cambio |
|---------|--------|--------|
| `engine.json` | 16-99 | Todas las entradas de `external_subdirectories`: prefijo `Gems/` → `plugins/`. Eliminar la entrada `Gems/FirstPersonController`. |
| `scripts/project_manager_lite.py` | 678 | `ENGINE_PATH / "Gems"` → `ENGINE_PATH / "plugins"` |
| `scripts/ProjectManager.py` | 678 | `ENGINE_PATH / "Gems"` → `ENGINE_PATH / "plugins"` |
| `Registry/AssetProcessorPlatformConfig.setreg` | 215 | Patrón `Gems/*` → agregar/reemplazar por `plugins/*`. Se recomienda mantener ambos para no romper proyectos ajenos que aún usen `Gems/`. |
| `AutomatedTesting/Config/shader_global_build_options.json` | 13 | `Gems/AtomTressFX/Assets/Shaders` → `plugins/AtomTressFX/Assets/Shaders` |
| `AutomatedTesting/UI/TextureAtlas/sample.texatlas` | 1 | `@engroot@/Gems/LyShineExamples/...` → `@engroot@/plugins/LyShineExamples/...` |
| `cmake/Platform/Linux/Packaging/postinst.in` | 44-45 | `Gems/Atom/RPI/Tools` y `Gems/AtomLyIntegration/...` → `plugins/...` |
| `cmake/Platform/Linux/Packaging_Snapcraft.cmake` | 32 | `"Code;Gems;scripts;Tools"` → `"Code;plugins;scripts;Tools"` |
| `Docker/build.sh` | 65-66 | `/opt/O3DE/.../Gems/...` → `/opt/O3DE/.../plugins/...` |
| `scripts/build/Jenkins/Jenkinsfile` | 417-420 | `fileExists("Gems")`, `--all-gems-path .../Gems`, `--all-gem-paths .../Gems` → `plugins` |
| `Templates/RemoteRepo/template.json` | 23, 37 | `"Gems/README.md"` y `"dir": "Gems"` → `"plugins"` |
| `.gitattributes` | 122-127 | Patrones LFS bajo `Gems/` → `plugins/` |

## 4. Pasos detallados

### Fase A: preparación

1. Crea una rama limpia sin cambios sin commitear.
2. Identifica `Gems/FirstPersonController` y `plugins/FirstPersonController` para eliminarlos; su entrada en `engine.json` también se va a quitar.
3. Lista los directorios a mover:

```bash
ls -1 /mnt/sda/DuskEngine/o3de/Gems
```

### Fase B: eliminar `FirstPersonController`

Borra ambas copias y su entrada en `engine.json`:

```bash
cd /mnt/sda/DuskEngine/o3de
git rm -rf Gems/FirstPersonController plugins/FirstPersonController
```

### Fase C: mover el resto de los directorios de gems

No uses `git mv Gems plugins` porque `plugins/` ya existe. En su lugar, mueve cada subdirectorio restante:

```bash
cd /mnt/sda/DuskEngine/o3de
for gem in Gems/*/; do
    name=$(basename "$gem")
    if [ -d "plugins/$name" ]; then
        echo "Omitido (ya existe): plugins/$name"
    else
        git mv "$gem" "plugins/$name"
    fi
done

rmdir Gems 2>/dev/null || true
```

Si existen archivos sueltos directamente bajo `Gems/`, muévelos también bajo `plugins/` con el destino que tenga sentido.

### Fase D: actualizar `engine.json`

Reemplaza el prefijo `Gems/` por `plugins/` en todo el array `external_subdirectories`. Elimina la entrada `Gems/FirstPersonController`.

### Fase E: actualizar scripts de build, empaquetado y tooling

Edita los archivos listados en la tabla del punto 3. En cada uno, reemplaza las rutas literales `Gems/...` por `plugins/...`.

### Fase F: actualizar `.gitattributes`

Reemplaza los patrones LFS que empiecen con `Gems/` por `plugins/`.

### Fase G: auditoría global de rutas literales

Busca rutas físicas que aún digan `Gems/`:

```bash
cd /mnt/sda/DuskEngine/o3de
grep -RIn --include='*.cmake' --include='*.json' --include='*.py' \
  --include='*.setreg' --include='*.sh' --include='*.in' \
  --include='*.bat' --include='*.cmd' --include='*.texatlas' \
  --include='*.azsli' --include='*.azsl' --include='*.txt' --include='*.md' \
  '\bGems/' . \
  | grep -v '/.venv/' | grep -v '/build/' | grep -v '/.git/' \
  | grep -v 'scripts/o3de/tests/' \
  | head -200
```

Para cada coincidencia, decide si es una ruta física del motor (cámbiala a `plugins/`) o es una referencia conceptual o de carpeta de usuario (no la cambies).

Archivos/conceptos que **NO** debes cambiar:

- `scripts/o3de/o3de/manifest.py`: `get_o3de_gems_folder()`, `default_gems_folder`, `default_restricted_gems_folder` (son carpetas de usuario `~/DuskEngine/Gems`).
- `scripts/o3de/o3de/register.py`: registro de la carpeta de gems por defecto del usuario.
- Claves JSON como `"Gems": { ... }` en `.setreg` que son nombres lógicos, no rutas.
- Test fixtures abstractos como `Gems/Gem1` si no representan rutas reales del filesystem.

### Fase H: actualizar comentarios y documentación (solo si contienen rutas reales)

Si un comentario o documento contiene una ruta literal del tipo `Gems/Atom/...`, actualízala a `plugins/Atom/...` para no dejar información falsa. No añadas comentarios explicativos del cambio.

Ejemplos que suelen necesitar revisión:

- `Templates/*/Template/Code/CMakeLists.txt` (comentarios con ejemplos de rutas).
- `AutomatedTesting/Gem/PythonTests/Atom/atom_utils/atom_constants.py` y `atom_tools_utils.py`.
- Scripts y readmes dentro de `plugins/AtomLyIntegration/TechnicalArt/DccScriptingInterface/`.

### Fase I: verificación

1. Revisa que no queden rutas físicas `Gems/` del motor:

```bash
grep -RIn --include='*.cmake' --include='*.json' --include='*.py' \
  --include='*.setreg' --include='*.sh' --include='*.in' \
  --include='*.bat' --include='*.cmd' --include='*.texatlas' \
  --include='*.azsli' --include='*.azsl' --include='*.txt' --include='*.md' \
  '\bGems/' . | grep -v '/scripts/o3de/tests/' | grep -v 'get_o3de_gems_folder' | grep -v 'default_gems_folder'
```

2. Configura el build:

```bash
cmake -B build/linux -S . -DLY_PROJECTS=AutomatedTesting
```

3. Compila un objetivo mínimo para comprobar que CMake encuentra las gems:

```bash
cmake --build build/linux --target AssetProcessor
```

4. Ejecuta los tests de Python de `scripts/o3de/tests` y corrige los que dependan de rutas reales del motor.

5. Revisa `git status` y asegúrate de que no queden archivos huérfanos bajo `Gems/`.

## 5. Reglas de estilo: sin comentarios en el código

- No añadas comentarios explicativos en CMake, Python, C++, JSON, `.setreg`, batch o shell para justificar el renombrado.
- No dejes `TODO`, `FIXME` ni notas del tipo "renombrado".
- Si un comentario existente contiene una ruta literal que ahora es falsa, actualízala a `plugins/...`, pero no añadas texto adicional.
- Los mensajes de commit y este documento sí pueden explicar el cambio.

## 6. Lista de verificación final

- [ ] Rama limpia creada.
- [ ] `Gems/FirstPersonController` y `plugins/FirstPersonController` fueron eliminados.
- [ ] La entrada `Gems/FirstPersonController` fue eliminada de `engine.json`.
- [ ] `Gems/` está vacía o eliminada.
- [ ] Todo el contenido anterior de `Gems/` está bajo `plugins/`.
- [ ] `engine.json` solo contiene prefijos `plugins/` en `external_subdirectories`.
- [ ] `FirstPersonController` no tiene carpetas ni entradas en `engine.json`.
- [ ] `.gitattributes` tiene los patrones LFS bajo `plugins/`.
- [ ] Los scripts `project_manager_lite.py` y `ProjectManager.py` escanean `plugins/`.
- [ ] `Registry/AssetProcessorPlatformConfig.setreg` excluye `plugins/*` (y conserva `Gems/*` si se quiere compatibilidad con proyectos ajenos).
- [ ] `cmake/Platform/Linux/Packaging/postinst.in`, `Packaging_Snapcraft.cmake` y `Docker/build.sh` apuntan a `plugins/`.
- [ ] `AutomatedTesting/Config/shader_global_build_options.json` y `AutomatedTesting/UI/TextureAtlas/sample.texatlas` apuntan a `plugins/`.
- [ ] CMake configura sin errores.
- [ ] Al menos `AssetProcessor` compila.
- [ ] `git status` no muestra archivos perdidos bajo `Gems/`.

## 7. Riesgos y decisiones pendientes

- Proyectos externos o plantillas que aún usen rutas `Gems/` dentro de sus propios repos necesitarán su propio renombrado.
- Usuarios que tengan el motor registrado en `~/.o3de/o3de_manifest.json` con rutas absolutas a gems dentro del motor pueden necesitar volver a registrar el motor (`scripts/o3de/o3de/register.py`).
- Este plan no renombra `_temp_disabled_gems/`; si se desea coherencia, trátalo por separado.
- Renombrar el concepto completo "gem" a "plugin" (clases C++, buses, `gem.json`, etc.) está fuera del alcance y rompería la compatibilidad.
