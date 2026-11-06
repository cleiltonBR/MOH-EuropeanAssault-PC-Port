#!/data/data/com.termux/files/usr/bin/bash
# Generate the private recompilation output from the user's own Xbox XBE.
# No XBE or generated game code is written into the Git working tree.

set -Eeuo pipefail
umask 077

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "${SCRIPT_DIR}/../.." && pwd)"
TOOLKIT_DIR="${REPO_ROOT}/source/xboxrecomp-0.16.0"

if [[ $# -lt 1 || $# -gt 2 ]]; then
    echo "Usage: bash android/tools/recompile-xbe-termux.sh /path/to/DEFAULT.XBE [private-output-dir]" >&2
    exit 2
fi

XBE_INPUT="$(realpath -- "$1")"
PRIVATE_ROOT="${2:-$HOME/MOH-EuropeanAssault-private}"
PRIVATE_ROOT="$(mkdir -p -- "$PRIVATE_ROOT" && cd -- "$PRIVATE_ROOT" && pwd)"
XBE_COPY="${PRIVATE_ROOT}/input/DEFAULT.XBE"
WORK_ROOT="${PRIVATE_ROOT}/recompilation"
mkdir -p "${PRIVATE_ROOT}/input" "${WORK_ROOT}"
chmod 700 "${PRIVATE_ROOT}" "${PRIVATE_ROOT}/input" "${WORK_ROOT}"

if [[ ! -f "$XBE_INPUT" ]]; then
    echo "ERROR: XBE file not found: $XBE_INPUT" >&2
    exit 1
fi
if [[ ! -f "${TOOLKIT_DIR}/tools/recomp/__main__.py" ]]; then
    echo "ERROR: xboxrecomp toolkit not found at: ${TOOLKIT_DIR}" >&2
    echo "Run this script from a checkout of cleiltonBR/MOH-EuropeanAssault-PC-Port (branch android-port)." >&2
    exit 1
fi

PYTHON_BIN="${PYTHON:-python}"
if ! command -v "$PYTHON_BIN" >/dev/null 2>&1; then
    echo "ERROR: Python is missing. In Termux run: pkg install python git" >&2
    exit 1
fi
PYTHON_VERSION="$("$PYTHON_BIN" -c 'import sys; print("%d.%d" % sys.version_info[:2])')"
"$PYTHON_BIN" -c 'import sys; sys.exit(0 if sys.version_info >= (3, 10) else 1)' || {
    echo "ERROR: Python 3.10 or newer is required (found $PYTHON_VERSION)." >&2
    exit 1
}

echo "Installing the recompiler's Python dependency (Capstone)..."
"$PYTHON_BIN" -m pip install --user --upgrade capstone

cp -- "$XBE_INPUT" "$XBE_COPY"
chmod 600 "$XBE_COPY"
XBE_COPY="$(realpath -- "$XBE_COPY")"

# All generated data is private and outside the Git repository.
rm -rf -- "$WORK_ROOT"
mkdir -p "$WORK_ROOT"/{disasm,func_id,abi,gen,recomp}
chmod 700 "$WORK_ROOT"

echo
echo "[1/5] Parse XBE headers, sections and kernel imports"
(
    cd "$TOOLKIT_DIR"
    PYTHONPATH="$TOOLKIT_DIR" "$PYTHON_BIN" -m tools.xbe_parser \
        "$XBE_COPY" --json "$WORK_ROOT/xbe-analysis.json"
)

echo
echo "[2/5] Disassemble executable code and detect functions"
(
    cd "$TOOLKIT_DIR"
    PYTHONPATH="$TOOLKIT_DIR" "$PYTHON_BIN" -m tools.disasm \
        "$XBE_COPY" --analysis-json "$WORK_ROOT/xbe-analysis.json" \
        --text-only --force -o "$WORK_ROOT/disasm" -v
)

echo
echo "[3/5] Identify CRT, RenderWare, XDK and game functions"
(
    cd "$TOOLKIT_DIR"
    PYTHONPATH="$TOOLKIT_DIR" "$PYTHON_BIN" -m tools.func_id \
        "$XBE_COPY" \
        --functions "$WORK_ROOT/disasm/functions.json" \
        --strings "$WORK_ROOT/disasm/strings.json" \
        --xrefs "$WORK_ROOT/disasm/xrefs.json" \
        --output "$WORK_ROOT/func_id" -v
)

echo
echo "[4/5] Recover function calling conventions and ABI information"
(
    cd "$TOOLKIT_DIR"
    PYTHONPATH="$TOOLKIT_DIR" "$PYTHON_BIN" -m tools.abi_analysis \
        "$XBE_COPY" \
        --functions "$WORK_ROOT/disasm/functions.json" \
        --identified "$WORK_ROOT/func_id/identified_functions.json" \
        --output-dir "$WORK_ROOT/abi" -v
)

echo
echo "[5/5] Recompile Xbox x86 functions into private C source files"
(
    cd "$TOOLKIT_DIR"
    PYTHONPATH="$TOOLKIT_DIR" "$PYTHON_BIN" -m tools.recomp \
        "$XBE_COPY" --game-name "Medal of Honor: European Assault" \
        --all --split 1000 \
        --disasm-dir "$WORK_ROOT/disasm" \
        --func-id-dir "$WORK_ROOT/func_id" \
        --abi-dir "$WORK_ROOT/abi" \
        --gen-dir "$WORK_ROOT/gen" \
        --output-dir "$WORK_ROOT/recomp" \
        --exclude-manual "$REPO_ROOT/source/mygame/src/recomp_manual.c" \
        -v
)

echo
echo "Recompilation pipeline finished."
echo "Private XBE copy:       $XBE_COPY"
echo "Generated C/headers:    $WORK_ROOT/gen"
echo "Analysis reports:       $WORK_ROOT"
echo
echo "IMPORTANT: These files are not yet an Android game build."
echo "Do not commit or upload DEFAULT.XBE, generated C files, or game assets."
echo "The Android runtime still needs its ARM64 graphics/audio/input/platform port."
