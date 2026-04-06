#!/usr/bin/env python3
import sys
import os
import json
import re
import subprocess
from pathlib import Path

from PyQt5.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QLabel, QPushButton, QComboBox, QFrame, QSizePolicy, QStackedWidget
)
from PyQt5.QtCore import Qt, QTimer, QSize
from PyQt5.QtGui import QWindow, QFont, QFontDatabase

ENGINE_PATH   = Path(__file__).parent.parent.resolve()
BUILD_PATH    = ENGINE_PATH / "build" / "linux"
EDITOR_BIN    = BUILD_PATH / "bin" / "profile" / "Editor"
MANIFEST_PATH = Path.home() / ".o3de" / "o3de_manifest.json"

APP_QSS = """
* { font-family: "Manrope"; color: #ffffff; }
QMainWindow, QWidget { background: #0d0d0d; }
QFrame#topbar { background: #111111; border-bottom: 1px solid rgba(255,255,255,0.07); }
QFrame#statusbar { background: #0a0a0a; border-top: 1px solid rgba(255,255,255,0.05); }
QLabel#projname { font-size: 13px; font-weight: 700; color: #ffffff; }
QLabel#subtitle { font-size: 11px; color: #555555; }
QLabel#status   { font-size: 11px; color: #888888; }
QLabel#statusOk { font-size: 11px; color: #4CAF50; }
QLabel#statusWarn { font-size: 11px; color: #FFB74D; }
QLabel#bigTitle { font-size: 28px; font-weight: 700; color: #ffffff; }
QLabel#bigSub   { font-size: 13px; color: #555555; }
QComboBox {
    background: rgba(255,255,255,0.05); border: 1px solid rgba(255,255,255,0.12);
    padding: 7px 12px; font-size: 12px; color: #ddd; min-height: 34px; min-width: 360px;
}
QComboBox:hover { border-color: rgba(255,255,255,0.22); }
QComboBox QAbstractItemView {
    background: #1a1a1a; border: 1px solid rgba(255,255,255,0.15);
    selection-background-color: rgba(76,175,80,0.22); color: #ddd; outline: none;
}
QComboBox::drop-down { border: none; width: 24px; }
QComboBox::down-arrow { image: none; }
QPushButton {
    border: 1px solid rgba(255,255,255,0.10); background: rgba(255,255,255,0.04);
    padding: 7px 18px; font-size: 12px; color: #cccccc;
}
QPushButton:hover { border-color: rgba(255,255,255,0.22); background: rgba(255,255,255,0.09); }
QPushButton:disabled { color: #444; border-color: rgba(255,255,255,0.04); }
QPushButton#openBtn {
    background: #4CAF50; border: 1px solid #4CAF50; color: #fff;
    font-weight: 700; font-size: 13px; padding: 9px 28px;
}
QPushButton#openBtn:hover { background: #66BB6A; border-color: #66BB6A; }
QPushButton#openBtn:disabled { background: #2a2a2a; border-color: #333; color: #555; }
QWidget#vpContainer { background: #050505; }
"""


def get_projects():
    if not MANIFEST_PATH.exists():
        return []
    try:
        return json.loads(MANIFEST_PATH.read_text()).get("projects", [])
    except Exception:
        return []


def project_name(path: str) -> str:
    try:
        return json.loads((Path(path) / "project.json").read_text()).get("project_name", Path(path).name)
    except Exception:
        return Path(path).name


def find_main_window_by_pid(pid: int) -> int:
    try:
        out = subprocess.check_output(
            ["xprop", "-root", "_NET_CLIENT_LIST_STACKING"],
            text=True, timeout=3, stderr=subprocess.DEVNULL
        )
        wids = re.findall(r"0x[0-9a-fA-F]+", out)
        for wid in reversed(wids):
            try:
                pid_out = subprocess.check_output(
                    ["xprop", "-id", wid, "_NET_WM_PID"],
                    text=True, timeout=1, stderr=subprocess.DEVNULL
                )
                m = re.search(r"= (\d+)", pid_out)
                if m and int(m.group(1)) == pid:
                    name_out = subprocess.check_output(
                        ["xprop", "-id", wid, "WM_NAME"],
                        text=True, timeout=1, stderr=subprocess.DEVNULL
                    )
                    if any(k in name_out for k in ("Editor", "Dusk", "O3DE")):
                        return int(wid, 16)
            except Exception:
                continue
    except Exception:
        pass
    return 0


class HybridEditorShell(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Dusk Engine")
        self.resize(1280, 800)
        self._proc = None
        self._poll_timer = None
        self._dots = 0
        self._container = None

        root = QWidget()
        self.setCentralWidget(root)
        vbox = QVBoxLayout(root)
        vbox.setContentsMargins(0, 0, 0, 0)
        vbox.setSpacing(0)

        self._topbar = self._build_topbar()
        vbox.addWidget(self._topbar)
        self._topbar.setVisible(False)

        self._stack = QStackedWidget()
        vbox.addWidget(self._stack, 1)

        self._statusbar = self._build_statusbar()
        vbox.addWidget(self._statusbar)
        self._statusbar.setVisible(False)

        self._stack.addWidget(self._build_picker_page())
        self._stack.addWidget(self._build_load_page())
        self._stack.addWidget(self._build_vp_page())

    def _build_topbar(self):
        bar = QFrame()
        bar.setObjectName("topbar")
        bar.setFixedHeight(38)
        row = QHBoxLayout(bar)
        row.setContentsMargins(14, 0, 10, 0)
        row.setSpacing(10)
        self._proj_lbl = QLabel("")
        self._proj_lbl.setObjectName("projname")
        row.addWidget(self._proj_lbl)
        sep = QFrame()
        sep.setFrameShape(QFrame.VLine)
        sep.setStyleSheet("color: rgba(255,255,255,0.08);")
        row.addWidget(sep)
        hint = QLabel("Hybrid PoC")
        hint.setObjectName("subtitle")
        row.addWidget(hint)
        row.addStretch()
        btn = QPushButton("Cerrar proyecto")
        btn.clicked.connect(self._close_project)
        row.addWidget(btn)
        return bar

    def _build_statusbar(self):
        bar = QFrame()
        bar.setObjectName("statusbar")
        bar.setFixedHeight(24)
        row = QHBoxLayout(bar)
        row.setContentsMargins(12, 0, 12, 0)
        self._status_lbl = QLabel("Listo")
        self._status_lbl.setObjectName("statusOk")
        row.addWidget(self._status_lbl)
        row.addStretch()
        self._wid_lbl = QLabel("")
        self._wid_lbl.setObjectName("status")
        row.addWidget(self._wid_lbl)
        return bar

    def _build_picker_page(self):
        page = QWidget()
        vbox = QVBoxLayout(page)
        vbox.addStretch(2)
        cl = QVBoxLayout()
        cl.setContentsMargins(80, 0, 80, 0)
        cl.setSpacing(0)
        title = QLabel("Dusk Engine")
        title.setObjectName("bigTitle")
        cl.addWidget(title)
        cl.addSpacing(8)
        sub = QLabel("Editor híbrido — PoC de embedding")
        sub.setObjectName("bigSub")
        cl.addWidget(sub)
        cl.addSpacing(40)
        lbl = QLabel("PROYECTO")
        lbl.setStyleSheet("letter-spacing: 1px; font-size: 10px; color: #555;")
        cl.addWidget(lbl)
        cl.addSpacing(8)
        row = QHBoxLayout()
        row.setSpacing(12)
        self._combo = QComboBox()
        projects = get_projects()
        for p in projects:
            self._combo.addItem(f"{project_name(p)}   —   {p}", p)
        if not projects:
            self._combo.addItem("No hay proyectos registrados", None)
        row.addWidget(self._combo)
        self._open_btn = QPushButton("Abrir →")
        self._open_btn.setObjectName("openBtn")
        self._open_btn.setEnabled(bool(projects))
        self._open_btn.clicked.connect(self._open_project)
        row.addWidget(self._open_btn)
        row.addStretch()
        cl.addLayout(row)
        vbox.addLayout(cl)
        vbox.addStretch(3)
        return page

    def _build_load_page(self):
        page = QWidget()
        vbox = QVBoxLayout(page)
        vbox.setAlignment(Qt.AlignCenter)
        vbox.setSpacing(18)
        self._load_lbl = QLabel("Iniciando editor…")
        self._load_lbl.setObjectName("bigTitle")
        self._load_lbl.setAlignment(Qt.AlignCenter)
        vbox.addWidget(self._load_lbl)
        self._load_sub = QLabel("")
        self._load_sub.setObjectName("status")
        self._load_sub.setAlignment(Qt.AlignCenter)
        vbox.addWidget(self._load_sub)
        vbox.addSpacing(24)
        cancel = QPushButton("Cancelar")
        cancel.setFixedWidth(120)
        cancel.clicked.connect(self._cancel_load)
        row = QHBoxLayout()
        row.addStretch()
        row.addWidget(cancel)
        row.addStretch()
        vbox.addLayout(row)
        return page

    def _build_vp_page(self):
        page = QWidget()
        page.setObjectName("vpPage")
        vbox = QVBoxLayout(page)
        vbox.setContentsMargins(0, 0, 0, 0)
        vbox.setSpacing(0)
        self._vp_slot = QWidget()
        self._vp_slot.setObjectName("vpContainer")
        vbox.addWidget(self._vp_slot, 1)
        self._vp_layout = QVBoxLayout(self._vp_slot)
        self._vp_layout.setContentsMargins(0, 0, 0, 0)
        ph = QLabel("Viewport se embederá aquí")
        ph.setAlignment(Qt.AlignCenter)
        ph.setStyleSheet("color: #333; font-size: 13px;")
        self._vp_layout.addWidget(ph)
        self._vp_placeholder = ph
        return page

    def _open_project(self):
        path = self._combo.currentData()
        if not path:
            return
        name = project_name(path)
        self._proj_lbl.setText(name)
        self._load_lbl.setText(f"Abriendo {name}…")
        self._load_sub.setText("Lanzando Editor")
        self._stack.setCurrentIndex(1)
        env = {**os.environ, "QT_QPA_PLATFORM": "xcb", "DRI_PRIME": "1"}
        self._proc = subprocess.Popen(
            [str(EDITOR_BIN),
             f"--regset=/Amazon/AzCore/Bootstrap/project_path={path}"],
            cwd=str(EDITOR_BIN.parent), env=env
        )
        self._dots = 0
        self._poll_timer = QTimer(self)
        self._poll_timer.timeout.connect(self._poll_for_window)
        self._poll_timer.start(1500)

    def _poll_for_window(self):
        if self._proc and self._proc.poll() is not None:
            self._poll_timer.stop()
            self._load_sub.setText("El Editor se cerró inesperadamente.")
            return
        self._dots += 1
        self._load_sub.setText("Esperando ventana principal" + "." * (self._dots % 4))
        if self._dots < 4:
            return
        wid = find_main_window_by_pid(self._proc.pid)
        if wid:
            self._poll_timer.stop()
            QTimer.singleShot(1500, lambda: self._embed(wid))

    def _embed(self, wid: int):
        qwin = QWindow.fromWinId(wid)
        qwin.setFlags(Qt.FramelessWindowHint)
        container = QWidget.createWindowContainer(qwin, self._vp_slot)
        container.setMinimumSize(200, 200)
        container.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self._vp_layout.removeWidget(self._vp_placeholder)
        self._vp_placeholder.hide()
        self._vp_layout.addWidget(container)
        self._container = container
        self._topbar.setVisible(True)
        self._statusbar.setVisible(True)
        self._status_lbl.setText("Editor embebido")
        self._wid_lbl.setText(f"WID: {hex(wid)}  PID: {self._proc.pid}")
        self._stack.setCurrentIndex(2)
        self._watchdog = QTimer(self)
        self._watchdog.timeout.connect(self._check_alive)
        self._watchdog.start(2000)

    def _check_alive(self):
        if self._proc and self._proc.poll() is not None:
            self._watchdog.stop()
            self._status_lbl.setText("El Editor se cerró")
            self._status_lbl.setObjectName("statusWarn")
            self._status_lbl.style().unpolish(self._status_lbl)
            self._status_lbl.style().polish(self._status_lbl)

    def _cancel_load(self):
        if self._poll_timer:
            self._poll_timer.stop()
        if self._proc and self._proc.poll() is None:
            self._proc.terminate()
        self._stack.setCurrentIndex(0)

    def _close_project(self):
        if hasattr(self, "_watchdog"):
            self._watchdog.stop()
        if self._container:
            self._vp_layout.removeWidget(self._container)
            self._container.hide()
            self._container = None
        self._vp_layout.addWidget(self._vp_placeholder)
        self._vp_placeholder.show()
        if self._proc and self._proc.poll() is None:
            self._proc.terminate()
        self._proc = None
        self._topbar.setVisible(False)
        self._statusbar.setVisible(False)
        self._stack.setCurrentIndex(0)

    def closeEvent(self, event):
        if self._proc and self._proc.poll() is None:
            self._proc.terminate()
        super().closeEvent(event)


def main():
    os.environ.setdefault("QT_QPA_PLATFORM", "xcb")
    app = QApplication(sys.argv)
    for f in (Path.home() / ".local" / "share" / "fonts").glob("Manrope*.ttf"):
        QFontDatabase.addApplicationFont(str(f))
    app.setFont(QFont("Manrope", 10))
    app.setStyleSheet(APP_QSS)
    win = HybridEditorShell()
    win.show()
    sys.exit(app.exec_())


if __name__ == "__main__":
    main()
