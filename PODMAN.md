# Podman Support (Additive Path)

This repository maintains Docker as its primary documented and production CI path. Podman is supported as an **additional** option for local development, rootless execution, and container build/smoke verification.

Existing Docker workflows (`.github/workflows/ngwpc-cicd.yml`) and production release tags remain untouched and active.

---

## Prerequisites

1. **Podman Installation:** Verify that Podman is installed on your workstation or runner:
   ```bash
   podman version
   podman info
   ```
   For Ubuntu 24.04+ (Noble) or RHEL 8/9, Podman 4.9+ is recommended.

2. **Git Submodules:** NGen relies on multiple submodules (`t-route`, `cfe`, `noah-owp-modular`, `topmodel`, `LASAM`, `bmi-cxx`, `lstm`, etc.). Clone with recursive submodules, or initialize them before building:
   ```bash
   git submodule update --init --recursive
   ```

3. **Base Image Access:** The build pulls `ghcr.io/ngwpc/ngen-bmi-forcing:latest` as its base. Ensure you are logged into GHCR if accessing private or internal images:
   ```bash
   podman login ghcr.io
   ```

---

## Building with Podman

Build the NextGen Engine image directly using the existing `Dockerfile`. Following NOAA-OWP/WRES conventions, use `--format docker` to ensure standard OCI/Docker compatibility:

```bash
podman build \
  --ulimit nofile=65535:65535 \
  --format docker \
  --build-arg GHCR_ORG=ngwpc \
  --build-arg FORCING_IMAGE="ghcr.io/ngwpc/ngen-bmi-forcing:latest" \
  --build-arg EWTS_ORG=NGWPC \
  --build-arg EWTS_REF=development \
  -f Dockerfile \
  -t local/ngen:podman-test \
  .
```

*Note: NGen compiles C, C++, and Fortran models across dozens of submodules with CMake and Boost. The `--ulimit nofile=65535:65535` flag ensures sufficient file descriptors are available during parallel compilation. Modern Podman (via Buildah $\ge$ 1.24) natively manages the cache mounts declared in the Dockerfile (`--mount=type=cache`).*

---

## Smoke Verification

### 1. Test ngen Binary Usage & Build Information
Running the compiled `ngen` binary without arguments prints the framework build configuration (enabled modules, MPI, NetCDF, UDUNITS, Python, Routing, Nexuses) and returns 0:
```bash
podman run --rm --entrypoint /ngen-app/ngen/cmake_build/ngen local/ngen:podman-test
```

Verify that the wrapper entrypoint script is executable:
```bash
podman run --rm --entrypoint test local/ngen:podman-test -x /ngen-app/bin/run-ngen.sh
```

### 2. Verify Python Runtime & Submodule Imports
Verify the container's Python environment, scientific packages, EWTS logging, and LSTM submodule:
```bash
podman run --rm --entrypoint /ngen-app/ngen-python/bin/python local/ngen:podman-test --version

podman run --rm --entrypoint /ngen-app/ngen-python/bin/python local/ngen:podman-test \
  -c "import numpy, pandas, netCDF4, xarray, ewts; print('Scientific packages and EWTS healthy')"

podman run --rm --entrypoint /ngen-app/ngen-python/bin/python local/ngen:podman-test \
  -c "import lstm; print('LSTM module import healthy')"
```

### 3. Verify Git Provenance Metadata
Inspect the generated provenance metadata combining git information from NGen, EWTS, and all submodules:
```bash
podman run --rm --entrypoint cat local/ngen:podman-test /ngen-app/ngen_git_info.json
podman run --rm --entrypoint test local/ngen:podman-test -s /ngen-app/ngen_git_info.json
```

---

## CI / Automation

* **Workflow:** `.github/workflows/podman-smoke.yml`
* **Triggers:** Manual (`workflow_dispatch`) and automated checks on pull requests modifying NGen engine files (`Dockerfile`, `run-ngen.sh`, `CMakeLists.txt`, `src/**`, `include/**`, `cmake/**`, `data/**`, `.gitmodules`).
* **Runner Environment:** Pinned to `ubuntu-24.04`.
* **Submodules:** Checked out recursively (`submodules: recursive`, `fetch-depth: 0`).
* **Registry Policy:** By default, builds remain local to the runner. When `push_images=true` is dispatched, only `:podman-test` and `:<sha>-podman-test` tags are published to GHCR. Production aliases (`:latest`, branch tags) are never touched.
