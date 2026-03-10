#!/usr/bin/env bash
# =============================================================================
#  O3DE Editor Watchdog  —  Linux
#  Similar to Unreal Engine's CrashReporter subprocess.
#
#  Lanza el Editor y lo monitorea en segundo plano:
#    • Si el Editor se CRASEA  (exit code != 0): guarda Editor.log + info
#    • Si el Editor se FREEZEA (heartbeat > FREEZE_TIMEOUT seg sin actualizar):
#        - Captura backtrace con GDB si está disponible
#        - Guarda snapshot del Editor.log
#        - Mata el proceso y reporta
#
#  Uso:
#    ./scripts/editor_watchdog.sh [--project-path /ruta/al/proyecto] [otros args del Editor...]
#
#  Todos los argumentos no reconocidos se pasan directo al Editor.
# =============================================================================

set -euo pipefail

# ── Configuración ─────────────────────────────────────────────────────────────
EDITOR_BIN="/mnt/sda/DuskEngine/o3de/build/linux/bin/profile/Editor"
FREEZE_TIMEOUT=10          # segundos sin heartbeat = freeze

# Entorno necesario en Linux
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-xcb}"
export DRI_PRIME="${DRI_PRIME:-1}"
LOG_DIR="/tmp/o3de_watchdog_logs"
mkdir -p "$LOG_DIR"

# ── Parse args ────────────────────────────────────────────────────────────────
PROJECT_PATH=""
EDITOR_ARGS=()
i=1
while [[ $i -le $# ]]; do
    arg="${!i}"
    if [[ "$arg" == "--project-path" || "$arg" == "-project-path" ]]; then
        i=$((i+1))
        PROJECT_PATH="${!i}"
        EDITOR_ARGS+=("$arg" "$PROJECT_PATH")
    else
        EDITOR_ARGS+=("$arg")
    fi
    i=$((i+1))
done

# Si no se pasó --project-path, intentar leerlo del project.json del CWD
if [[ -z "$PROJECT_PATH" && -f "project.json" ]]; then
    PROJECT_PATH="$(pwd)"
fi

TS() { date '+%Y%m%d_%H%M%S'; }
LOG_PREFIX="$LOG_DIR/o3de_$(TS)"

# ── Función: guardar snapshot del log del Editor ──────────────────────────────
save_editor_log() {
    local reason="$1"
    local dest="${LOG_PREFIX}_${reason}_editor.log"
    local src=""

    if [[ -n "$PROJECT_PATH" ]]; then
        src="$PROJECT_PATH/user/log/Editor.log"
    fi

    if [[ -n "$src" && -f "$src" ]]; then
        cp "$src" "$dest"
        echo "[Watchdog] Editor.log guardado en: $dest"
    else
        echo "[Watchdog] No se encontró Editor.log en: $src"
    fi
}

# ── Función: capturar backtrace con GDB ───────────────────────────────────────
capture_gdb_backtrace() {
    local pid="$1"
    local dest="${LOG_PREFIX}_freeze_backtrace.log"

    if command -v gdb &>/dev/null; then
        echo "[Watchdog] Capturando backtrace de PID $pid con GDB..."
        timeout 15 gdb -batch \
            -ex "set pagination 0" \
            -ex "attach $pid" \
            -ex "thread apply all bt full" \
            -ex "detach" \
            -ex "quit" \
            "$EDITOR_BIN" "$pid" > "$dest" 2>&1 || true
        echo "[Watchdog] Backtrace guardado en: $dest"
    else
        echo "[Watchdog] GDB no disponible. Instalá con: sudo pacman -S gdb  o  sudo apt install gdb"
        # Fallback: /proc/<pid>/wchan muestra en qué syscall está bloqueado
        local wchan_dest="${LOG_PREFIX}_freeze_wchan.txt"
        {
            echo "=== PID $pid wchan (syscall bloqueado) ==="
            cat /proc/"$pid"/wchan 2>/dev/null || echo "(no disponible)"
            echo ""
            echo "=== /proc/$pid/status ==="
            cat /proc/"$pid"/status 2>/dev/null || true
            echo ""
            echo "=== /proc/$pid/stack ==="
            cat /proc/"$pid"/stack   2>/dev/null || echo "(requiere root)"
        } > "$wchan_dest"
        echo "[Watchdog] Info del proceso guardada en: $wchan_dest"
    fi
}

# ── Lanzar Editor ─────────────────────────────────────────────────────────────
echo "================================================"
echo "  O3DE Editor Watchdog"
echo "  Editor  : $EDITOR_BIN"
echo "  Proyecto: ${PROJECT_PATH:-'(no especificado)'}"
echo "  Freeze timeout: ${FREEZE_TIMEOUT}s"
echo "  Logs en : $LOG_DIR"
echo "================================================"

"$EDITOR_BIN" "${EDITOR_ARGS[@]}" &
EDITOR_PID=$!
HEARTBEAT_FILE="/tmp/o3de_editor_heartbeat_${EDITOR_PID}"

echo "[Watchdog] Editor PID: $EDITOR_PID"
echo "[Watchdog] Heartbeat : $HEARTBEAT_FILE"

# Esperar a que el Editor muera o congelarse
LAST_HEARTBEAT=0
FREEZE_DETECTED=false

while kill -0 "$EDITOR_PID" 2>/dev/null; do
    sleep 2

    # Leer heartbeat
    if [[ -f "$HEARTBEAT_FILE" ]]; then
        HB_TIME=$(cat "$HEARTBEAT_FILE" 2>/dev/null | tr -d '[:space:]' || echo "0")
        NOW=$(date +%s)
        AGE=$((NOW - HB_TIME))

        if [[ "$HB_TIME" -gt 0 && $AGE -gt $FREEZE_TIMEOUT ]]; then
            echo ""
            echo "╔══════════════════════════════════════════╗"
            echo "║  ⚠  FREEZE DETECTADO  (${AGE}s sin heartbeat) ║"
            echo "╚══════════════════════════════════════════╝"
            echo "[Watchdog] El Editor lleva ${AGE}s sin responder."

            capture_gdb_backtrace "$EDITOR_PID"
            save_editor_log "freeze"

            {
                echo "=== O3DE Freeze Report ==="
                echo "Time     : $(date)"
                echo "PID      : $EDITOR_PID"
                echo "Frozen   : ${AGE}s"
                echo "Heartbeat: $HB_TIME (epoch)"
                echo "Project  : $PROJECT_PATH"
            } > "${LOG_PREFIX}_freeze_report.txt"

            echo "[Watchdog] Terminando Editor congelado..."
            kill -9 "$EDITOR_PID" 2>/dev/null || true
            FREEZE_DETECTED=true
            break
        fi
    fi
done

# Esperar a que termine
wait "$EDITOR_PID" 2>/dev/null || true
EXIT_CODE=$?

echo ""
if $FREEZE_DETECTED; then
    echo "══ RESULTADO: FREEZE (proceso terminado por watchdog) ══"
elif [[ $EXIT_CODE -ne 0 && $EXIT_CODE -ne 2 ]]; then
    # exit code 2 = ProjectManager lanzado (normal)
    echo "╔══════════════════════════════════════════╗"
    echo "║  ✖  CRASH DETECTADO  (exit code: $EXIT_CODE)   ║"
    echo "╚══════════════════════════════════════════╝"

    save_editor_log "crash"

    {
        echo "=== O3DE Crash Report ==="
        echo "Time     : $(date)"
        echo "PID      : $EDITOR_PID"
        echo "ExitCode : $EXIT_CODE"
        echo "Project  : $PROJECT_PATH"
        echo ""
        echo "=== Crash logs in /tmp/ ==="
        ls -lt /tmp/o3de_crash_*.log 2>/dev/null | head -5 || echo "(ninguno)"
    } > "${LOG_PREFIX}_crash_report.txt"

    echo "[Watchdog] Reporte guardado en: ${LOG_PREFIX}_crash_report.txt"

    # Mostrar último crash log si existe
    LATEST_CRASH=$(ls -t /tmp/o3de_crash_*.log 2>/dev/null | head -1 || true)
    if [[ -n "$LATEST_CRASH" ]]; then
        echo ""
        echo "── Último crash log ($LATEST_CRASH) ──────"
        cat "$LATEST_CRASH"
    fi
else
    echo "══ Editor terminó normalmente (exit code: $EXIT_CODE) ══"
fi

echo "[Watchdog] Todos los logs están en: $LOG_DIR"
