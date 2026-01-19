# -*- mode: python ; coding: utf-8 -*-

from PyInstaller.utils.hooks import collect_data_files

# Сбор статических данных: локальные UI-файлы + данные пакета xraydb (включая xraydb.sqlite)
extra_datas = [('UI', 'UI')] + collect_data_files('xraydb')

a = Analysis(
    ['main_32.py'],
    pathex=[],
    binaries=[],
    datas=extra_datas,
    hiddenimports=[],
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=[],
    noarchive=False,
    optimize=0,
)
pyz = PYZ(a.pure)

exe = EXE(
    pyz,
    a.scripts,
    a.binaries,
    a.datas,
    [],
    name='main_32',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    upx_exclude=[],
    runtime_tmpdir=None,
    console=False,
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch='32bit',
    codesign_identity=None,
    entitlements_file=None,
    icon=['Icon\\Icon.ico'],
)


