#!/usr/bin/env python3
"""xAT operations tool: build, flash, debug and monitor firmware on the FRDM-MCXN947.

Single-file tool. Run from the xAT root:
    python3 scripts/operations.py doctor
    python3 scripts/operations.py build   [--config debug|release] [--clean] [--app src/hello_uart]
    python3 scripts/operations.py flash   [--flasher auto|pyocd|linkserver|jlink] [--probe UID] [--no-verify]
    python3 scripts/operations.py debug   [--debugger auto|pyocd|linkserver|jlink] [--batch FILE] [--port 3333]
    python3 scripts/operations.py monitor [--port /dev/ttyACM0] [--baud 115200] [--expect TEXT] [--timeout 10]
    python3 scripts/operations.py run

Design: ``Target`` is the Strategy-pattern context. It holds one ``BuildStrategy``, one
``FlashStrategy`` and one ``DebugServerStrategy``, chosen by name from the registries
below. Adding a backend means adding one class plus one registry entry.

Only the standard library is imported at module level. ``pyserial`` is imported lazily by
``SerialMonitor``; pyOCD, LinkServer, J-Link, CMake and GDB are driven through their CLIs
via ``CommandRunner``.
"""

from __future__ import annotations

import abc
import argparse
import dataclasses
import json
import logging
import os
import re
import shutil
import signal
import struct
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Callable, Mapping, Optional, Sequence, TextIO

LOG = logging.getLogger("xat.operations")

# =====================================================================================
# Defaults / configuration
# =====================================================================================

XAT_ROOT = Path(__file__).resolve().parent.parent  # the workspace is XAT_ROOT.parent

EXIT_OK = 0
EXIT_FAILURE = 1
EXIT_TIMEOUT = 3


@dataclass(frozen=True)
class Defaults:
    """Project defaults. Override with environment variables (see ENV_OVERRIDES) or CLI options."""

    board: str = "frdmmcxn947"
    core_id: str = "cm33_core0"
    app: str = "src/hello_uart"
    build_config: str = "debug"
    sdk_dir_name: str = "mcuxsdk"  # <workspace>/mcuxsdk
    toolchain_glob: str = "tools/arm-gnu-toolchain-*-arm-none-eabi"  # under <workspace>
    venv_dir_name: str = ".venv"  # <xAT>/.venv
    builder: str = "cmake"
    flasher: str = "auto"
    debugger: str = "auto"
    probe_uid: str = ""
    probe_usb_id: str = "1fc9:0143"  # NXP MCU-LINK FRDM-MCXN947 CMSIS-DAP
    # pyOCD: settings verified on hardware are recorded here.
    pyocd_target: str = "mcxn947vdf"  # pack target (NXP.MCXN947_DFP); board part MCXN947VDF
    pyocd_frequency: str = "4000000"
    # Verified on FRDM-MCXN947 (pyOCD 0.45.1, NXP.MCXN947_DFP 26.09.00):
    # - GDB server: with the default/sw reset every 'continue' after 'monitor reset halt'
    #   only single-steps, so the hardware reset line is used;
    # - load/commander/reset: 'hw' makes the pack's ResetCatchClear sequence fail on
    #   cm33_core1 (SWD FAULT ACK), so the pack default ("") is used.
    pyocd_flash_reset_type: str = ""
    pyocd_debug_reset_type: str = "hw"
    flash_base: int = 0x00000000
    # LinkServer / J-Link device names (from the SDK board_runner.cmake).
    linkserver_device: str = "MCXN947:FRDM-MCXN947"
    jlink_device: str = "MCXN947_M33_0"
    gdb_host: str = "localhost"
    gdb_port: int = 3333
    gdb_candidates: tuple[str, ...] = ("arm-none-eabi-gdb", "gdb-multiarch")
    server_start_timeout: float = 20.0
    serial_port: str = ""  # empty: auto-detect the MCU-Link VCOM
    serial_vid: int = 0x1FC9
    baud: int = 115200
    expect: str = "boot OK"
    monitor_timeout: float = 10.0
    min_cmake: str = "3.30.0"
    min_ninja: str = "1.12.0"


DEFAULTS = Defaults()

# Environment variable -> Config field.
ENV_OVERRIDES: Mapping[str, str] = {
    "XAT_SDK_ROOT": "sdk_root",
    "ARMGCC_DIR": "armgcc_dir",
    "XAT_BOARD": "board",
    "XAT_CORE": "core_id",
    "XAT_BUILDER": "builder",
    "XAT_FLASHER": "flasher",
    "XAT_DEBUGGER": "debugger",
    "XAT_PROBE": "probe_uid",
    "XAT_GDB": "gdb",
    "XAT_PYOCD_TARGET": "pyocd_target",
    "XAT_SERIAL_PORT": "serial_port",
    "XAT_LINKSERVER_DIR": "linkserver_dir",
}


@dataclass(frozen=True)
class Config:
    """Fully resolved configuration shared by all strategies."""

    xat_root: Path
    sdk_root: Path
    armgcc_dir: Optional[Path]
    app_dir: Path
    linkserver_dir: Optional[Path] = None  # only needed if LinkServer is not on PATH
    build_config: str = DEFAULTS.build_config
    board: str = DEFAULTS.board
    core_id: str = DEFAULTS.core_id
    builder: str = DEFAULTS.builder
    flasher: str = DEFAULTS.flasher
    debugger: str = DEFAULTS.debugger
    probe_uid: str = DEFAULTS.probe_uid
    gdb: str = ""
    gdb_port: int = DEFAULTS.gdb_port
    pyocd_target: str = DEFAULTS.pyocd_target
    serial_port: str = DEFAULTS.serial_port
    baud: int = DEFAULTS.baud
    defaults: Defaults = DEFAULTS

    @property
    def app_name(self) -> str:
        return self.app_dir.name

    @property
    def build_dir(self) -> Path:
        return self.xat_root / "build" / self.app_name / self.build_config

    @property
    def venv_bin(self) -> Path:
        return self.xat_root / self.defaults.venv_dir_name / "bin"

    @property
    def toolchain_bin(self) -> Optional[Path]:
        return self.armgcc_dir / "bin" if self.armgcc_dir else None

    def tool_dirs(self) -> list[Path]:
        """Directories searched before PATH: toolchain, project venv, optional LinkServer dir."""
        return [p for p in (self.toolchain_bin, self.venv_bin, self.linkserver_dir) if p is not None]

    @staticmethod
    def find_toolchain(workspace: Path, pattern: str) -> Optional[Path]:
        matches = sorted(p for p in workspace.glob(pattern) if (p / "bin").is_dir())
        return matches[-1] if matches else None

    @classmethod
    def resolve(
        cls,
        overrides: Mapping[str, Any] | None = None,
        environ: Mapping[str, str] | None = None,
        xat_root: Path | None = None,
        defaults: Defaults = DEFAULTS,
    ) -> "Config":
        """Layer defaults <- environment <- CLI overrides (None values are ignored)."""
        environ = os.environ if environ is None else environ
        xat_root = XAT_ROOT if xat_root is None else xat_root
        workspace = xat_root.parent
        values: dict[str, Any] = {
            "sdk_root": workspace / defaults.sdk_dir_name,
            "armgcc_dir": cls.find_toolchain(workspace, defaults.toolchain_glob),
            "app_dir": defaults.app,
        }
        shared = {f.name for f in dataclasses.fields(Defaults)} & {f.name for f in dataclasses.fields(cls)}
        values.update({name: getattr(defaults, name) for name in shared})
        for env_name, field_name in ENV_OVERRIDES.items():
            if environ.get(env_name):
                values[field_name] = environ[env_name]
        for key, value in (overrides or {}).items():
            if value is not None:
                values[key] = value

        app_dir = Path(values.pop("app_dir"))
        values["app_dir"] = (app_dir if app_dir.is_absolute() else xat_root / app_dir).resolve()
        values["sdk_root"] = Path(values["sdk_root"]).resolve()
        for path_field in ("armgcc_dir", "linkserver_dir"):
            if values.get(path_field) is not None:
                values[path_field] = Path(values[path_field]).resolve()
        for int_field in ("gdb_port", "baud"):
            if int_field in values:
                values[int_field] = int(values[int_field])
        known = {f.name for f in dataclasses.fields(cls)}
        config = cls(xat_root=xat_root, defaults=defaults, **{k: v for k, v in values.items() if k in known})
        config.check_build_dir()
        return config

    def check_build_dir(self) -> None:
        if self.build_dir.resolve().is_relative_to(self.sdk_root):
            raise OperationsError(f"build directory {self.build_dir} must not be inside the SDK")


# =====================================================================================
# Errors
# =====================================================================================


class OperationsError(Exception):
    """Any failure of an operation; carries a human readable message."""


class ToolNotFoundError(OperationsError):
    """A required external tool is not installed or not on the search path."""


class CommandError(OperationsError):
    """An external command exited with a non-zero status."""

    def __init__(self, cmd: Sequence[str], returncode: int, stdout: str, stderr: str) -> None:
        self.cmd = list(cmd)
        self.returncode = returncode
        self.stdout = stdout
        self.stderr = stderr
        details = "\n".join(s for s in (stdout.strip(), stderr.strip()) if s)
        super().__init__(f"command failed ({returncode}): {format_cmd(cmd)}" + (f"\n{details}" if details else ""))


# =====================================================================================
# CommandRunner
# =====================================================================================


def format_cmd(cmd: Sequence[str]) -> str:
    return subprocess.list2cmdline([str(c) for c in cmd])


@dataclass
class CommandResult:
    returncode: int
    stdout: str = ""
    stderr: str = ""


class CommandRunner:
    """Runs every external command. Tests replace it with a fake."""

    def __init__(self, extra_path: Sequence[Path] = (), base_env: Mapping[str, str] | None = None) -> None:
        self._extra_path = [str(p) for p in extra_path]
        self._base_env = dict(os.environ if base_env is None else base_env)

    def env(self, extra: Mapping[str, str] | None = None) -> dict[str, str]:
        env = dict(self._base_env)
        env["PATH"] = os.pathsep.join(self._extra_path + [env.get("PATH", "")])
        env.update(extra or {})
        return env

    def which(self, name: str) -> Optional[str]:
        return shutil.which(name, path=self.env()["PATH"])

    def run(
        self,
        cmd: Sequence[str],
        *,
        cwd: Path | None = None,
        env: Mapping[str, str] | None = None,
        capture: bool = True,
        check: bool = True,
        timeout: float | None = None,
        merge_output: bool = False,
    ) -> CommandResult:
        """Run a command. ``merge_output`` sends stderr into stdout to keep their order."""
        LOG.info("$ %s", format_cmd(cmd))
        pipe = subprocess.PIPE if capture else None
        try:
            proc = subprocess.run(
                [str(c) for c in cmd],
                cwd=cwd,
                env=self.env(env),
                stdout=pipe,
                stderr=subprocess.STDOUT if capture and merge_output else pipe,
                text=True,
                timeout=timeout,
                check=False,
            )
        except FileNotFoundError as exc:
            raise ToolNotFoundError(f"tool not found: {cmd[0]}") from exc
        except subprocess.TimeoutExpired as exc:
            raise OperationsError(f"command timed out after {timeout}s: {format_cmd(cmd)}") from exc
        result = CommandResult(proc.returncode, proc.stdout or "", proc.stderr or "")
        if result.stdout:
            LOG.debug("stdout:\n%s", result.stdout.rstrip())
        if result.stderr:
            LOG.debug("stderr:\n%s", result.stderr.rstrip())
        if check and result.returncode != 0:
            raise CommandError(cmd, result.returncode, result.stdout, result.stderr)
        return result

    def spawn(self, cmd: Sequence[str], log_path: Path, *, env: Mapping[str, str] | None = None) -> Any:
        """Start a long-running process (debug server) with its output sent to log_path."""
        LOG.info("$ %s &  (log: %s)", format_cmd(cmd), log_path)
        log_path.parent.mkdir(parents=True, exist_ok=True)
        with open(log_path, "w", encoding="utf-8") as log_file:
            try:
                return subprocess.Popen(
                    [str(c) for c in cmd],
                    env=self.env(env),
                    stdin=subprocess.DEVNULL,
                    stdout=log_file,
                    stderr=subprocess.STDOUT,
                    start_new_session=True,  # Ctrl-C in GDB must not kill the server
                )
            except FileNotFoundError as exc:
                raise ToolNotFoundError(f"tool not found: {cmd[0]}") from exc


# =====================================================================================
# Value objects
# =====================================================================================


@dataclass(frozen=True)
class BuildArtifacts:
    elf: Path
    bin: Path
    hex: Optional[Path] = None
    map: Optional[Path] = None


@dataclass(frozen=True)
class ProbeInfo:
    uid: str
    description: str
    backend: str


@dataclass(frozen=True)
class GdbEndpoint:
    host: str
    port: int
    # GDB commands run right after connecting; they leave the MCU reset and halted.
    session_commands: tuple[str, ...] = ("monitor reset halt",)

    @property
    def address(self) -> str:
        return f"{self.host}:{self.port}"


@dataclass(frozen=True)
class LoadSegment:
    """Bytes of one ELF PT_LOAD segment and the (physical) address they are programmed to."""

    address: int
    data: bytes


def elf_load_segments(elf: Path) -> list[LoadSegment]:
    """Return the PT_LOAD segments with file contents of a 32-bit little-endian ELF.

    Used to verify flash contents: unlike the .bin image, it does not contain the
    zero-filled gaps between sections that are never programmed.
    """
    raw = elf.read_bytes()
    if raw[:4] != b"\x7fELF" or raw[4] != 1 or raw[5] != 1:
        raise OperationsError(f"{elf}: not a 32-bit little-endian ELF file")
    phoff, = struct.unpack_from("<I", raw, 28)
    phentsize, phnum = struct.unpack_from("<HH", raw, 42)
    segments = []
    for index in range(phnum):
        p_type, p_offset, _vaddr, p_paddr, p_filesz = struct.unpack_from("<5I", raw, phoff + index * phentsize)
        if p_type == 1 and p_filesz > 0:  # PT_LOAD
            segments.append(LoadSegment(p_paddr, raw[p_offset : p_offset + p_filesz]))
    return segments


# =====================================================================================
# Strategy interfaces
# =====================================================================================


class Backend(abc.ABC):
    """Common base: every backend has a registry name and knows if its tool is installed."""

    name: str = ""
    tool: str = ""

    def __init__(self, config: Config, runner: CommandRunner) -> None:
        self.config = config
        self.runner = runner

    def is_available(self) -> bool:
        return self.runner.which(self.tool) is not None

    def require_tool(self) -> str:
        path = self.runner.which(self.tool)
        if path is None:
            raise ToolNotFoundError(f"{self.name}: '{self.tool}' not found (see docs/setup_host.md)")
        return path


class BuildStrategy(Backend):
    @abc.abstractmethod
    def configure(self) -> None: ...

    @abc.abstractmethod
    def build(self) -> None: ...

    @abc.abstractmethod
    def clean(self) -> None: ...

    @abc.abstractmethod
    def artifacts(self) -> BuildArtifacts: ...


class FlashStrategy(Backend):
    @abc.abstractmethod
    def probes(self) -> list[ProbeInfo]: ...

    @abc.abstractmethod
    def flash(self, image: BuildArtifacts, verify: bool = True) -> None: ...

    @abc.abstractmethod
    def erase(self) -> None: ...

    @abc.abstractmethod
    def reset(self) -> None: ...


class DebugServerStrategy(Backend):
    """Starts a GDB server in the background. Use as a context manager so it is always stopped.

    Subclasses provide the server command line and the log line that means "ready".
    """

    ready_pattern: str = ""
    # GDB commands run right after connecting that leave the MCU reset and halted
    # (server specific), so GDB scripts stay backend independent.
    session_commands: tuple[str, ...] = ("monitor reset halt",)

    def __init__(self, config: Config, runner: CommandRunner) -> None:
        super().__init__(config, runner)
        self._process: Any = None
        self.log_path = config.build_dir / f"{self.name}_gdbserver.log"

    @abc.abstractmethod
    def server_command(self) -> list[str]: ...

    def start(self) -> GdbEndpoint:
        self.require_tool()
        self._process = self.runner.spawn(self.server_command(), self.log_path)
        deadline = time.monotonic() + self.config.defaults.server_start_timeout
        while time.monotonic() < deadline:
            if re.search(self.ready_pattern, self._read_log()):
                return GdbEndpoint(self.config.defaults.gdb_host, self.config.gdb_port, self.session_commands)
            if self._process.poll() is not None:
                break
            time.sleep(0.2)
        self.stop()
        raise OperationsError(f"{self.name} GDB server did not start; log {self.log_path}:\n{self._read_log()}")

    def stop(self) -> None:
        proc, self._process = self._process, None
        if proc is None or proc.poll() is not None:
            return
        LOG.info("stopping %s GDB server (pid %s)", self.name, proc.pid)
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait(timeout=5)

    def _read_log(self) -> str:
        try:
            return self.log_path.read_text(encoding="utf-8", errors="replace")
        except FileNotFoundError:
            return ""

    def __enter__(self) -> GdbEndpoint:
        return self.start()

    def __exit__(self, *exc: object) -> None:
        self.stop()


# =====================================================================================
# Strategy implementations
# =====================================================================================

# ---- build --------------------------------------------------------------------------


class CMakeNinjaBuild(BuildStrategy):
    """Out-of-tree MCUXpresso SDK CMake build with the Ninja generator."""

    name = "cmake"
    tool = "cmake"

    def is_available(self) -> bool:
        return all(self.runner.which(t) for t in ("cmake", "ninja"))

    def _env(self) -> dict[str, str]:
        if self.config.armgcc_dir is None:
            raise ToolNotFoundError("Arm GNU toolchain not found: set ARMGCC_DIR (see docs/setup_host.md)")
        # The SDK runs its own Python scripts (Kconfig); keep them from writing
        # __pycache__ into the read-only SDK tree.
        return {"ARMGCC_DIR": str(self.config.armgcc_dir), "PYTHONDONTWRITEBYTECODE": "1"}

    def configure(self) -> None:
        cfg = self.config
        self.require_tool()
        self.runner.run(
            [
                "cmake",
                "-G",
                "Ninja",
                "-S",
                cfg.app_dir,
                "-B",
                cfg.build_dir,
                f"-DCMAKE_TOOLCHAIN_FILE={cfg.sdk_root / 'cmake' / 'toolchain' / 'armgcc.cmake'}",
                f"-DSdkRootDirPath={cfg.sdk_root}",
                f"-Dboard={cfg.board}",
                f"-Dcore_id={cfg.core_id}",
                "-DCONFIG_TOOLCHAIN=armgcc",
                f"-DCMAKE_BUILD_TYPE={cfg.build_config}",
                "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
            ],
            env=self._env(),
        )

    def build(self) -> None:
        self.require_tool()
        self.runner.run(["cmake", "--build", self.config.build_dir], env=self._env())

    def clean(self) -> None:
        build_dir = self.config.build_dir
        self.config.check_build_dir()
        if build_dir.exists():
            LOG.info("removing %s", build_dir)
            shutil.rmtree(build_dir)

    def artifacts(self) -> BuildArtifacts:
        out = self.config.build_dir
        # The SDK names the image <project>_<core_id> on multicore devices, else <project>.
        names = (f"{self.config.app_name}_{self.config.core_id}", self.config.app_name)
        name = next((n for n in names if (out / f"{n}.elf").is_file()), names[0])
        elf, binary = out / f"{name}.elf", out / f"{name}.bin"
        for path in (elf, binary):
            if not path.is_file():
                raise OperationsError(f"build artifact missing: {path} (run 'build' first)")
        hex_file = out / f"{name}.hex"
        map_file = out / "output.map"
        return BuildArtifacts(
            elf=elf,
            bin=binary,
            hex=hex_file if hex_file.is_file() else None,
            map=map_file if map_file.is_file() else None,
        )


class WestBuild(CMakeNinjaBuild):
    """Optional: builds through 'west build' when west is installed (same output layout)."""

    name = "west"
    tool = "west"

    def is_available(self) -> bool:
        return self.runner.which("west") is not None

    def configure(self) -> None:
        cfg = self.config
        self.require_tool()
        self.runner.run(
            [
                "west",
                "build",
                "--cmake-only",
                "-b",
                cfg.board,
                "-d",
                cfg.build_dir,
                cfg.app_dir,
                "--config",
                cfg.build_config,
                f"-Dcore_id={cfg.core_id}",
            ],
            cwd=cfg.sdk_root,
            env=self._env(),
        )

    def build(self) -> None:
        self.require_tool()
        self.runner.run(["west", "build", "-d", self.config.build_dir], cwd=self.config.sdk_root, env=self._env())


# ---- pyOCD --------------------------------------------------------------------------


class PyOcdMixin:
    """Common pyOCD command-line options."""

    config: Config

    def pyocd_args(self, reset_type: str = "") -> list[str]:
        args = ["-t", self.config.pyocd_target, "-f", self.config.defaults.pyocd_frequency]
        if reset_type:
            args += ["-O", f"reset_type={reset_type}"]
        if self.config.probe_uid:
            args += ["-u", self.config.probe_uid]
        return args


class PyOcdFlash(PyOcdMixin, FlashStrategy):
    name = "pyocd"
    tool = "pyocd"

    def _args(self) -> list[str]:
        return self.pyocd_args(self.config.defaults.pyocd_flash_reset_type)

    def probes(self) -> list[ProbeInfo]:
        self.require_tool()
        result = self.runner.run(["pyocd", "json", "--probes"])
        data = json.loads(result.stdout)
        return [
            ProbeInfo(b.get("unique_id", ""), b.get("description") or b.get("product_name", ""), self.name)
            for b in data.get("boards", [])
        ]

    def flash(self, image: BuildArtifacts, verify: bool = True) -> None:
        self.require_tool()
        self.runner.run(["pyocd", "load", *self._args(), "--no-reset", image.elf])
        if verify:
            self.verify(image)

    def verify(self, image: BuildArtifacts) -> None:
        """Read the target memory back and compare it with every ELF load segment."""
        segments = elf_load_segments(image.elf)
        with tempfile.TemporaryDirectory(prefix="xat_verify_") as tmp:
            commands: list[str] = []
            for index, segment in enumerate(segments):
                path = Path(tmp) / f"segment{index}.bin"
                path.write_bytes(segment.data)
                commands += ["-c", f"compare 0x{segment.address:08x} {path}"]
            result = self.runner.run(["pyocd", "commander", *self._args(), *commands, "-c", "exit"])
        output = result.stdout + result.stderr
        if re.search(r"mismatch|differ|error", output, re.IGNORECASE):
            raise OperationsError(f"pyocd: verify failed\n{output}")
        total = sum(len(s.data) for s in segments)
        LOG.info("pyocd: verify passed (%d segments, %d bytes)", len(segments), total)

    def erase(self) -> None:
        self.require_tool()
        self.runner.run(["pyocd", "erase", *self._args(), "--chip"])

    def reset(self) -> None:
        self.require_tool()
        self.runner.run(["pyocd", "reset", *self._args()])


class PyOcdGdbServer(PyOcdMixin, DebugServerStrategy):
    name = "pyocd"
    tool = "pyocd"
    ready_pattern = r"GDB server (started|listening) on port"

    def server_command(self) -> list[str]:
        args = self.pyocd_args(self.config.defaults.pyocd_debug_reset_type)
        return ["pyocd", "gdbserver", *args, "-p", str(self.config.gdb_port), "--persist"]


# ---- NXP LinkServer -----------------------------------------------------------------


class LinkServerMixin:
    """Common NXP LinkServer arguments (CLI verified against LinkServer 26.9.130)."""

    config: Config

    def device_args(self) -> list[str]:
        """``[--probe UID] DEVICE``: the probe option must come before the device."""
        args = [self.config.defaults.linkserver_device]
        if self.config.probe_uid:
            args = ["--probe", self.config.probe_uid] + args
        return args


def parse_table(text: str) -> list[dict[str, str]]:
    """Parse a LinkServer text table: a header line, a dashed line, then rows.

    Column boundaries come from the dashed line, so cells may contain single spaces.
    """
    lines = text.splitlines()
    for index, line in enumerate(lines):
        if index > 0 and re.fullmatch(r"\s*-+(\s+-+)*\s*", line):
            spans = [m.span() for m in re.finditer(r"-+", line)]
            # The last column runs to the end of the line.
            spans[-1] = (spans[-1][0], None)  # type: ignore[assignment]
            header = [lines[index - 1][a:b].strip() for a, b in spans]
            rows = []
            for row in lines[index + 1 :]:
                if not row.strip():
                    break
                rows.append({h: row[a:b].strip() for h, (a, b) in zip(header, spans)})
            return rows
    return []


class LinkServerFlash(LinkServerMixin, FlashStrategy):
    name = "linkserver"
    tool = "LinkServer"

    def probes(self) -> list[ProbeInfo]:
        self.require_tool()
        result = self.runner.run(["LinkServer", "probes"])
        return [ProbeInfo(r.get("Serial", ""), r.get("Description", ""), self.name) for r in parse_table(result.stdout)]

    def flash(self, image: BuildArtifacts, verify: bool = True) -> None:
        self.require_tool()
        self.runner.run(["LinkServer", "flash", *self.device_args(), "load", "--no-reset", image.elf])
        if verify:
            self.runner.run(["LinkServer", "flash", *self.device_args(), "verify", image.elf])
            LOG.info("linkserver: verify passed")

    def erase(self) -> None:
        self.require_tool()
        self.runner.run(["LinkServer", "flash", *self.device_args(), "erase"])

    def reset(self) -> None:
        self.require_tool()
        self.runner.run(["LinkServer", "reset", *self.device_args()])


class LinkServerGdbServer(LinkServerMixin, DebugServerStrategy):
    name = "linkserver"
    tool = "LinkServer"
    ready_pattern = r"GDB server listening on port \d+"
    # LinkServer requires 'load' after connecting (it needs the image to reset into);
    # without it the PC stays in the flash driver in RAM and 'monitor reset' fails.
    session_commands = ("load", "monitor reset")

    def server_command(self) -> list[str]:
        return [
            "LinkServer",
            "gdbserver",
            "--core",
            self.config.core_id,
            "--gdb-port",
            str(self.config.gdb_port),
            "--semihost-port",
            "-1",
            *self.device_args(),
        ]


# ---- SEGGER J-Link ------------------------------------------------------------------


class JLinkFlash(FlashStrategy):
    """Needs a J-Link probe (or MCU-Link reflashed with J-Link firmware)."""

    name = "jlink"
    tool = "JLinkExe"

    def _exe(self, commands: Sequence[str]) -> CommandResult:
        self.require_tool()
        args = ["JLinkExe", "-nogui", "1", "-device", self.config.defaults.jlink_device, "-if", "SWD"]
        args += ["-speed", "4000", "-autoconnect", "1", "-ExitOnError", "1"]
        if self.config.probe_uid:
            args += ["-USB", self.config.probe_uid]
        with tempfile.NamedTemporaryFile("w", suffix=".jlink", delete=False) as script:
            script.write("\n".join([*commands, "q"]) + "\n")
        try:
            return self.runner.run([*args, "-CommanderScript", script.name])
        finally:
            os.unlink(script.name)

    def probes(self) -> list[ProbeInfo]:
        result = self._exe(["ShowEmuList"])
        return [
            ProbeInfo(m.group(1), m.group(2).strip(), self.name)
            for m in re.finditer(r"Serial number:\s*(\d+),\s*ProductName:\s*([^\n,]+)", result.stdout)
        ]

    def flash(self, image: BuildArtifacts, verify: bool = True) -> None:
        base = f"0x{self.config.defaults.flash_base:08x}"
        commands = ["r", "h", f"loadbin {image.bin} {base}"]
        if verify:
            commands.append(f"verifybin {image.bin} {base}")
        self._exe(commands)

    def erase(self) -> None:
        self._exe(["r", "h", "erase"])

    def reset(self) -> None:
        self._exe(["r", "g"])


class JLinkGdbServer(DebugServerStrategy):
    name = "jlink"
    tool = "JLinkGDBServerCLExe"
    ready_pattern = r"Waiting for GDB connection"
    session_commands = ("monitor reset",)  # J-Link halts after reset

    def server_command(self) -> list[str]:
        args = ["JLinkGDBServerCLExe", "-nogui", "-device", self.config.defaults.jlink_device]
        args += ["-if", "SWD", "-speed", "4000", "-port", str(self.config.gdb_port), "-noir"]
        if self.config.probe_uid:
            args += ["-select", f"USB={self.config.probe_uid}"]
        return args


# =====================================================================================
# Registry
# =====================================================================================

BUILDERS: dict[str, type[BuildStrategy]] = {
    CMakeNinjaBuild.name: CMakeNinjaBuild,
    WestBuild.name: WestBuild,
}
FLASHERS: dict[str, type[FlashStrategy]] = {
    PyOcdFlash.name: PyOcdFlash,
    LinkServerFlash.name: LinkServerFlash,
    JLinkFlash.name: JLinkFlash,
}
DEBUGGERS: dict[str, type[DebugServerStrategy]] = {
    PyOcdGdbServer.name: PyOcdGdbServer,
    LinkServerGdbServer.name: LinkServerGdbServer,
    JLinkGdbServer.name: JLinkGdbServer,
}

# 'auto' tries these first, then any other registered backend in registration order.
AUTO_ORDER: tuple[str, ...] = ("pyocd", "linkserver", "jlink")


def select_backend(registry: Mapping[str, type[Backend]], name: str, config: Config, runner: CommandRunner) -> Any:
    """Create the backend called ``name``; 'auto' picks the first available one.

    If no backend is available, 'auto' returns the first candidate so the error is
    reported (with a clear "tool not found" message) only when it is actually used.
    """
    if name != "auto":
        if name not in registry:
            raise OperationsError(f"unknown backend '{name}', choose from: {', '.join(registry)}")
        return registry[name](config, runner)
    order = [n for n in AUTO_ORDER if n in registry] + [n for n in registry if n not in AUTO_ORDER]
    candidates = [registry[n](config, runner) for n in order]
    for backend in candidates:
        if backend.is_available():
            LOG.debug("auto selected backend '%s'", backend.name)
            return backend
    LOG.warning("no available backend among %s", ", ".join(order))
    return candidates[0]


# =====================================================================================
# Target (Strategy context)
# =====================================================================================


@dataclass
class Target:
    """The board/application being worked on. Delegates every operation to its strategies."""

    config: Config
    build_strategy: BuildStrategy
    flash_strategy: FlashStrategy
    debug_strategy: DebugServerStrategy

    @classmethod
    def create(cls, config: Config, runner: CommandRunner) -> "Target":
        return cls(
            config=config,
            build_strategy=select_backend(BUILDERS, config.builder, config, runner),
            flash_strategy=select_backend(FLASHERS, config.flasher, config, runner),
            debug_strategy=select_backend(DEBUGGERS, config.debugger, config, runner),
        )

    @property
    def elf(self) -> Path:
        return self.build_strategy.artifacts().elf

    def build(self, clean: bool = False) -> BuildArtifacts:
        if clean:
            self.build_strategy.clean()
        self.build_strategy.configure()
        self.build_strategy.build()
        return self.build_strategy.artifacts()

    def flash(self, verify: bool = True) -> None:
        self.flash_strategy.flash(self.build_strategy.artifacts(), verify=verify)
        self.flash_strategy.reset()

    def erase(self) -> None:
        self.flash_strategy.erase()

    def reset(self) -> None:
        self.flash_strategy.reset()

    def probes(self) -> list[ProbeInfo]:
        return self.flash_strategy.probes()

    def debug_server(self) -> DebugServerStrategy:
        return self.debug_strategy


# =====================================================================================
# SerialMonitor
# =====================================================================================


class SerialMonitor:
    """Reads the debug UART line by line, optionally waiting for an expected text."""

    def __init__(
        self,
        port: str,
        baud: int,
        out: TextIO | None = None,
        serial_factory: Callable[..., Any] | None = None,
    ) -> None:
        self.port = port
        self.baud = baud
        self.out = out or sys.stdout
        self._serial_factory = serial_factory
        self._serial: Any = None

    @staticmethod
    def find_port(vid: int) -> Optional[str]:
        """Return the first serial port whose USB vendor id matches (needs pyserial)."""
        try:
            from serial.tools import list_ports  # lazy optional dependency
        except ImportError:
            return None
        ports = sorted((p for p in list_ports.comports() if p.vid == vid), key=lambda p: p.device)
        return ports[0].device if ports else None

    def open(self) -> "SerialMonitor":
        factory = self._serial_factory
        if factory is None:
            try:
                import serial  # lazy optional dependency
            except ImportError as exc:
                raise ToolNotFoundError("pyserial is not installed (see docs/setup_host.md)") from exc
            factory = serial.Serial
        LOG.info("opening %s at %d baud", self.port, self.baud)
        try:
            self._serial = factory(self.port, self.baud, timeout=0.2)
        except Exception as exc:  # serial.SerialException and OSError
            raise OperationsError(f"cannot open serial port {self.port}: {exc}") from exc
        if hasattr(self._serial, "reset_input_buffer"):
            self._serial.reset_input_buffer()
        return self

    def close(self) -> None:
        if self._serial is not None:
            self._serial.close()
            self._serial = None

    def __enter__(self) -> "SerialMonitor":
        return self.open()

    def __exit__(self, *exc: object) -> None:
        self.close()

    def watch(self, expect: str | None = None, timeout: float | None = None) -> bool:
        """Echo received lines. Return True once ``expect`` is seen, False on timeout.

        Without ``expect`` it runs until the timeout (or forever if timeout is None) and
        returns True.
        """
        if self._serial is None:
            raise OperationsError("serial port is not open")
        deadline = None if timeout is None else time.monotonic() + timeout
        pending = ""
        while deadline is None or time.monotonic() < deadline:
            chunk = self._serial.read(256)
            if not chunk:
                continue
            pending += chunk.decode("utf-8", errors="replace")
            *lines, pending = pending.split("\n")
            for line in lines:
                self.out.write(line.rstrip("\r") + "\n")
                self.out.flush()
                if expect is not None and expect in line:
                    return True
        return expect is None


# =====================================================================================
# GdbClient
# =====================================================================================


class GdbClient:
    """Runs GDB against a GdbEndpoint, either interactively or in batch mode."""

    def __init__(self, gdb: str, runner: CommandRunner) -> None:
        self.gdb = gdb
        self.runner = runner

    @staticmethod
    def find(config: Config, runner: CommandRunner) -> str:
        candidates = [config.gdb] if config.gdb else list(config.defaults.gdb_candidates)
        for candidate in candidates:
            path = runner.which(candidate)
            if path and GdbClient._works(path, runner):
                return path
        raise ToolNotFoundError(f"no working GDB found (tried {', '.join(candidates)})")

    @staticmethod
    def _works(path: str, runner: CommandRunner) -> bool:
        """Some toolchain GDB builds fail to start (missing libpython); skip those."""
        try:
            return runner.run([path, "--version"], check=False).returncode == 0
        except OperationsError:
            return False

    def command(self, endpoint: GdbEndpoint, elf: Path, script: Path | None = None) -> list[str]:
        cmd = [self.gdb, "-q", "-nx", str(elf), "-ex", f"target extended-remote {endpoint.address}"]
        for command in endpoint.session_commands:  # every session starts from a halted reset
            cmd += ["-ex", command]
        if script is not None:
            cmd = [*cmd[:1], "-batch", *cmd[1:], "-x", str(script)]
        return cmd

    def run_batch(self, endpoint: GdbEndpoint, elf: Path, script: Path) -> CommandResult:
        result = self.runner.run(self.command(endpoint, elf, script), check=False, merge_output=True)
        transcript = result.stdout + result.stderr
        if result.returncode != 0 or re.search(r"^(Remote communication error|.*: No such file)", transcript, re.M):
            cmd = self.command(endpoint, elf, script)
            raise CommandError(cmd, result.returncode or 1, result.stdout, result.stderr)
        return result

    def run_interactive(self, endpoint: GdbEndpoint, elf: Path) -> int:
        # Ctrl-C belongs to GDB (it interrupts the target). Python gets a no-op handler, not
        # SIG_IGN, because an ignored signal would be inherited by GDB.
        previous = signal.signal(signal.SIGINT, lambda *_: None)
        try:
            return self.runner.run(self.command(endpoint, elf), capture=False, check=False).returncode
        finally:
            signal.signal(signal.SIGINT, previous)


# =====================================================================================
# CLI
# =====================================================================================


@dataclass
class Check:
    name: str
    ok: bool
    detail: str
    required: bool = True


def parse_version(text: str) -> Optional[tuple[int, ...]]:
    match = re.search(r"(\d+)\.(\d+)(?:\.(\d+))?", text)
    return tuple(int(g) for g in match.groups() if g is not None) if match else None


def version_ok(text: str, minimum: str) -> bool:
    found = parse_version(text)
    return found is not None and found >= (parse_version(minimum) or ())


class Doctor:
    """Collects host checks for the 'doctor' command."""

    def __init__(self, config: Config, runner: CommandRunner) -> None:
        self.config = config
        self.runner = runner

    def _tool_version(self, tool: str, args: Sequence[str] = ("--version",)) -> Optional[str]:
        path = self.runner.which(tool)
        if path is None:
            return None
        result = self.runner.run([path, *args], check=False)
        lines = (result.stdout or result.stderr).strip().splitlines()
        return lines[0] if result.returncode == 0 and lines else None

    def _versioned(self, name: str, tool: str, minimum: str | None = None) -> Check:
        version = self._tool_version(tool)
        if version is None:
            return Check(name, False, f"'{tool}' not found")
        if minimum and not version_ok(version, minimum):
            return Check(name, False, f"{version} (need >= {minimum})")
        return Check(name, True, version)

    def checks(self) -> list[Check]:
        cfg = self.config
        d = cfg.defaults
        out = [
            Check("SDK", (cfg.sdk_root / "cmake" / "extension" / "mcux.cmake").is_file(), str(cfg.sdk_root)),
            Check("ARMGCC_DIR", cfg.armgcc_dir is not None and cfg.armgcc_dir.is_dir(), str(cfg.armgcc_dir)),
            self._versioned("arm-none-eabi-gcc", "arm-none-eabi-gcc"),
            self._versioned("cmake", "cmake", d.min_cmake),
            self._versioned("ninja", "ninja", d.min_ninja),
        ]
        try:
            out.append(Check("gdb", True, GdbClient.find(cfg, self.runner)))
        except ToolNotFoundError as exc:
            out.append(Check("gdb", False, str(exc)))
        out.append(self._versioned("pyocd", "pyocd"))
        out.append(self._pyocd_target())
        out.append(self._probe())
        out.append(self._serial())
        for registry, kind in ((FLASHERS, "flasher"), (DEBUGGERS, "debugger")):
            for name, cls in registry.items():
                available = cls(cfg, self.runner).is_available()
                out.append(Check(f"{kind}:{name}", available, "available" if available else "not installed", False))
        west = self.runner.which("west")
        out.append(Check("builder:west", west is not None, west or "not installed", False))
        return out

    def _pyocd_target(self) -> Check:
        name = f"pyocd target {self.config.pyocd_target}"
        if self.runner.which("pyocd") is None:
            return Check(name, False, "pyocd not installed")
        result = self.runner.run(["pyocd", "list", "--targets"], check=False)
        found = re.search(rf"^\s*{re.escape(self.config.pyocd_target)}\s", result.stdout, re.M | re.I)
        return Check(name, bool(found), "supported" if found else "missing: pyocd pack install mcxn947")

    def _probe(self) -> Check:
        usb_id = self.config.defaults.probe_usb_id
        if self.runner.which("lsusb") is None:
            return Check("probe", False, "lsusb not found")
        result = self.runner.run(["lsusb", "-d", usb_id], check=False)
        line = result.stdout.strip().splitlines()[0] if result.stdout.strip() else ""
        return Check("probe", bool(line), line or f"no USB device {usb_id}")

    def _serial(self) -> Check:
        port = self.config.serial_port or SerialMonitor.find_port(self.config.defaults.serial_vid)
        if not port:
            return Check("serial port", False, "MCU-Link VCOM not found (is pyserial installed?)")
        writable = os.access(port, os.R_OK | os.W_OK)
        return Check("serial port", writable, port if writable else f"{port} (no permission: join 'dialout')")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="operations.py", description="xAT build / flash / debug tool")
    parser.add_argument("-v", "--verbose", action="store_true", help="debug logging (shows tool output)")
    parser.add_argument("--sdk", dest="sdk_root", help="SDK root (default <workspace>/mcuxsdk, env XAT_SDK_ROOT)")
    parser.add_argument("--armgcc", dest="armgcc_dir", help="Arm GNU toolchain root (env ARMGCC_DIR)")
    parser.add_argument("--board", help=f"board (default {DEFAULTS.board})")
    parser.add_argument("--core", dest="core_id", help=f"core id (default {DEFAULTS.core_id})")
    parser.add_argument("--app", dest="app_dir", help=f"application dir (default {DEFAULTS.app})")
    parser.add_argument("--config", dest="build_config", choices=("debug", "release"), help="build configuration")
    sub = parser.add_subparsers(dest="command", required=True)

    def with_flasher(p: argparse.ArgumentParser) -> None:
        p.add_argument("--flasher", choices=["auto", *FLASHERS], help="flash backend (default auto)")
        p.add_argument("--probe", dest="probe_uid", help="probe unique id")

    def with_serial(p: argparse.ArgumentParser) -> None:
        p.add_argument("--port", dest="serial_port", help="serial port (default: auto-detect MCU-Link VCOM)")
        p.add_argument("--baud", type=int, help=f"baud rate (default {DEFAULTS.baud})")

    sub.add_parser("doctor", help="check the host toolchain, probe and serial port")

    p_build = sub.add_parser("build", help="configure and build the application")
    p_build.add_argument("--builder", choices=list(BUILDERS), help="build backend (default cmake)")
    p_build.add_argument("--clean", action="store_true", help="remove the build directory first")

    p_flash = sub.add_parser("flash", help="program the built image and reset the MCU")
    with_flasher(p_flash)
    p_flash.add_argument("--no-verify", dest="verify", action="store_false", help="skip read-back verify")
    p_flash.add_argument("--erase", action="store_true", help="only erase the chip")

    p_debug = sub.add_parser("debug", help="start a GDB server and attach GDB")
    p_debug.add_argument("--debugger", choices=["auto", *DEBUGGERS], help="debug server backend (default auto)")
    p_debug.add_argument("--probe", dest="probe_uid", help="probe unique id")
    p_debug.add_argument("--port", dest="gdb_port", type=int, help=f"GDB server port (default {DEFAULTS.gdb_port})")
    p_debug.add_argument("--gdb", help="GDB executable (default: arm-none-eabi-gdb, then gdb-multiarch)")
    p_debug.add_argument("--batch", type=Path, help="run this GDB script in batch mode and exit")

    p_mon = sub.add_parser("monitor", help="show the debug UART output")
    with_serial(p_mon)
    with_flasher(p_mon)
    p_mon.add_argument("--expect", help="exit 0 when this text is received, non-zero on timeout")
    p_mon.add_argument("--timeout", type=float, help="seconds to wait (default: 10 with --expect, else forever)")
    p_mon.add_argument(
        "--no-reset", dest="reset", action="store_false", help="do not reset the MCU after opening the port"
    )

    p_run = sub.add_parser("run", help="build + flash + monitor --expect (smoke test)")
    with_serial(p_run)
    with_flasher(p_run)
    p_run.add_argument("--clean", action="store_true", help="clean build")
    p_run.add_argument("--expect", default=DEFAULTS.expect, help=f"text to wait for (default '{DEFAULTS.expect}')")
    p_run.add_argument("--timeout", type=float, default=DEFAULTS.monitor_timeout, help="seconds to wait")
    return parser


CONFIG_ARGS = (
    "sdk_root", "armgcc_dir", "board", "core_id", "app_dir", "build_config", "builder", "flasher",
    "debugger", "probe_uid", "gdb", "gdb_port", "serial_port", "baud",
)


def config_from_args(args: argparse.Namespace, environ: Mapping[str, str] | None = None) -> Config:
    return Config.resolve({k: getattr(args, k, None) for k in CONFIG_ARGS}, environ)


def resolve_serial_port(config: Config) -> str:
    port = config.serial_port or SerialMonitor.find_port(config.defaults.serial_vid)
    if not port:
        raise OperationsError("serial port not found; pass --port (is pyserial installed?)")
    return port


def cmd_doctor(config: Config, runner: CommandRunner, args: argparse.Namespace) -> int:
    checks = Doctor(config, runner).checks()
    width = max(len(c.name) for c in checks)
    for c in checks:
        status = "OK  " if c.ok else ("FAIL" if c.required else "--  ")
        print(f"[{status}] {c.name:<{width}}  {c.detail}")
    failed = [c.name for c in checks if c.required and not c.ok]
    usable_flasher = any(c.ok for c in checks if c.name.startswith("flasher:"))
    if not usable_flasher:
        failed.append("no flash backend")
    print(f"\n{'all required checks passed' if not failed else 'FAILED: ' + ', '.join(failed)}")
    return EXIT_OK if not failed else EXIT_FAILURE


def cmd_build(config: Config, runner: CommandRunner, args: argparse.Namespace) -> int:
    artifacts = Target.create(config, runner).build(clean=args.clean)
    print(f"ELF: {artifacts.elf}\nBIN: {artifacts.bin}")
    if artifacts.map:
        print(f"MAP: {artifacts.map}")
    if runner.which("arm-none-eabi-size"):
        print(runner.run(["arm-none-eabi-size", artifacts.elf]).stdout.rstrip())
    return EXIT_OK


def cmd_flash(config: Config, runner: CommandRunner, args: argparse.Namespace) -> int:
    target = Target.create(config, runner)
    print(f"flasher: {target.flash_strategy.name}", flush=True)
    if args.erase:
        target.erase()
        print("erase done")
        return EXIT_OK
    target.flash(verify=args.verify)
    print(f"flash done ({'verified' if args.verify else 'not verified'}), MCU reset")
    return EXIT_OK


def cmd_debug(config: Config, runner: CommandRunner, args: argparse.Namespace) -> int:
    target = Target.create(config, runner)
    gdb = GdbClient(GdbClient.find(config, runner), runner)
    elf = target.elf
    print(f"debugger: {target.debug_strategy.name}, gdb: {gdb.gdb}", flush=True)
    with target.debug_server() as endpoint:
        if args.batch is not None:
            script = args.batch if args.batch.is_absolute() else (Path.cwd() / args.batch)
            result = gdb.run_batch(endpoint, elf, script.resolve())
            print((result.stdout + result.stderr).rstrip())
            return EXIT_OK
        return gdb.run_interactive(endpoint, elf)


def watch_serial(
    config: Config,
    target: Target | None,
    expect: str | None,
    timeout: float | None,
    before: Callable[[], None] | None = None,
) -> int:
    """Open the UART, then reset the MCU (or run ``before``), then watch for ``expect``."""
    with SerialMonitor(resolve_serial_port(config), config.baud) as monitor:
        if before is not None:
            before()
        elif target is not None:
            target.reset()
        if monitor.watch(expect, timeout):
            if expect:
                print(f"\nexpected text found: {expect!r}")
            return EXIT_OK
    print(f"\ntimeout: {expect!r} not received within {timeout}s", file=sys.stderr)
    return EXIT_TIMEOUT


def cmd_monitor(config: Config, runner: CommandRunner, args: argparse.Namespace) -> int:
    default_timeout = config.defaults.monitor_timeout if args.expect else None
    timeout = args.timeout if args.timeout is not None else default_timeout
    target = Target.create(config, runner) if args.reset else None
    try:
        return watch_serial(config, target, args.expect, timeout)
    except KeyboardInterrupt:
        return EXIT_OK


def cmd_run(config: Config, runner: CommandRunner, args: argparse.Namespace) -> int:
    target = Target.create(config, runner)
    artifacts = target.build(clean=args.clean)
    print(f"built {artifacts.elf}")
    # Open the UART before flashing so the boot message printed after reset is not missed.
    return watch_serial(config, target, args.expect, args.timeout, before=lambda: target.flash(verify=True))


COMMANDS: Mapping[str, Callable[[Config, CommandRunner, argparse.Namespace], int]] = {
    "doctor": cmd_doctor,
    "build": cmd_build,
    "flash": cmd_flash,
    "debug": cmd_debug,
    "monitor": cmd_monitor,
    "run": cmd_run,
}


# =====================================================================================
# main()
# =====================================================================================


def main(argv: Sequence[str] | None = None, runner_factory: Callable[[Config], CommandRunner] | None = None) -> int:
    args = build_parser().parse_args(argv)
    # SIGTERM (e.g. from 'timeout') becomes SystemExit so context managers still stop
    # any debug server that was started.
    signal.signal(signal.SIGTERM, lambda *_: sys.exit(128 + signal.SIGTERM))
    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format="%(levelname)s %(message)s",
    )
    try:
        config = config_from_args(args)
        runner = (runner_factory or (lambda c: CommandRunner(c.tool_dirs())))(config)
        return COMMANDS[args.command](config, runner, args)
    except OperationsError as exc:
        LOG.error("%s", exc)
        return EXIT_FAILURE


if __name__ == "__main__":
    sys.exit(main())
