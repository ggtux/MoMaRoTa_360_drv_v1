from pathlib import Path

from SCons.Script import COMMAND_LINE_TARGETS

Import("env")


if "upload" in COMMAND_LINE_TARGETS:
    password_file = Path(env.subst("$PROJECT_DIR")) / ".ota-password"
    if password_file.is_file():
        ota_password = password_file.read_text(encoding="utf-8").strip()
        if ota_password:
            env.Replace(UPLOAD_FLAGS="--auth=" + ota_password)
