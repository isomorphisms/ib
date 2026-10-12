# Hosted rish-bundle provenance receipt

Source under test: `0cf33631020d8b371b92bc0214c6056c915dd748`, based on
the retained B01 repair `5205209f189cb3813d18ec251a273d59f369e50e`.

On 2026-10-08 the actual Grease consumer wrapper was
`/workspace/scratch/4cbbf3939bb5/il0/qualification/grease`:

- wrapper SHA-256 `37088097c36a2f82cf45aaea9392e4a4cf16155fecb9da87d08a9c7fb4d92e4a`;
- implementation executable SHA-256 `7e31cd05b7a9d8fb2a4a9e003a7f3fcb0159138506d17f0fb28da8cbe22aa85c`;
- observed implementation revision `5651cf97a1b5042f24f14112a7ade9a1518eb0bc`.

Invocation used `ASAN_OPTIONS=detect_leaks=0` because LeakSanitizer cannot
inspect the container's process tree, and explicit `+O ysh:all` inherited
compatibility mode. The initial default-mode attempt failed on the inherited
nested command substitution; this receipt does not claim default-mode source
acceptance or a maintained consumer build qualification.

| Executed case | Result |
| --- | --- |
| `tests/test_rish_bundle_provenance.grease` | PASS, exit 0, ten positive/refusal/recovery groups. |
| `tests/test_android_device_capabilities.grease` | PASS, exit 0, existing report regression. |
| Controlled mutant always accepting active-pair hashes | Rejected, exit 1 at the changed-active-byte case. |
| Local observation changes input files or executes fixture launcher | Rejected by equality/no-invocation controls. |
| Physical MIRO A1/C67, Shizuku, reboot, ART or actual UID | NOT_RUN. |
| Durable automatic Grease CI runtime provisioning | BLOCKED, no durable runtime artifact was introduced. |

The launcher in the new fixture is opaque external `/bin/sh` test data and is
not executed. The existing report fixture separately uses a simulated external
provider; its reported UID is fixture evidence, not live-device authority.

No source, saved bundle, device identity, inventory, or active DEX is restored
or created by observation. Hashes and the current pointer are observations with
the concurrency limitations recorded in `rish-bundle-provenance.md`.

## 2026-10-12 Binder setup regression

At parent input `3233277c914715892a37d2bf50cd572f4d09e94c`, the dedicated
provenance suite returned exit 1 at its active-pair positive control. The recent
Binder-neutral launcher repair changed a required test dependency: the suite
still loaded only the host and generic capability modules. Launcher resolution
therefore returned unknown rather than the real explicit fixture path.

The suite now loads the actual Shizuku worker/capability providers and Binder
adapter, preserving that production boundary. The complete suite then passes
all ten positive/refusal/recovery groups through actual Grease 5651cf97 in
explicit compatibility mode. The setup repair is also applied and executed on
the dependent storage input `cc5771576d37b5651ecfe39da090901a1f77428d`.
The earlier Binder-boundary and device-capability suites passed on the parent.

Corrected test SHA-256:
`e046971b7d52eee20806b73a3415e8d2ede3a2fcc972d19e1a105e979f775447`.
Existing runtime executable SHA-256 remains `7e31cd05...22aa85c` above; the
current exec-only consumer launcher is
`29c673cf786d167e606af96a167c51cf6ee56d4527cfabdf21d727a269a649ee`.
No source runtime, device files, restore operation or credential was created.

The green exact-head browser-foundation workflow on the input did not execute
this dedicated suite. Automatic qualified Grease coverage of this regression
remains incomplete; historical green checks do not replace its execution.
