# Compatibility

These checks used Wine 11.17 on x86_64 Linux and the same shim and native launcher
for all three versions, built with LLVM-MinGW Clang 22.1.7.
Each application ran without external network access or
host hardware. No trial or license was activated during these checks.

| LightBurn | Installation | Startup after 35 seconds | Closing the license dialog |
| --- | --- | --- | --- |
| 2.1.04 | Passed | License Management | Exit 0 |
| 2.1.00 | Passed | License Management | Exit 0 |
| 2.0.05 | Passed | License Management | Exit 0 |

The maintainer separately confirmed editor use and shutdown with 2.1.04.
The isolated checks do not establish editor, licensed-feature, or laser support.

All three versions also passed no-argument repair and subsequent launch
without version overrides. Repair preserved the installed executable and version
record and did not run a LightBurn installer. Launch succeeded with no working
compiler configured, and closing left no native-helper or application processes.

## The 2.0.05 failure

The dialog reported that fingerprint generation failed because WMI was disabled.
Independent Wine WMI queries succeeded before and after a settling interval.
Immediate retry in the same prefix produced the same error.

Wine tracing then showed a successful service-status query for `Winmgmt` before
any observed WMI-provider call. The service was stopped, not disabled. Starting
the existing Wine builtin service through `StartServiceW` and waiting for
`SERVICE_RUNNING` allowed 2.0.05 to open License Management and close with exit 0.
A subsequent stopped-service control reproduced the original error. Version
2.1.00 opened with either service state.

The native launcher now starts Winmgmt through the service-control API and
requires `SERVICE_RUNNING` before starting LightBurn. It waits for native status
notifications if startup is pending. It does not change service configuration,
WMI identity, provider implementation, license checks, or activation state.
The exact private licensing predicate remains unknown.

A private Windows job owns the helper and its descendants. Closing or interrupting
this launch does not terminate other applications in the same Wine prefix.
The helper is built during installation, not on every launch. Existing prefixes
need a no-argument repair to install it.

Automated tests run the production helper against real Wine services with a
synthetic application. First and repeated launches passed on Wine 9.0 and 11.17.
Pending states and failure paths also have focused tests. This does not make
Wine 9.0 a tested runtime for the proprietary application itself.

## Testing another version

Download an installer from the [official release archive](https://release.lightburnsoftware.com/LightBurn/Release/).
Keep experimental versions in separate prefixes and review the applicable EULA.
A new prefix is not permission to restart a trial or exceed activation limits.

For [2.1.00](https://release.lightburnsoftware.com/LightBurn/Release/LightBurn-v2.1.00/),
the checksum is already recorded in `versions.env`.

```bash
export WINEPREFIX="$HOME/.local/share/lightburn-2.1.00/wineprefix"
LB_VERSION=2.1.00 ./scripts/install.sh "$HOME/Downloads/LightBurn-v2.1.00.exe"
./scripts/run.sh
```

For an unlisted version, supply both `LB_VERSION` and `LB_WIN_SHA256` when
installing. Obtain the installer from the vendor and verify its provenance before
recording its digest. A locally computed hash is not a vendor signature.
Launch and no-argument repair need only the chosen `WINEPREFIX`, not those
installer settings. Installation still refuses a known version mismatch rather
than silently replacing an existing application.

There are no LightBurn-version branches in the shim. It implements public WinRT
interfaces, and the tests exercise their layouts and object lifetimes. Future
versions may use additional interfaces or different licensing dependencies;
changing the version setting cannot guarantee compatibility.

## Download records

The official archive linked to these files. Digests were computed from the
downloads; the proprietary files are not included in this repository.

| Version | Official installer | SHA-256 |
| --- | --- | --- |
| 2.1.00 | [Download](https://files.release.lightburnsoftware.com/LightBurn/Release/LightBurn-v2.1.00/LightBurn-v2.1.00.exe) | `00b22facdd24465195a1ce8e92546a9580d216c76ee1c82e505519b7ebcbd8ce` |
| 2.0.05 | [Download](https://files.release.lightburnsoftware.com/LightBurn/Release/LightBurn-v2.0.05/LightBurn-v2.0.05.exe) | `12207cf2ee9700a940f87bea424cdc3edaa8ff1476671a43f7547ef136b10df4` |
