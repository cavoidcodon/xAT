# Host setup (Ubuntu 24.04, aarch64 / Raspberry Pi)

How the build/flash/debug host for xAT was set up for TASK-001. Every command that was
run is listed. Commands marked **sudo** change the system.

Verified on 2026-10-03: Ubuntu 24.04.4 LTS, `aarch64`, Python 3.12.3, board FRDM-MCXN947
on the on-board MCU-Link (`1fc9:0143`, CMSIS-DAP V3.128).

## Workspace layout

```
workspace/
├── mcuxsdk/                                    MCUXpresso SDK 26.09.00 (read-only, vendor)
├── tools/arm-gnu-toolchain-15.2.rel1-aarch64-arm-none-eabi/   Arm GNU Toolchain
└── xAT/                                        this repository
    └── .venv/                                  Python tools (not committed)
```

`scripts/operations.py` finds the SDK (`<workspace>/mcuxsdk`), the toolchain
(`<workspace>/tools/arm-gnu-toolchain-*-arm-none-eabi`) and the venv (`xAT/.venv/bin`)
relative to its own location. It also searches them before `PATH`, so nothing has to be
activated. Override with `XAT_SDK_ROOT`, `ARMGCC_DIR` or the CLI options.

## 1. Arm GNU Toolchain (AArch64 host → arm-none-eabi)

No sudo. Version **15.2.rel1** (SDK minimum: 13.2.1).

```sh
cd ~/workspace/tools
V=15.2.rel1
N=arm-gnu-toolchain-$V-aarch64-arm-none-eabi
B=https://developer.arm.com/-/media/Files/downloads/gnu/$V/binrel
wget $B/$N.tar.xz $B/$N.tar.xz.sha256asc
sha256sum -c $N.tar.xz.sha256asc     # d061559d814b205ed30c5b7c577c03317ec447ca51cd5a159d26b12a5bbeb20c
tar xJf $N.tar.xz && rm $N.tar.xz $N.tar.xz.sha256asc
```

GDB: `arm-none-eabi-gdb` (GDB 16.3.90) from this toolchain works on this host.
`arm-none-eabi-gdb-py` does **not** work, because it needs `libpython3.8` and Ubuntu 24.04
has 3.12. The tool uses `arm-none-eabi-gdb` and falls back to `gdb-multiarch` if that is
installed (`sudo apt install gdb-multiarch`). Choose a specific GDB with `--gdb` or `XAT_GDB`.

## 2. Python venv, CMake, Ninja, pyOCD

**sudo** (venv support):

```sh
sudo apt-get update
sudo apt-get install -y python3-venv python3-pip
```

No sudo:

```sh
cd ~/workspace/xAT
python3 -m venv .venv
.venv/bin/pip install --upgrade pip
.venv/bin/pip install pyocd==0.45.1 pyserial==3.5 pytest==9.1.1 \
    cmake==3.31.10 ninja==1.13.2 jsonschema==4.26.0 PyYAML==6.0.3
```

| Package | Version | Why |
|---|---|---|
| cmake | 3.31.10 | SDK requires ≥ 3.30 (apt has 3.28.3) |
| ninja | 1.13.2 | SDK requires ≥ 1.12 (apt has 1.11.1) |
| pyocd | 0.45.1 | default flash / GDB server backend (CMSIS-DAP) |
| pyserial | 3.5 | `monitor` (debug UART) |
| pytest | 9.1.1 | unit tests |
| jsonschema, PyYAML | 4.26.0, 6.0.3 | SDK build scripts use them to validate `tool.yml` (without them: harmless warning) |

CMake and Ninja come from pip instead of apt, because the apt versions are below the
SDK minimums. This was approved in the TASK-001 plan.

### pyOCD target pack

pyOCD 0.45.1 has no built-in MCXN947 target, so install the CMSIS pack:

```sh
.venv/bin/pyocd pack install mcxn947        # installs NXP.MCXN947_DFP 26.09.00
.venv/bin/pyocd list --targets | grep -i mcxn947
```

The tool uses target **`mcxn947vdf`** (the board's part number). Reset settings that were
verified on hardware are recorded in `Defaults` in `scripts/operations.py`:

- `load`, `reset`, `commander` use the pack default reset. `reset_type=hw` makes the pack's
  `ResetCatchClear` sequence fail on cm33_core1 (SWD FAULT ACK).
- `gdbserver` uses `reset_type=hw`. With the default reset, every GDB `continue` after
  `monitor reset halt` only single-steps.

## 3. NXP LinkServer 26.9.130 (ARM64 build)

LinkServer has a Linux ARM64 build since 25.12.83. Download
`LinkServer_26.9.130.aarch64.deb.bin` from nxp.com (needs an NXP account) into `~/workspace`.

**sudo**:

```sh
cd ~/workspace
sudo sh LinkServer_26.9.130.aarch64.deb.bin -- acceptLicense skipIdeSelect
sudo ln -sf /usr/local/LinkServer/LinkServer /usr/local/bin/LinkServer
LinkServer --version
```

- `acceptLicense` accepts the NXP license non-interactively. Read `LICENSE` in the archive
  first (`sh LinkServer_*.deb.bin --noexec --target <dir>` unpacks it without installing).
- The installer installs the packages `linkserver_26.9.130`, `mcu-link_installer_3.174` and
  `lpcscrypt 2.1.4`, and pulls in `libusb-1.0-0-dev` and `dfu-util` from apt.
- It installs to `/usr/local/LinkServer_26.9.130`, with a symlink at `/usr/local/LinkServer`.
  That is not on `PATH`, hence the `ln -sf`. Alternatively set
  `XAT_LINKSERVER_DIR=/usr/local/LinkServer`.
- `LinkServer probes` reports that the probe firmware (V3.128) is older than the bundled
  3.174. It was **not** updated; updating is optional (`LinkServer probe '#1' update auto`).

## 4. USB / serial permissions

The LinkServer installer adds udev rules (`/lib/udev/rules.d/85-linkserver_26.9.130.rules`,
`85-mcu-link_3.174.rules`). They give `MODE="0666"` to the MCU-Link USB device, its HID
interfaces **and** `/dev/ttyACM*` (`1fc9:0143`). pyOCD and pyserial therefore work without
a project-specific udev rule and without joining `dialout`.

**sudo**, only needed once after installing, so that devices that are already connected
pick up the rules:

```sh
sudo udevadm trigger
```

Without LinkServer (pyOCD only), use these instead:

```sh
printf '%s\n' \
 'SUBSYSTEM=="usb", ATTR{idVendor}=="1fc9", ATTR{idProduct}=="0143", MODE="0660", GROUP="dialout"' \
 'KERNEL=="hidraw*", ATTRS{idVendor}=="1fc9", ATTRS{idProduct}=="0143", MODE="0660", GROUP="dialout"' \
 | sudo tee /etc/udev/rules.d/50-xat-mcu-link.rules
sudo udevadm control --reload-rules && sudo udevadm trigger
sudo usermod -aG dialout $USER     # log out and in again
```

## 5. Check

```sh
python3 scripts/operations.py doctor
```

Every required line must be `[OK  ]`, and the exit code must be 0. J-Link and west are
optional and show as `--` when they are not installed. J-Link would need a J-Link probe, or
the MCU-Link reflashed with J-Link firmware, which is not done here.
