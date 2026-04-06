#!/usr/bin/env python3

import sys
import os
import json
import shutil
import subprocess
from pathlib import Path

from PyQt5.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QLabel, QPushButton, QLineEdit, QComboBox, QScrollArea, QFrame,
    QMenu, QAction, QMessageBox, QFileDialog, QSizePolicy, QStackedWidget,
    QAbstractScrollArea, QDialog, QDialogButtonBox, QFormLayout, QSpacerItem,
    QListWidget, QListWidgetItem, QCheckBox, QSplitter, QTextEdit
)
from PyQt5.QtCore import Qt, QThread, pyqtSignal, QProcess, QTimer, QSize
from PyQt5.QtGui import QFont, QFontDatabase, QIcon, QCursor, QColor, QTextCharFormat, QTextCursor

ENGINE_PATH = Path(__file__).parent.parent.resolve()
BUILD_PATH = ENGINE_PATH / "build" / "linux"
EDITOR_BIN = BUILD_PATH / "bin" / "profile" / "Editor"
ASSET_PROCESSOR_BIN = BUILD_PATH / "bin" / "profile" / "AssetProcessor"
MANIFEST_PATH = Path.home() / ".o3de" / "o3de_manifest.json"
O3DE_SCRIPT = ENGINE_PATH / "scripts" / "o3de.py"

APP_QSS = """
* {
    font-family: "Manrope";
    color: #ffffff;
}

QMainWindow, QDialog {
    background: #131313;
}

QWidget#centralWidget {
    background: #131313;
}

QScrollBar:vertical {
    background: #1a1a1a;
    width: 8px;
    border-radius: 0;
    margin: 0;
}
QScrollBar::handle:vertical {
    background: #444444;
    border-radius: 0;
    min-height: 30px;
}
QScrollBar::handle:vertical:hover { background: #555555; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }

QScrollBar:horizontal {
    background: #1a1a1a;
    height: 8px;
}
QScrollBar::handle:horizontal {
    background: #444444;
    min-width: 30px;
}
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }

QScrollArea {
    background: transparent;
    border: none;
}

QWidget#scrollContent {
    background: transparent;
}

QPushButton {
    border-radius: 0;
    border: 1px solid rgba(255,255,255,0.12);
    background: rgba(255,255,255,0.05);
    padding: 6px 16px;
    font-size: 12px;
    color: #ffffff;
}
QPushButton:hover {
    border: 1px solid rgba(255,255,255,0.25);
    background: rgba(255,255,255,0.10);
}
QPushButton:pressed {
    background: rgba(255,255,255,0.03);
}
QPushButton:focus {
    outline: none;
    border: 1px solid #4CAF50;
}
QPushButton:disabled {
    color: #666666;
    border-color: rgba(255,255,255,0.05);
}

QPushButton#primaryButton {
    background: #4CAF50;
    border: 1px solid #4CAF50;
    color: #fff;
    font-weight: 600;
    padding: 6px 18px;
}
QPushButton#primaryButton:hover { background: #66BB6A; border-color: #66BB6A; }
QPushButton#primaryButton:pressed { background: #388E3C; border-color: #388E3C; }
QPushButton#primaryButton:disabled { background: #555555; border-color: #555555; }

QPushButton#menuButton {
    background: #4CAF50;
    border: 1px solid #4CAF50;
    color: #fff;
    font-weight: 600;
    padding: 6px 18px;
}
QPushButton#menuButton:hover { background: #66BB6A; border-color: #66BB6A; }
QPushButton#menuButton::menu-indicator { image: none; width: 0; }

QPushButton#dangerButton {
    background: #E32C27;
    border: 1px solid #E32C27;
    color: #fff;
}
QPushButton#dangerButton:hover { background: #FD3129; }

QPushButton#iconButton {
    background: transparent;
    border: 1px solid rgba(255,255,255,0.08);
    padding: 2px 6px;
    color: #aaaaaa;
    font-size: 16px;
    font-weight: 700;
    min-width: 28px;
    max-width: 28px;
    min-height: 28px;
    max-height: 28px;
}
QPushButton#iconButton:hover {
    border: 1px solid rgba(255,255,255,0.20);
    color: #ffffff;
    background: rgba(255,255,255,0.06);
}

QLineEdit {
    background: rgba(255,255,255,0.06);
    border: 1px solid rgba(255,255,255,0.12);
    border-radius: 0;
    padding: 5px 10px;
    font-size: 13px;
    color: #fff;
    selection-background-color: #4CAF50;
}
QLineEdit:focus {
    border: 1px solid #4CAF50;
    background: rgba(255,255,255,0.08);
}

QComboBox {
    background: rgba(255,255,255,0.06);
    border: 1px solid rgba(255,255,255,0.12);
    border-radius: 0;
    padding: 5px 10px;
    font-size: 12px;
    color: #ddd;
    min-height: 28px;
}
QComboBox:hover { border: 1px solid rgba(255,255,255,0.25); }
QComboBox:focus { border: 1px solid #4CAF50; }
QComboBox::drop-down { border: none; width: 20px; }
QComboBox::down-arrow { image: none; }
QComboBox QAbstractItemView {
    background: #1a1a1a;
    border: 1px solid rgba(255,255,255,0.15);
    selection-background-color: rgba(76,175,80,0.25);
    color: #ddd;
    outline: none;
}

QMenu {
    background: #1e1e1e;
    border: 1px solid rgba(255,255,255,0.15);
    padding: 4px 0;
}
QMenu::item {
    padding: 6px 24px 6px 12px;
    font-size: 12px;
    color: #ccc;
}
QMenu::item:selected {
    background: rgba(76,175,80,0.20);
    color: #fff;
}
QMenu::separator {
    height: 1px;
    background: rgba(255,255,255,0.08);
    margin: 4px 0;
}

QMessageBox {
    background: #1e1e1e;
}

QLabel#windowTitle {
    font-size: 28px;
    font-weight: 700;
    color: #ffffff;
}

QLabel#sectionTitle {
    font-size: 22px;
    font-weight: 700;
    color: #ffffff;
}

QLabel#countBadge {
    font-size: 12px;
    font-weight: 600;
    color: #81C784;
    padding: 3px 10px;
    background: rgba(76,175,80,0.12);
}

QLabel#sortLabel {
    font-size: 12px;
    color: #888888;
}

QFrame#toolbar {
    background: rgba(0,0,0,0.25);
    border-bottom: 1px solid rgba(255,255,255,0.06);
    padding: 4px 0;
}

QFrame#projectRow {
    background: #1a1a1a;
    border: 1px solid rgba(255,255,255,0.06);
    border-radius: 0;
}
QFrame#projectRow:hover {
    background: #222222;
    border: 1px solid rgba(255,255,255,0.10);
}

QLabel#projectName {
    font-size: 15px;
    font-weight: 700;
    color: #ffffff;
}
QLabel#projectPath {
    font-size: 11px;
    color: #888888;
}
QLabel#projectEngine {
    font-size: 11px;
    color: #aaaaaa;
}

QLabel#statusReady {
    font-size: 11px;
    font-weight: 600;
    color: #4CAF50;
    padding: 2px 8px;
    background: rgba(76,175,80,0.12);
}
QLabel#statusBuilding {
    font-size: 11px;
    font-weight: 600;
    color: #FFB74D;
    padding: 2px 8px;
    background: rgba(255,183,77,0.12);
}
QLabel#statusFailed {
    font-size: 11px;
    font-weight: 600;
    color: #EF5350;
    padding: 2px 8px;
    background: rgba(239,83,80,0.12);
}
QLabel#statusNeedsBuild {
    font-size: 11px;
    font-weight: 600;
    color: #FFB74D;
    padding: 2px 8px;
    background: rgba(255,183,77,0.12);
}
QLabel#statusRemote {
    font-size: 11px;
    font-weight: 600;
    color: #64B5F6;
    padding: 2px 8px;
    background: rgba(100,181,246,0.12);
}

QFrame#emptyState {
    background: transparent;
}
QLabel#emptyTitle {
    font-size: 36px;
    font-weight: 700;
    color: #ffffff;
}
QLabel#emptySubtitle {
    font-size: 15px;
    color: #888888;
}
QPushButton#emptyAction {
    background: rgba(0,0,0,0.5);
    border: 1px solid rgba(255,255,255,0.15);
    padding: 16px 24px;
    font-size: 14px;
    font-weight: 600;
    color: #ffffff;
    min-width: 200px;
    min-height: 64px;
}
QPushButton#emptyAction:hover {
    border: 2px solid #4CAF50;
    background: rgba(76,175,80,0.08);
}

QDialog {
    background: #1e1e1e;
}
QDialog QLabel {
    color: #cccccc;
    font-size: 13px;
}
QDialog QLineEdit {
    background: rgba(255,255,255,0.06);
    border: 1px solid rgba(255,255,255,0.15);
    color: #ffffff;
    padding: 6px 10px;
}
QDialog QDialogButtonBox QPushButton {
    min-width: 90px;
}

QTextEdit#buildLog {
    background: #0d0d0d;
    border: 1px solid rgba(255,255,255,0.08);
    color: #cccccc;
    font-family: "Monospace";
    font-size: 9pt;
    padding: 6px;
}

QLabel#buildStatusLabel {
    font-size: 13px;
    font-weight: 600;
    color: #FFB74D;
}

QWidget#tabBar {
    background: #131313;
    border-bottom: 1px solid rgba(255,255,255,0.08);
}
QPushButton#tabButton {
    background: transparent;
    border: none;
    border-bottom: 3px solid transparent;
    padding: 10px 24px;
    font-size: 13px;
    font-weight: 600;
    color: #666666;
    border-radius: 0;
    margin-bottom: -1px;
}
QPushButton#tabButton:hover {
    color: #aaaaaa;
    background: rgba(255,255,255,0.03);
    border-bottom: 3px solid rgba(255,255,255,0.15);
}
QPushButton#tabButton[active=true] {
    color: #ffffff;
    border-bottom: 3px solid #4CAF50;
}

QFrame#engineRow {
    background: #1a1a1a;
    border: 1px solid rgba(255,255,255,0.06);
}
QFrame#engineRow:hover {
    background: #222222;
    border: 1px solid rgba(255,255,255,0.10);
}
QLabel#engineName {
    font-size: 15px;
    font-weight: 700;
    color: #ffffff;
}
QLabel#engineVersion {
    font-size: 11px;
    color: #81C784;
    padding: 2px 8px;
    background: rgba(76,175,80,0.12);
}
QLabel#enginePath {
    font-size: 11px;
    color: #888888;
}
QLabel#engineActiveBadge {
    font-size: 11px;
    font-weight: 600;
    color: #64B5F6;
    padding: 2px 8px;
    background: rgba(100,181,246,0.12);
}
"""


def read_manifest():
    if not MANIFEST_PATH.exists():
        return {"projects": [], "engines": [], "engines_path": {}}
    with open(MANIFEST_PATH, "r", encoding="utf-8") as f:
        return json.load(f)


def write_manifest(data):
    with open(MANIFEST_PATH, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=4)


def read_project_json(project_path: Path) -> dict:
    pjson = project_path / "project.json"
    if not pjson.exists():
        return {}
    try:
        with open(pjson, "r", encoding="utf-8") as f:
            return json.load(f)
    except Exception:
        return {}


def read_engine_json(engine_path: Path) -> dict:
    ejson = engine_path / "engine.json"
    if not ejson.exists():
        return {}
    try:
        with open(ejson, "r", encoding="utf-8") as f:
            return json.load(f)
    except Exception:
        return {}


def get_all_projects():
    manifest = read_manifest()
    projects = []
    for p in manifest.get("projects", []):
        path = Path(p)
        data = read_project_json(path)
        if not data and not path.exists():
            continue
        projects.append({
            "path": str(path),
            "name": data.get("display_name") or data.get("project_name") or path.name,
            "version": data.get("version", ""),
            "engine_name": data.get("engine") or data.get("engine_name", ""),
            "needs_build": not (path / "build").exists(),
            "remote": False,
            "raw": data,
        })
    return projects


def get_engine_info(engine_path_str: str) -> dict:
    ep = Path(engine_path_str)
    data = read_engine_json(ep)
    return {
        "name": data.get("engine_name", ep.name),
        "version": data.get("version", ""),
    }


def find_engine_for_project(project_info: dict) -> dict:
    manifest = read_manifest()
    for epath in manifest.get("engines", []):
        edata = read_engine_json(Path(epath))
        eng_name = edata.get("engine_name", "")
        if eng_name and eng_name == project_info.get("engine_name", ""):
            return {"name": eng_name, "version": edata.get("version", ""), "path": epath}
    engines_path = manifest.get("engines_path", {})
    if engines_path:
        first_name = next(iter(engines_path))
        first_path = engines_path[first_name]
        edata = read_engine_json(Path(first_path))
        return {"name": first_name, "version": edata.get("version", ""), "path": first_path}
    return {"name": "Unknown Engine", "version": "", "path": ""}


class BuildWorker(QThread):
    output_line = pyqtSignal(str)
    finished = pyqtSignal(bool)

    def __init__(self, project_path: str, project_name: str):
        super().__init__()
        self._project_path = project_path
        self._project_name = project_name
        self._current_proc = None
        self._cancelled = False

    def cancel(self):
        self._cancelled = True
        if self._current_proc and self._current_proc.poll() is None:
            self._current_proc.terminate()

    def _run_cmd(self, cmd: list) -> int:
        self._current_proc = subprocess.Popen(
            cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            text=True, bufsize=1
        )
        for line in self._current_proc.stdout:
            if self._cancelled:
                break
            self.output_line.emit(line.rstrip())
        self._current_proc.wait()
        return self._current_proc.returncode

    def run(self):
        import multiprocessing
        j = multiprocessing.cpu_count()
        engine_build = str(BUILD_PATH)
        engine_src = str(ENGINE_PATH)

        configure_cmd = [
            "cmake", "-B", engine_build, "-S", engine_src,
            f"-DLY_PROJECTS={self._project_path}"
        ]
        try:
            self.output_line.emit("=== Configuring project ===")
            rc = self._run_cmd(configure_cmd)
            if rc != 0 or self._cancelled:
                self.finished.emit(False)
                return

            launcher_target = f"{self._project_name}.GameLauncher"
            build_cmd = [
                "cmake", "--build", engine_build,
                "--config", "profile",
                "--target", launcher_target, "Editor",
                "--", f"-j{j}"
            ]
            self.output_line.emit("=== Building targets ===")
            rc = self._run_cmd(build_cmd)
            self.finished.emit(rc == 0 and not self._cancelled)
        except Exception as e:
            self.output_line.emit(f"Error: {e}")
            self.finished.emit(False)


class EngineBuildWorker(QThread):
    output_line = pyqtSignal(str)
    finished = pyqtSignal(bool)

    def __init__(self, targets: list = None):
        super().__init__()
        self._targets = targets or ["Editor", "AssetProcessor"]
        self._current_proc = None
        self._cancelled = False

    def cancel(self):
        self._cancelled = True
        if self._current_proc and self._current_proc.poll() is None:
            self._current_proc.terminate()

    def run(self):
        import multiprocessing
        j = multiprocessing.cpu_count()
        build_cmd = [
            "cmake", "--build", str(BUILD_PATH),
            "--config", "profile",
            "--target", *self._targets,
            "--", f"-j{j}"
        ]
        try:
            self.output_line.emit(f"=== Building Engine ({', '.join(self._targets)}) ===")
            self._current_proc = subprocess.Popen(
                build_cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                text=True, bufsize=1
            )
            for line in self._current_proc.stdout:
                if self._cancelled:
                    break
                self.output_line.emit(line.rstrip())
            self._current_proc.wait()
            self.finished.emit(self._current_proc.returncode == 0 and not self._cancelled)
        except Exception as e:
            self.output_line.emit(f"Error: {e}")
            self.finished.emit(False)


class BuildOutputDialog(QDialog):
    def __init__(self, project_name: str, worker: 'BuildWorker', parent=None):
        super().__init__(parent)
        self.setWindowTitle(f"Building — {project_name}")
        self.setMinimumSize(QSize(860, 520))
        self.setModal(False)
        self._worker = worker
        self._finished = False

        root = QVBoxLayout(self)
        root.setSpacing(10)
        root.setContentsMargins(16, 14, 16, 14)

        header = QHBoxLayout()
        self._status_lbl = QLabel("Configuring…")
        self._status_lbl.setObjectName("buildStatusLabel")
        header.addWidget(self._status_lbl)
        header.addStretch()
        root.addLayout(header)

        self._log = QTextEdit()
        self._log.setReadOnly(True)
        self._log.setObjectName("buildLog")
        self._log.setFont(QFont("Monospace", 9))
        root.addWidget(self._log)

        footer = QHBoxLayout()
        footer.addStretch()
        self._stop_btn = QPushButton("Stop Build")
        self._stop_btn.setObjectName("dangerButton")
        self._stop_btn.clicked.connect(self._cancel)
        footer.addWidget(self._stop_btn)
        self._close_btn = QPushButton("Close")
        self._close_btn.setEnabled(False)
        self._close_btn.clicked.connect(self.accept)
        footer.addWidget(self._close_btn)
        root.addLayout(footer)

        worker.output_line.connect(self.append_line)
        worker.finished.connect(self._on_finished)

    def append_line(self, line: str):
        cursor = self._log.textCursor()
        cursor.movePosition(QTextCursor.End)
        fmt = QTextCharFormat()
        low = line.lower()
        if line.startswith("==="):
            fmt.setForeground(QColor("#4CAF50"))
        elif "error:" in low or low.startswith("error"):
            fmt.setForeground(QColor("#ef5350"))
        elif "warning:" in low:
            fmt.setForeground(QColor("#FFA726"))
        else:
            fmt.setForeground(QColor("#cccccc"))
        cursor.setCharFormat(fmt)
        cursor.insertText(line + "\n")
        self._log.setTextCursor(cursor)
        self._log.ensureCursorVisible()
        if "=== Building" in line:
            self._status_lbl.setText("Building…")

    def _on_finished(self, success: bool):
        self._finished = True
        self._stop_btn.hide()
        self._close_btn.setEnabled(True)
        if success:
            self._status_lbl.setText("Build complete ✓")
            self._status_lbl.setStyleSheet("color: #4CAF50;")
        else:
            self._status_lbl.setText("Build failed ✗")
            self._status_lbl.setStyleSheet("color: #ef5350;")

    def _cancel(self):
        self._stop_btn.setEnabled(False)
        self._stop_btn.hide()
        self._status_lbl.setText("Cancelling…")
        self._worker.cancel()

    def closeEvent(self, event):
        if not self._finished:
            event.ignore()
        else:
            super().closeEvent(event)


def get_all_available_gems() -> list[dict]:
    results = []
    gems_root = ENGINE_PATH / "Gems"
    if not gems_root.exists():
        return results
    for gem_json_path in sorted(gems_root.rglob("gem.json")):
        try:
            data = json.loads(gem_json_path.read_text(encoding="utf-8"))
            name = data.get("gem_name", "")
            if not name:
                continue
            results.append({
                "name": name,
                "display_name": data.get("display_name", name),
                "summary": data.get("summary", ""),
                "tags": data.get("tags", []),
                "path": str(gem_json_path.parent),
            })
        except Exception:
            continue
    return results


class PluginConfigDialog(QDialog):
    def __init__(self, project: dict, parent=None):
        super().__init__(parent)
        self.setWindowTitle(f"Configure Plugins — {project['name']}")
        self.setMinimumSize(QSize(820, 580))
        self.setModal(True)
        self._project = project
        self._project_path = Path(project["path"])
        self._enabled_names: set = set()
        self._all_gems: list[dict] = []
        self._pending_enable: set = set()
        self._pending_disable: set = set()
        self._setup_ui()
        self._load_data()

    def _setup_ui(self):
        self.setStyleSheet("""
            QDialog { background: #1a1a1a; }
            QLabel { color: #cccccc; }
            QLabel#dlgTitle { font-size: 16px; font-weight: 700; color: #ffffff; }
            QLabel#dlgSub { font-size: 12px; color: #888888; }
            QLineEdit {
                background: rgba(255,255,255,0.06);
                border: 1px solid rgba(255,255,255,0.12);
                border-radius: 0;
                padding: 5px 10px;
                font-size: 12px;
                color: #fff;
            }
            QLineEdit:focus { border: 1px solid #4CAF50; }
            QListWidget {
                background: #111111;
                border: 1px solid rgba(255,255,255,0.08);
                border-radius: 0;
                outline: none;
                font-size: 12px;
                color: #cccccc;
            }
            QListWidget::item { padding: 8px 12px; border-bottom: 1px solid rgba(255,255,255,0.04); }
            QListWidget::item:selected { background: rgba(76,175,80,0.20); color: #ffffff; }
            QListWidget::item:hover { background: rgba(255,255,255,0.05); }
            QTextEdit {
                background: #111111;
                border: 1px solid rgba(255,255,255,0.08);
                border-radius: 0;
                font-size: 12px;
                color: #aaaaaa;
                padding: 8px;
            }
            QPushButton {
                border-radius: 0;
                border: 1px solid rgba(255,255,255,0.12);
                background: rgba(255,255,255,0.05);
                padding: 5px 14px;
                font-size: 12px;
                color: #ffffff;
            }
            QPushButton:hover { border: 1px solid rgba(255,255,255,0.25); background: rgba(255,255,255,0.10); }
            QPushButton:disabled { color: #555555; border-color: rgba(255,255,255,0.05); }
            QPushButton#enableBtn { background: rgba(76,175,80,0.15); border-color: #4CAF50; color: #81C784; }
            QPushButton#enableBtn:hover { background: rgba(76,175,80,0.30); }
            QPushButton#disableBtn { background: rgba(239,83,80,0.15); border-color: #EF5350; color: #EF9A9A; }
            QPushButton#disableBtn:hover { background: rgba(239,83,80,0.30); }
            QPushButton#applyBtn { background: #4CAF50; border-color: #4CAF50; color: #fff; font-weight: 600; min-width: 90px; }
            QPushButton#applyBtn:hover { background: #66BB6A; }
            QPushButton#applyBtn:disabled { background: #555; border-color: #555; color: #888; }
            QLabel#statusEnabled { color: #4CAF50; font-size: 11px; font-weight: 600; }
            QLabel#statusDisabled { color: #888888; font-size: 11px; }
            QLabel#countLabel { font-size: 11px; color: #888888; }
        """)

        root = QVBoxLayout(self)
        root.setContentsMargins(20, 20, 20, 16)
        root.setSpacing(12)

        title = QLabel("Configure Plugins")
        title.setObjectName("dlgTitle")
        root.addWidget(title)

        sub = QLabel(f"Enable or disable plugins for: {self._project['path']}")
        sub.setObjectName("dlgSub")
        root.addWidget(sub)

        splitter = QSplitter(Qt.Horizontal)
        splitter.setHandleWidth(4)
        splitter.setStyleSheet("QSplitter::handle { background: rgba(255,255,255,0.06); }")

        left = QWidget()
        left_layout = QVBoxLayout(left)
        left_layout.setContentsMargins(0, 0, 0, 0)
        left_layout.setSpacing(8)

        search_row = QHBoxLayout()
        self._search = QLineEdit()
        self._search.setPlaceholderText("Search plugins…")
        self._search.setClearButtonEnabled(True)
        self._search.textChanged.connect(self._apply_filter)
        search_row.addWidget(self._search)

        self._filter_combo = QComboBox()
        self._filter_combo.addItem("All")
        self._filter_combo.addItem("Enabled")
        self._filter_combo.addItem("Disabled")
        self._filter_combo.setFixedWidth(110)
        self._filter_combo.setStyleSheet("""
            QComboBox { background: rgba(255,255,255,0.06); border: 1px solid rgba(255,255,255,0.12);
                        border-radius: 0; padding: 5px 10px; font-size: 12px; color: #ddd; min-height: 28px; }
            QComboBox::drop-down { border: none; width: 16px; }
            QComboBox QAbstractItemView { background: #1a1a1a; border: 1px solid rgba(255,255,255,0.15);
                                          selection-background-color: rgba(76,175,80,0.25); color: #ddd; }
        """)
        self._filter_combo.currentIndexChanged.connect(self._apply_filter)
        search_row.addWidget(self._filter_combo)
        left_layout.addLayout(search_row)

        self._list = QListWidget()
        self._list.setSelectionMode(QListWidget.SingleSelection)
        self._list.currentItemChanged.connect(self._on_selection_changed)
        left_layout.addWidget(self._list)

        self._count_label = QLabel("0 plugins")
        self._count_label.setObjectName("countLabel")
        left_layout.addWidget(self._count_label)

        right = QWidget()
        right_layout = QVBoxLayout(right)
        right_layout.setContentsMargins(8, 0, 0, 0)
        right_layout.setSpacing(10)

        self._detail_name = QLabel("Select a plugin")
        self._detail_name.setStyleSheet("font-size: 15px; font-weight: 700; color: #ffffff;")
        self._detail_name.setWordWrap(True)
        right_layout.addWidget(self._detail_name)

        self._detail_status = QLabel("")
        right_layout.addWidget(self._detail_status)

        self._detail_text = QTextEdit()
        self._detail_text.setReadOnly(True)
        self._detail_text.setMinimumHeight(100)
        right_layout.addWidget(self._detail_text)

        action_row = QHBoxLayout()
        self._enable_btn = QPushButton("Enable Plugin")
        self._enable_btn.setObjectName("enableBtn")
        self._enable_btn.setEnabled(False)
        self._enable_btn.clicked.connect(self._toggle_enable)
        action_row.addWidget(self._enable_btn)
        action_row.addStretch()
        right_layout.addLayout(action_row)

        right_layout.addStretch()

        splitter.addWidget(left)
        splitter.addWidget(right)
        splitter.setSizes([420, 360])
        root.addWidget(splitter)

        btn_row = QHBoxLayout()
        btn_row.addStretch()

        self._pending_label = QLabel("")
        self._pending_label.setObjectName("countLabel")
        btn_row.addWidget(self._pending_label)

        cancel_btn = QPushButton("Cancel")
        cancel_btn.clicked.connect(self.reject)
        btn_row.addWidget(cancel_btn)

        self._apply_btn = QPushButton("Apply")
        self._apply_btn.setObjectName("applyBtn")
        self._apply_btn.setEnabled(False)
        self._apply_btn.clicked.connect(self._apply_changes)
        btn_row.addWidget(self._apply_btn)

        root.addLayout(btn_row)

    def _load_data(self):
        pjson_path = self._project_path / "project.json"
        if pjson_path.exists():
            try:
                data = json.loads(pjson_path.read_text(encoding="utf-8"))
                self._enabled_names = set(data.get("gem_names", []))
            except Exception:
                self._enabled_names = set()

        self._all_gems = get_all_available_gems()
        self._apply_filter()

    def _apply_filter(self):
        text = self._search.text().lower()
        mode = self._filter_combo.currentText()

        self._list.clear()
        visible = 0
        for gem in self._all_gems:
            is_enabled = gem["name"] in self._enabled_names
            if mode == "Enabled" and not is_enabled:
                continue
            if mode == "Disabled" and is_enabled:
                continue
            if text and text not in gem["name"].lower() and text not in gem["display_name"].lower():
                continue

            display = gem["display_name"] or gem["name"]
            status = "  ✓" if is_enabled else ""
            item = QListWidgetItem(f"{display}{status}")
            item.setData(Qt.UserRole, gem)
            if is_enabled:
                item.setForeground(__import__("PyQt5.QtGui", fromlist=["QColor"]).QColor("#81C784"))
            self._list.addItem(item)
            visible += 1

        self._count_label.setText(f"{visible} of {len(self._all_gems)} plugins")

    def _on_selection_changed(self, current, _previous):
        if not current:
            self._enable_btn.setEnabled(False)
            return

        gem = current.data(Qt.UserRole)
        is_enabled = gem["name"] in self._enabled_names

        self._detail_name.setText(gem["display_name"] or gem["name"])

        if is_enabled:
            self._detail_status.setObjectName("statusEnabled")
            self._detail_status.setText("● Enabled")
        else:
            self._detail_status.setObjectName("statusDisabled")
            self._detail_status.setText("○ Disabled")
        self._detail_status.style().unpolish(self._detail_status)
        self._detail_status.style().polish(self._detail_status)

        lines = []
        if gem["summary"]:
            lines.append(gem["summary"])
        if gem["tags"]:
            lines.append(f"\nTags: {', '.join(gem['tags'])}")
        lines.append(f"\nPath: {gem['path']}")
        self._detail_text.setPlainText("\n".join(lines))

        self._enable_btn.setEnabled(True)
        if is_enabled:
            self._enable_btn.setText("Disable Plugin")
            self._enable_btn.setObjectName("disableBtn")
        else:
            self._enable_btn.setText("Enable Plugin")
            self._enable_btn.setObjectName("enableBtn")
        self._enable_btn.style().unpolish(self._enable_btn)
        self._enable_btn.style().polish(self._enable_btn)

    def _toggle_enable(self):
        item = self._list.currentItem()
        if not item:
            return
        gem = item.data(Qt.UserRole)
        name = gem["name"]
        if name in self._enabled_names:
            self._enabled_names.discard(name)
            self._pending_disable.add(name)
            self._pending_enable.discard(name)
        else:
            self._enabled_names.add(name)
            self._pending_enable.add(name)
            self._pending_disable.discard(name)

        total = len(self._pending_enable) + len(self._pending_disable)
        self._pending_label.setText(f"{total} pending change{'s' if total != 1 else ''}" if total else "")
        self._apply_btn.setEnabled(total > 0)

        self._apply_filter()
        self._on_selection_changed(self._list.currentItem(), None)

    def _apply_changes(self):
        pjson_path = self._project_path / "project.json"
        try:
            data = json.loads(pjson_path.read_text(encoding="utf-8"))
            current = set(data.get("gem_names", []))
            current |= self._pending_enable
            current -= self._pending_disable
            data["gem_names"] = sorted(current)
            pjson_path.write_text(json.dumps(data, indent=4), encoding="utf-8")
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Failed to save project.json:\n{e}")
            return

        enabled_count = len(self._pending_enable)
        disabled_count = len(self._pending_disable)
        parts = []
        if enabled_count:
            parts.append(f"{enabled_count} plugin{'s' if enabled_count != 1 else ''} enabled")
        if disabled_count:
            parts.append(f"{disabled_count} plugin{'s' if disabled_count != 1 else ''} disabled")

        self._pending_enable.clear()
        self._pending_disable.clear()
        self._apply_btn.setEnabled(False)
        self._pending_label.setText("")

        QMessageBox.information(self, "Plugins Updated",
                                f"{', '.join(parts)} successfully.\n\n"
                                "Rebuild the project for changes to take effect.")
        self.accept()


class NewProjectDialog(QDialog):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Create New Project")
        self.setMinimumWidth(480)
        self.setModal(True)

        layout = QVBoxLayout(self)
        layout.setSpacing(12)
        layout.setContentsMargins(24, 24, 24, 24)

        form = QFormLayout()
        form.setSpacing(10)
        form.setLabelAlignment(Qt.AlignRight)

        self._name_edit = QLineEdit()
        self._name_edit.setPlaceholderText("MyProject")
        form.addRow("Project Name:", self._name_edit)

        path_row = QHBoxLayout()
        self._path_edit = QLineEdit()
        self._path_edit.setPlaceholderText("/path/to/projects/MyProject")
        browse_btn = QPushButton("Browse…")
        browse_btn.clicked.connect(self._browse_path)
        path_row.addWidget(self._path_edit)
        path_row.addWidget(browse_btn)
        form.addRow("Project Path:", path_row)

        layout.addLayout(form)

        btns = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
        btns.button(QDialogButtonBox.Ok).setObjectName("primaryButton")
        btns.button(QDialogButtonBox.Ok).setText("Create")
        btns.accepted.connect(self._validate_and_accept)
        btns.rejected.connect(self.reject)
        layout.addWidget(btns)

    def _browse_path(self):
        folder = QFileDialog.getExistingDirectory(self, "Choose Project Folder")
        if folder:
            self._path_edit.setText(folder)
            if not self._name_edit.text():
                self._name_edit.setText(Path(folder).name)

    def _validate_and_accept(self):
        if not self._name_edit.text().strip():
            QMessageBox.warning(self, "Missing Name", "Please enter a project name.")
            return
        if not self._path_edit.text().strip():
            QMessageBox.warning(self, "Missing Path", "Please choose a project path.")
            return
        self.accept()

    def get_name(self) -> str:
        return self._name_edit.text().strip()

    def get_path(self) -> str:
        return self._path_edit.text().strip()


class ProjectRow(QFrame):
    open_editor_requested = pyqtSignal(dict)
    build_requested = pyqtSignal(dict)
    edit_settings_requested = pyqtSignal(dict)
    edit_gems_requested = pyqtSignal(dict)
    remove_requested = pyqtSignal(dict)
    delete_requested = pyqtSignal(dict)
    open_folder_requested = pyqtSignal(dict)
    launch_asset_processor_requested = pyqtSignal(dict)
    duplicate_requested = pyqtSignal(dict)

    def __init__(self, project: dict, engine: dict, parent=None):
        super().__init__(parent)
        self.setObjectName("projectRow")
        self.setAttribute(Qt.WA_Hover, True)
        self._project = project
        self._engine = engine
        self._status = "ready" if not project.get("needs_build") else "needs_build"
        self._build_worker = None
        self._setup_ui()

    def _setup_ui(self):
        outer = QHBoxLayout(self)
        outer.setContentsMargins(16, 14, 12, 14)
        outer.setSpacing(12)

        left = QVBoxLayout()
        left.setSpacing(3)

        name_row = QHBoxLayout()
        name_row.setSpacing(10)
        name_row.setContentsMargins(0, 0, 0, 0)

        name_text = self._project["name"]
        if self._project.get("version"):
            name_text += f"  v{self._project['version']}"

        self._name_label = QLabel(name_text)
        self._name_label.setObjectName("projectName")
        name_row.addWidget(self._name_label)

        self._status_label = QLabel()
        self._update_status_label()
        name_row.addWidget(self._status_label)
        name_row.addStretch()
        left.addLayout(name_row)

        self._path_label = QLabel(self._project["path"])
        self._path_label.setObjectName("projectPath")
        self._path_label.setToolTip(self._project["path"])
        left.addWidget(self._path_label)

        engine_text = self._engine.get("name", "")
        if self._engine.get("version"):
            engine_text += f"  {self._engine['version']}"
        self._engine_label = QLabel(engine_text)
        self._engine_label.setObjectName("projectEngine")
        left.addWidget(self._engine_label)

        outer.addLayout(left, 1)

        right = QHBoxLayout()
        right.setSpacing(8)
        right.setAlignment(Qt.AlignVCenter | Qt.AlignRight)

        self._open_btn = QPushButton("Open Editor")
        self._open_btn.setObjectName("primaryButton")
        self._open_btn.setFixedWidth(108)
        self._open_btn.clicked.connect(lambda: self.open_editor_requested.emit(self._project))
        right.addWidget(self._open_btn)

        self._menu_btn = QPushButton("⋯")
        self._menu_btn.setObjectName("iconButton")
        self._menu_btn.setToolTip("More actions")
        self._menu_btn.clicked.connect(self._show_context_menu)
        right.addWidget(self._menu_btn)

        outer.addLayout(right)
        self.setMinimumHeight(80)

    def _update_status_label(self):
        status_map = {
            "ready": ("Ready", "statusReady"),
            "needs_build": ("Build Required", "statusNeedsBuild"),
            "building": ("Building…", "statusBuilding"),
            "failed": ("Build Failed", "statusFailed"),
            "launching": ("Opening…", "statusBuilding"),
            "remote": ("Remote", "statusRemote"),
        }
        text, obj = status_map.get(self._status, ("Ready", "statusReady"))
        self._status_label.setText(text)
        self._status_label.setObjectName(obj)
        self._status_label.style().unpolish(self._status_label)
        self._status_label.style().polish(self._status_label)

    def set_status(self, status: str):
        self._status = status
        self._update_status_label()
        is_building = status in ("building", "launching")
        self._open_btn.setEnabled(not is_building)
        self._menu_btn.setEnabled(not is_building)

    def get_project(self) -> dict:
        return self._project

    def _show_context_menu(self):
        menu = QMenu(self)
        menu.addAction("Open Editor", lambda: self.open_editor_requested.emit(self._project))
        menu.addSeparator()
        menu.addAction("Edit Project Settings…", lambda: self.edit_settings_requested.emit(self._project))
        menu.addAction("Configure Plugins…", lambda: self.edit_gems_requested.emit(self._project))
        menu.addSeparator()
        menu.addAction("Build", lambda: self.build_requested.emit(self._project))
        export_menu = menu.addMenu("Export Launcher")
        export_menu.addAction("Linux", lambda: self._export("export_source_built_project.py"))
        export_menu.addAction("Android", lambda: self._export("export_source_android.py"))
        menu.addSeparator()
        menu.addAction("Launch Asset Processor", lambda: self.launch_asset_processor_requested.emit(self._project))
        menu.addSeparator()
        menu.addAction("Open Project Folder…", lambda: self.open_folder_requested.emit(self._project))
        menu.addSeparator()
        menu.addAction("Duplicate", lambda: self.duplicate_requested.emit(self._project))
        menu.addSeparator()
        menu.addAction("Remove from Engine", lambda: self.remove_requested.emit(self._project))
        menu.addAction("Delete Project", lambda: self.delete_requested.emit(self._project))
        menu.exec_(QCursor.pos())

    def _export(self, script_name: str):
        script = ENGINE_PATH / "scripts" / script_name
        if not script.exists():
            QMessageBox.warning(self, "Not Found", f"Export script not found:\n{script}")
            return
        subprocess.Popen(
            ["python3", str(script), "--project-path", self._project["path"]],
            cwd=str(ENGINE_PATH)
        )


class ProjectsListWidget(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setObjectName("scrollContent")
        self._layout = QVBoxLayout(self)
        self._layout.setSpacing(4)
        self._layout.setContentsMargins(0, 0, 0, 0)
        self._layout.setAlignment(Qt.AlignTop)
        self._rows: list[ProjectRow] = []

    def clear_rows(self):
        for row in self._rows:
            self._layout.removeWidget(row)
            row.deleteLater()
        self._rows.clear()

    def add_row(self, row: ProjectRow):
        self._rows.append(row)
        self._layout.addWidget(row)

    def get_rows(self) -> list:
        return list(self._rows)


class ProjectManagerLite(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Dusk Engine — Project Manager")
        self.setMinimumSize(QSize(1000, 640))
        self.resize(1200, 760)

        self._all_projects: list[dict] = []
        self._search_text = ""
        self._sort_order = 0
        self._building_paths: set = set()
        self._build_workers: dict = {}

        central = QWidget()
        central.setObjectName("centralWidget")
        self.setCentralWidget(central)

        root = QVBoxLayout(central)
        root.setContentsMargins(0, 0, 0, 0)
        root.setSpacing(0)

        root.addWidget(self._build_tab_bar())

        self._tab_stack = QStackedWidget()

        # ── Projects tab ──────────────────────────────────────────────
        projects_tab = QWidget()
        projects_tab.setObjectName("centralWidget")
        ptab_layout = QVBoxLayout(projects_tab)
        ptab_layout.setContentsMargins(48, 32, 48, 24)
        ptab_layout.setSpacing(0)
        ptab_layout.addWidget(self._build_header())
        ptab_layout.addSpacing(20)

        self._stack = QStackedWidget()
        self._empty_page = self._build_empty_page()
        self._projects_page = self._build_projects_page()
        self._stack.addWidget(self._empty_page)
        self._stack.addWidget(self._projects_page)
        ptab_layout.addWidget(self._stack)

        # ── Engine tab ────────────────────────────────────────────────
        engine_tab = self._build_engine_page()

        self._tab_stack.addWidget(projects_tab)   # index 0
        self._tab_stack.addWidget(engine_tab)     # index 1
        root.addWidget(self._tab_stack)

        self._load_projects()

    def _build_tab_bar(self) -> QWidget:
        bar = QWidget()
        bar.setObjectName("tabBar")
        layout = QHBoxLayout(bar)
        layout.setContentsMargins(48, 0, 48, 0)
        layout.setSpacing(0)

        self._tab_projects_btn = QPushButton("Projects")
        self._tab_projects_btn.setObjectName("tabButton")
        self._tab_projects_btn.setProperty("active", True)
        self._tab_projects_btn.setFixedHeight(44)
        self._tab_projects_btn.clicked.connect(lambda: self._switch_tab(0))
        layout.addWidget(self._tab_projects_btn)

        self._tab_engine_btn = QPushButton("Engine")
        self._tab_engine_btn.setObjectName("tabButton")
        self._tab_engine_btn.setProperty("active", False)
        self._tab_engine_btn.setFixedHeight(44)
        self._tab_engine_btn.clicked.connect(lambda: self._switch_tab(1))
        layout.addWidget(self._tab_engine_btn)

        layout.addStretch()
        return bar

    def _switch_tab(self, index: int):
        self._tab_stack.setCurrentIndex(index)
        self._tab_projects_btn.setProperty("active", index == 0)
        self._tab_engine_btn.setProperty("active", index == 1)
        for btn in (self._tab_projects_btn, self._tab_engine_btn):
            btn.style().unpolish(btn)
            btn.style().polish(btn)
        if index == 1:
            self._load_engines()

    def _build_engine_page(self) -> QWidget:
        page = QWidget()
        page.setObjectName("centralWidget")
        layout = QVBoxLayout(page)
        layout.setContentsMargins(48, 32, 48, 24)
        layout.setSpacing(0)

        # Header
        header = QWidget()
        hlayout = QHBoxLayout(header)
        hlayout.setContentsMargins(0, 0, 0, 0)
        hlayout.setSpacing(16)
        title = QLabel("Engines")
        title.setObjectName("sectionTitle")
        hlayout.addWidget(title)
        hlayout.addStretch()
        add_btn = QPushButton("Add Engine")
        add_btn.setObjectName("primaryButton")
        add_btn.setFixedHeight(34)
        add_btn.clicked.connect(self._handle_add_engine)
        hlayout.addWidget(add_btn)
        layout.addWidget(header)
        layout.addSpacing(20)

        # Scrollable engine list
        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        scroll.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        self._engines_list_widget = QWidget()
        self._engines_list_widget.setObjectName("scrollContent")
        self._engines_list_layout = QVBoxLayout(self._engines_list_widget)
        self._engines_list_layout.setSpacing(4)
        self._engines_list_layout.setContentsMargins(0, 0, 0, 0)
        self._engines_list_layout.setAlignment(Qt.AlignTop)
        scroll.setWidget(self._engines_list_widget)
        layout.addWidget(scroll)

        return page

    def _load_engines(self):
        while self._engines_list_layout.count():
            item = self._engines_list_layout.takeAt(0)
            if item.widget():
                item.widget().deleteLater()

        manifest = read_manifest()
        engines = manifest.get("engines", [])

        if not engines:
            empty = QLabel(
                "No engines registered.\n\n"
                "Click 'Add Engine' to register an engine installation."
            )
            empty.setAlignment(Qt.AlignCenter)
            empty.setStyleSheet("color: #555555; font-size: 14px; padding: 48px;")
            self._engines_list_layout.addWidget(empty)
            return

        for epath in engines:
            self._engines_list_layout.addWidget(self._build_engine_row(epath))

    def _build_engine_row(self, engine_path: str) -> QFrame:
        row = QFrame()
        row.setObjectName("engineRow")
        row.setMinimumHeight(80)

        outer = QHBoxLayout(row)
        outer.setContentsMargins(16, 14, 12, 14)
        outer.setSpacing(12)

        left = QVBoxLayout()
        left.setSpacing(4)

        # Name + badges row
        name_row = QHBoxLayout()
        name_row.setSpacing(10)
        name_row.setContentsMargins(0, 0, 0, 0)

        edata = read_engine_json(Path(engine_path))
        name = edata.get("engine_name", Path(engine_path).name)
        version = edata.get("version", "")
        is_current = Path(engine_path).resolve() == ENGINE_PATH.resolve()

        name_lbl = QLabel(name)
        name_lbl.setObjectName("engineName")
        name_row.addWidget(name_lbl)

        if version:
            ver_lbl = QLabel(f"v{version}")
            ver_lbl.setObjectName("engineVersion")
            name_row.addWidget(ver_lbl)

        if is_current:
            active_lbl = QLabel("Active")
            active_lbl.setObjectName("engineActiveBadge")
            name_row.addWidget(active_lbl)

        name_row.addStretch()
        left.addLayout(name_row)

        path_lbl = QLabel(engine_path)
        path_lbl.setObjectName("enginePath")
        path_lbl.setToolTip(engine_path)
        left.addWidget(path_lbl)

        outer.addLayout(left, 1)

        btn_row = QHBoxLayout()
        btn_row.setSpacing(8)

        if is_current:
            build_menu = QMenu()
            build_menu.addAction("Editor + AssetProcessor",
                lambda: self._handle_build_engine(["Editor", "AssetProcessor"]))
            build_menu.addAction("Editor only",
                lambda: self._handle_build_engine(["Editor"]))
            build_menu.addAction("AssetProcessor only",
                lambda: self._handle_build_engine(["AssetProcessor"]))
            build_btn = QPushButton("Build Engine  ▾")
            build_btn.setObjectName("menuButton")
            build_btn.setMenu(build_menu)
            build_btn.setFixedHeight(30)
            btn_row.addWidget(build_btn)

        more_btn = QPushButton("⋯")
        more_btn.setObjectName("iconButton")
        more_btn.setToolTip("More options")
        ep_capture = engine_path
        is_current_capture = is_current
        more_btn.clicked.connect(
            lambda _=False, ep=ep_capture, active=is_current_capture:
                self._show_engine_menu(ep, active)
        )
        btn_row.addWidget(more_btn)

        outer.addLayout(btn_row)
        return row

    def _handle_add_engine(self):
        folder = QFileDialog.getExistingDirectory(self, "Select Engine Folder")
        if not folder:
            return
        if not (Path(folder) / "engine.json").exists():
            QMessageBox.warning(
                self, "Not an Engine",
                "No engine.json found in the selected folder.\n\n"
                "Please select the root directory of an O3DE/Dusk Engine installation."
            )
            return
        manifest = read_manifest()
        engines = manifest.get("engines", [])
        if folder not in engines:
            engines.append(folder)
            manifest["engines"] = engines
            write_manifest(manifest)
        self._load_engines()

    def _handle_build_engine(self, targets: list):
        worker = EngineBuildWorker(targets)
        dlg = BuildOutputDialog("Dusk Engine", worker, self)
        dlg.setWindowTitle(f"Building Engine — {', '.join(targets)}")
        dlg.show()
        worker.start()

    def _show_engine_menu(self, engine_path: str, is_current: bool):
        menu = QMenu(self)
        if is_current:
            build_sub = menu.addMenu("Build Engine")
            build_sub.addAction("Editor + AssetProcessor",
                lambda: self._handle_build_engine(["Editor", "AssetProcessor"]))
            build_sub.addAction("Editor only",
                lambda: self._handle_build_engine(["Editor"]))
            build_sub.addAction("AssetProcessor only",
                lambda: self._handle_build_engine(["AssetProcessor"]))
            menu.addSeparator()
            open_folder = menu.addAction("Open Engine Folder…")
            open_folder.triggered.connect(lambda: subprocess.Popen(["xdg-open", engine_path]))
        else:
            remove_act = menu.addAction("Remove from registry")
            remove_act.triggered.connect(lambda: self._handle_remove_engine(engine_path))
        menu.exec_(QCursor.pos())

    def _handle_remove_engine(self, engine_path: str):
        reply = QMessageBox.question(
            self, "Remove Engine",
            f"Remove this engine from the registry?\n\nPath: {engine_path}\n\n"
            "The engine files will NOT be deleted.",
            QMessageBox.Yes | QMessageBox.No
        )
        if reply != QMessageBox.Yes:
            return
        manifest = read_manifest()
        manifest["engines"] = [e for e in manifest.get("engines", []) if e != engine_path]
        write_manifest(manifest)
        self._load_engines()

    def _build_header(self) -> QWidget:
        w = QWidget()
        layout = QHBoxLayout(w)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(16)

        title = QLabel("My Projects")
        title.setObjectName("sectionTitle")
        layout.addWidget(title)
        layout.addStretch()

        new_menu = QMenu(self)
        new_menu.addAction("Create New Project", self._handle_create_project)
        new_menu.addAction("Open Existing Project", self._handle_add_project)
        new_menu.addAction("Add Remote Project…", self._handle_add_remote_project)

        new_btn = QPushButton("New Project  ▾")
        new_btn.setObjectName("menuButton")
        new_btn.setMenu(new_menu)
        new_btn.setFixedHeight(34)
        layout.addWidget(new_btn)

        return w

    def _build_toolbar(self) -> QFrame:
        bar = QFrame()
        bar.setObjectName("toolbar")
        bar_layout = QHBoxLayout(bar)
        bar_layout.setContentsMargins(0, 8, 0, 8)
        bar_layout.setSpacing(12)

        self._search_field = QLineEdit()
        self._search_field.setObjectName("searchField")
        self._search_field.setPlaceholderText("Search projects…")
        self._search_field.setClearButtonEnabled(True)
        self._search_field.setFixedHeight(32)
        self._search_field.setMaximumWidth(280)
        self._search_field.textChanged.connect(self._on_search_changed)
        bar_layout.addWidget(self._search_field)

        self._count_label = QLabel("0 projects")
        self._count_label.setObjectName("countBadge")
        bar_layout.addWidget(self._count_label)

        bar_layout.addStretch()

        sort_lbl = QLabel("Sort:")
        sort_lbl.setObjectName("sortLabel")
        bar_layout.addWidget(sort_lbl)

        self._sort_combo = QComboBox()
        self._sort_combo.addItem("Name (A–Z)")
        self._sort_combo.addItem("Name (Z–A)")
        self._sort_combo.setFixedHeight(32)
        self._sort_combo.setMinimumWidth(130)
        self._sort_combo.currentIndexChanged.connect(self._on_sort_changed)
        bar_layout.addWidget(self._sort_combo)

        refresh_btn = QPushButton("↻")
        refresh_btn.setObjectName("iconButton")
        refresh_btn.setToolTip("Refresh project list")
        refresh_btn.setFixedSize(32, 32)
        refresh_btn.clicked.connect(self._load_projects)
        bar_layout.addWidget(refresh_btn)

        return bar

    def _build_empty_page(self) -> QWidget:
        page = QWidget()
        page.setObjectName("scrollContent")
        layout = QVBoxLayout(page)
        layout.setAlignment(Qt.AlignCenter)
        layout.setSpacing(24)

        title = QLabel("Ready? Set. Create!")
        title.setObjectName("emptyTitle")
        title.setAlignment(Qt.AlignCenter)
        layout.addWidget(title)

        sub = QLabel("Welcome to Dusk Engine! Start something new by creating a project.")
        sub.setObjectName("emptySubtitle")
        sub.setAlignment(Qt.AlignCenter)
        layout.addWidget(sub)

        layout.addSpacing(16)

        btns_row = QHBoxLayout()
        btns_row.setAlignment(Qt.AlignCenter)
        btns_row.setSpacing(20)

        create_btn = QPushButton("Create a project\n\nStart from scratch or use a template")
        create_btn.setObjectName("emptyAction")
        create_btn.clicked.connect(self._handle_create_project)
        btns_row.addWidget(create_btn)

        open_btn = QPushButton("Open a project\n\nOpen an existing project from disk")
        open_btn.setObjectName("emptyAction")
        open_btn.clicked.connect(self._handle_add_project)
        btns_row.addWidget(open_btn)

        remote_btn = QPushButton("Add a remote project\n\nDownload a project from a repository")
        remote_btn.setObjectName("emptyAction")
        remote_btn.clicked.connect(self._handle_add_remote_project)
        btns_row.addWidget(remote_btn)

        layout.addLayout(btns_row)

        return page

    def _build_projects_page(self) -> QWidget:
        page = QWidget()
        page.setObjectName("scrollContent")
        layout = QVBoxLayout(page)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(8)

        layout.addWidget(self._build_toolbar())

        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        scroll.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        self._list_widget = ProjectsListWidget()
        scroll.setWidget(self._list_widget)
        layout.addWidget(scroll)

        return page

    def _load_projects(self):
        projects = get_all_projects()
        self._all_projects = self._sort_projects(projects)
        self._render_projects()

    def _sort_projects(self, projects: list) -> list:
        if self._sort_order == 0:
            return sorted(projects, key=lambda p: p["name"].lower())
        else:
            return sorted(projects, key=lambda p: p["name"].lower(), reverse=True)

    def _render_projects(self):
        self._list_widget.clear_rows()

        filtered = [
            p for p in self._all_projects
            if not self._search_text or
               self._search_text.lower() in p["name"].lower() or
               self._search_text.lower() in p["path"].lower()
        ]

        if not self._all_projects:
            self._stack.setCurrentWidget(self._empty_page)
            return

        self._stack.setCurrentWidget(self._projects_page)

        total = len(self._all_projects)
        visible = len(filtered)
        if visible == total:
            self._count_label.setText(f"{total} {'project' if total == 1 else 'projects'}")
        else:
            self._count_label.setText(f"{visible} of {total} projects")

        for p in filtered:
            engine = find_engine_for_project(p)
            row = ProjectRow(p, engine, self._list_widget)

            if p["path"] in self._building_paths:
                row.set_status("building")

            row.open_editor_requested.connect(self._handle_open_editor)
            row.build_requested.connect(self._handle_build)
            row.edit_settings_requested.connect(self._handle_edit_settings)
            row.edit_gems_requested.connect(self._handle_edit_gems)
            row.remove_requested.connect(self._handle_remove)
            row.delete_requested.connect(self._handle_delete)
            row.open_folder_requested.connect(self._handle_open_folder)
            row.launch_asset_processor_requested.connect(self._handle_launch_ap)
            row.duplicate_requested.connect(self._handle_duplicate)

            self._list_widget.add_row(row)

    def _find_row(self, project_path: str) -> "ProjectRow | None":
        for row in self._list_widget.get_rows():
            if row.get_project()["path"] == project_path:
                return row
        return None

    def _handle_create_project(self):
        dlg = NewProjectDialog(self)
        if dlg.exec_() != QDialog.Accepted:
            return
        name = dlg.get_name()
        path = dlg.get_path()
        try:
            result = subprocess.run(
                ["python3", str(O3DE_SCRIPT), "create-project",
                 "--project-path", path, "--project-name", name],
                capture_output=True, text=True, cwd=str(ENGINE_PATH)
            )
            if result.returncode != 0:
                QMessageBox.critical(self, "Create Failed",
                                     f"Failed to create project:\n{result.stderr or result.stdout}")
                return
            self._register_project(path)
            self._load_projects()
        except Exception as e:
            QMessageBox.critical(self, "Error", str(e))

    def _handle_add_project(self):
        folder = QFileDialog.getExistingDirectory(self, "Select Project Folder")
        if not folder:
            return
        pjson = Path(folder) / "project.json"
        if not pjson.exists():
            QMessageBox.warning(self, "Not a Project",
                                "No project.json found in the selected folder.")
            return
        self._register_project(folder)
        self._load_projects()

    def _handle_add_remote_project(self):
        QMessageBox.information(
            self, "Add Remote Project",
            "Remote project support requires the o3de repo tool.\n\n"
            "Run via terminal:\n"
            f"  python3 {O3DE_SCRIPT} download-project --project-uri <URL>"
        )

    def _register_project(self, path: str):
        manifest = read_manifest()
        projects = manifest.get("projects", [])
        if path not in projects:
            projects.append(path)
            manifest["projects"] = projects
            write_manifest(manifest)

    def _handle_open_editor(self, project: dict):
        if not EDITOR_BIN.exists():
            QMessageBox.warning(self, "Editor Not Found",
                                f"Editor binary not found:\n{EDITOR_BIN}\n\n"
                                "Build the project first.")
            return
        row = self._find_row(project["path"])
        if row:
            row.set_status("launching")
            QTimer.singleShot(4000, lambda: row.set_status("ready") if row else None)
        env = {**os.environ, "QT_QPA_PLATFORM": "xcb", "DRI_PRIME": "1"}
        subprocess.Popen(
            [str(EDITOR_BIN),
             f"--regset=/Amazon/AzCore/Bootstrap/project_path={project['path']}"],
            cwd=str(EDITOR_BIN.parent),
            env=env
        )

    def _handle_build(self, project: dict):
        path = project["path"]
        if path in self._building_paths:
            QMessageBox.information(self, "Already Building",
                                    "This project is already being built.")
            return

        project_json_path = Path(path) / "project.json"
        try:
            project_name = json.loads(project_json_path.read_text())["project_name"]
        except Exception:
            project_name = project.get("name", "")

        self._building_paths.add(path)
        row = self._find_row(path)
        if row:
            row.set_status("building")

        worker = BuildWorker(path, project_name)
        self._build_workers[path] = worker
        worker.finished.connect(lambda ok, p=path: self._on_build_finished(p, ok))

        dlg = BuildOutputDialog(project_name, worker, self)
        dlg.show()

        worker.start()

    def _on_build_finished(self, project_path: str, success: bool):
        self._building_paths.discard(project_path)
        worker = self._build_workers.pop(project_path, None)
        row = self._find_row(project_path)
        if row:
            row.set_status("ready" if success else "failed")

    def _handle_edit_settings(self, project: dict):
        pjson = Path(project["path"]) / "project.json"
        if not pjson.exists():
            QMessageBox.warning(self, "Not Found", f"project.json not found:\n{pjson}")
            return
        subprocess.Popen(["xdg-open", str(pjson)])

    def _handle_edit_gems(self, project: dict):
        dlg = PluginConfigDialog(project, self)
        dlg.exec_()

    def _handle_remove(self, project: dict):
        reply = QMessageBox.question(
            self, "Remove Project",
            f"Remove \"{project['name']}\" from Dusk Engine?\n\n"
            "The project files will NOT be deleted.",
            QMessageBox.Yes | QMessageBox.No
        )
        if reply != QMessageBox.Yes:
            return
        manifest = read_manifest()
        projects = [p for p in manifest.get("projects", []) if p != project["path"]]
        manifest["projects"] = projects
        write_manifest(manifest)
        self._load_projects()

    def _handle_delete(self, project: dict):
        reply = QMessageBox.warning(
            self, "Delete Project",
            f"Permanently delete \"{project['name']}\" and all its files?\n\n"
            f"Path: {project['path']}\n\n"
            "This CANNOT be undone.",
            QMessageBox.Yes | QMessageBox.Cancel
        )
        if reply != QMessageBox.Yes:
            return
        try:
            shutil.rmtree(project["path"])
        except Exception as e:
            QMessageBox.critical(self, "Delete Failed", str(e))
            return
        manifest = read_manifest()
        manifest["projects"] = [p for p in manifest.get("projects", [])
                                 if p != project["path"]]
        write_manifest(manifest)
        self._load_projects()

    def _handle_open_folder(self, project: dict):
        subprocess.Popen(["xdg-open", project["path"]])

    def _handle_launch_ap(self, project: dict):
        if not ASSET_PROCESSOR_BIN.exists():
            QMessageBox.warning(self, "Not Found",
                                f"Asset Processor not found:\n{ASSET_PROCESSOR_BIN}")
            return
        env = {**os.environ, "QT_QPA_PLATFORM": "xcb", "DRI_PRIME": "1"}
        subprocess.Popen(
            [str(ASSET_PROCESSOR_BIN),
             f"--regset=/Amazon/AzCore/Bootstrap/project_path={project['path']}"],
            cwd=str(ASSET_PROCESSOR_BIN.parent),
            env=env
        )

    def _handle_duplicate(self, project: dict):
        dest = QFileDialog.getExistingDirectory(
            self, "Choose Destination Folder for Duplicate"
        )
        if not dest:
            return
        dest_path = Path(dest) / (Path(project["path"]).name + "_copy")
        try:
            shutil.copytree(project["path"], str(dest_path),
                            ignore=shutil.ignore_patterns("build", "Cache", "user"))
        except Exception as e:
            QMessageBox.critical(self, "Duplicate Failed", str(e))
            return
        self._register_project(str(dest_path))
        self._load_projects()

    def _on_search_changed(self, text: str):
        self._search_text = text
        self._render_projects()

    def _on_sort_changed(self, index: int):
        self._sort_order = index
        self._all_projects = self._sort_projects(self._all_projects)
        self._render_projects()


def main():
    app = QApplication(sys.argv)
    app.setApplicationName("Dusk Engine Project Manager")

    font_dirs = [
        ENGINE_PATH / "Assets" / "Editor" / "Fonts",
        Path("/usr/share/fonts"),
        Path.home() / ".local" / "share" / "fonts",
    ]
    for d in font_dirs:
        if d.exists():
            for f in d.rglob("*.ttf"):
                QFontDatabase.addApplicationFont(str(f))
            for f in d.rglob("*.otf"):
                QFontDatabase.addApplicationFont(str(f))

    app.setStyleSheet(APP_QSS)

    win = ProjectManagerLite()
    win.show()
    sys.exit(app.exec_())


if __name__ == "__main__":
    main()
