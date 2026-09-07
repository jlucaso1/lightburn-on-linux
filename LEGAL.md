# Licensing and publication review

This is a technical review, not legal advice or a certification that publication
is permitted. The MIT grant covers original project contributions whose authors
have the right to license them. It does not relicense LightBurn or override any
agreement with its vendor.

## Findings

The inspected Git files and available local history did not contain a LightBurn
installer, extracted application, vendor PDB, license key, or Wine prefix. No
apparent activation bypass, key generator, forged licensing response, clock
manipulation, or trial-reset code was found. The shim implements empty camera
enumeration; activation remains the application's responsibility.

The maintainer confirmed that the shim was wholly authored for this project,
with no copied third-party code. This records the maintainer's authorship
statement, not independent verification of compiled binaries or runtime code.

These findings are not a complete binary audit. The public repository starts
with a source-only history. The earlier compiled shim and development history
are preserved locally and are not part of the publication. Review incorporated
runtime licenses and required notices before distributing compiled DLLs.

Personal download paths and earlier commit metadata are excluded from the new
public history. Public commits use a GitHub noreply address. No apparent
credentials were found in the inspected Git content. Ignored files, external
release assets, and private local data are not covered by that finding.

## Unresolved EULA Question

The [official published LightBurn EULA](https://docs.lightburnsoftware.com/latest/Licensing/LightBurnEULA/)
identifies itself as Version June 2026. Its preamble says the EULA provided with
each software version covers that version only. The exact agreement applicable
to the 2.1.04 installer has not been established from its contents.

The inspected `LightBurn-v2.1.04.exe` has SHA-256
`1209eb5c8467a9aefa4eabbace5e982e232f8352db24797bb01f56671299b17b`.
Innoextract 1.10-dev successfully listed its Inno Setup 6.3.0 payload, which
contained no standalone file named as a license or EULA. A license-only reader
using the same parser found empty license fields in both the setup header and
its single default language entry. No application resources or program code
were examined to locate further terms. These checks do not prove that no EULA
is supplied elsewhere or that no terms apply. The published EULA also says
downloading, installing, or using the software constitutes acceptance; the
absence of an acceptance dialog does not resolve that question.

Section 1.9.d restricts modifying, disassembling, reverse compiling, and reverse
engineering the software. It describes an interoperability exception where
applicable law prohibits the restriction, and says to first contact LightBurn
to give it an opportunity to provide the needed changes. Other jurisdictional
provisions may apply.

The original investigation used the vendor's PDB, `llvm-symbolizer`, `winedbg`,
and gdbproxy to inspect startup behavior and interface layout. That does not
prove a violation, but it prevents an unconditional
claim of compliance. A shipped PDB is not express permission to reverse engineer.
The repository does not establish prior vendor permission or a legally
applicable exception. Get written clarification from LightBurn covering the
investigation and shim distribution, or a jurisdiction-specific legal review,
before declaring this issue resolved.

Sections 1.9.a and 4 also restrict redistribution and reserve the vendor's
intellectual property rights. The [official download page](https://lightburnsoftware.com/pages/download-trial)
offers a trial, not a general redistribution license. Downloading from the
vendor's server does not authorize mirroring the installer on GitHub or AUR.

## Before Publication

- Resolve the applicable EULA and development-history question. Do not advertise this review as vendor approval.
- Retain the shim authorship confirmation and confirm the right to license all original project contributions under MIT.
- Review runtime notices and build provenance before adding binary releases.
- Keep the private development history separate from the public branch.
- Keep installers, vendor binaries, PDBs, activation data, prefixes, and private logs out of Git and release archives. `.gitignore` is only an accident-prevention measure.
- Require users to obtain the official installer and accept its applicable terms before installation. Use a valid license or authorized trial; do not reset trials or bypass activation limits.
- Describe the project as unofficial and retain the camera and untested-hardware warnings.

The [LightBurn website terms](https://lightburnsoftware.com/policies/terms-of-service)
explicitly distinguish website terms from the software EULA. Review the software
agreement rather than treating website access or a successful download as
permission to redistribute or reverse engineer the application.
