# OpenLCB C Library Component Wrapper

This directory wraps the third-party OpenLCB C library as an ESP-IDF component.

## Git Submodule Warning

> [!WARNING]
> The directory `OpenLcbCLib` is an external **Git Submodule** pointing to the upstream repository:
> **[JimKueneman/OpenLcbCLib](https://github.com/JimKueneman/OpenLcbCLib)**
>
> **Do not modify files inside `OpenLcbCLib` directly.** Any local changes will be lost or cause conflicts when the submodule is updated.

---

## Commands for Submodule Management

### Initialize and Fetch
If you have just cloned the parent repository and this folder is empty, run the following in the project root:
```bash
git submodule update --init --recursive
```

### Pull Upstream Updates
To update the submodule to the latest upstream version on the remote master branch:
```bash
git submodule update --remote --merge
```

---

## How it was Added (for Reference)

The submodule was added to the project by running the following command from the project root:
```bash
git submodule add https://github.com/JimKueneman/OpenLcbCLib.git components/lcc/OpenLcbCLib
```
This registers the submodule in [.gitmodules](file:///home/robert/CLionProjects/esp/lcc-node-2/.gitmodules) and downloads the repository into the specified path.

