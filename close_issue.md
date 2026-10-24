# Closed Issues

本檔記錄已遇到、已修正並完成驗證的問題。

## 1. `config/versions.env` 不是有效的 shell assignment

問題：版本值寫成 `QEMU_VERSION= v9.2.0`，source 時 shell 會把 `v9.2.0` 當成命令執行。

解法：移除 `=` 後的空白，改成：

```sh
QEMU_VERSION=v9.2.0
LINUX_VERSION=v6.12
BUILDROOT_VERSION=2025.02
```

原因：shell variable assignment 的 `=` 前後不能有未引用空白。修正後已用 `bash -n`、`source` 與值比對驗證。

## 2. Homebrew 建置依賴未安裝

問題：初始 `host-check.sh` 找不到 QEMU、Meson、Ninja。

解法：安裝：

```sh
brew install qemu meson ninja pkg-config
```

原因：QEMU 使用 Meson/Ninja 建置，ARM64 guest 執行需要 `qemu-system-aarch64`。修正後 `host-check.sh` 回報 `host-check: OK`。

## 3. QEMU configure 被 Python 執行

問題：`build-qemu.sh` 使用 `python3 ./configure`，導致 shell script 出現 Python `SyntaxError`。

解法：改成直接執行：

```sh
./configure ...
```

原因：QEMU 的 `configure` 是 shell script，不是 Python 程式。

## 4. QEMU 編譯目錄判斷錯誤

問題：QEMU configure 預設產生 `build/qemu/build/build.ninja`，腳本卻檢查 `build/qemu/build.ninja`，並把 Ninja 指向 source tree。

解法：改用：

```sh
ninja -C "$QEMU/build"
ninja -C "$QEMU/build" install
```

原因：QEMU 9.2.0 使用 Meson out-of-tree build。修正後 QEMU 已成功安裝至 `output/qemu/bin/qemu-system-aarch64`。

## 5. QEMU Python configure 缺少 `distlib`

問題：QEMU `mkvenv` 顯示 `found no usable distlib`。

解法：在 Homebrew Python 3.15 安裝：

```sh
python3 -m pip install --user --break-system-packages distlib
```

原因：QEMU configure 需要 distlib 建立其 Python virtual environment。修正後 configure 成功。

## 6. Linux kernel 使用 macOS 內建 GNU Make 3.81

問題：Linux 6.12 要求 GNU Make >= 4.0，但 `/usr/bin/make` 是 3.81。

解法：

```sh
brew install make
```

腳本改用 `/opt/homebrew/opt/make/libexec/gnubin/make`，並允許 `MAKE` 環境變數覆寫。

原因：Linux kernel Makefile 使用新版 GNU Make 語法與功能。

## 7. LLVM kernel build 缺少 `ld.lld`

問題：`LLVM=1` 配置時找不到 `ld.lld`。

解法：

```sh
brew install llvm lld
```

並將以下路徑加入 `build-guest.sh` 的 PATH：

```sh
/opt/homebrew/opt/llvm/bin
/opt/homebrew/opt/lld/bin
```

原因：Homebrew 新版將 LLD 拆成獨立 formula；Apple Clang 本身不保證提供 Linux kernel 所需的 `ld.lld`。

## 8. Linux kernel host tool 缺少 `elf.h`

問題：macOS 沒有 Linux/glibc 風格的 `elf.h`，最先在 `scripts/sorttable` 與 `scripts/mod/mk_elfconfig` 出現。

解法：新增 `host-compat/elf.h`，提供這些 host tools 所需的最小 ELF 型別與常數，並透過 `HOSTCFLAGS_*` 注入 include path。

原因：這些 header 只供 kernel 建置時執行於 host 的工具使用，不是 guest kernel header。

目前 `sorttable` 與 `mk_elfconfig` 已可成功編譯；這個 workaround 後來在 `modpost` 暴露更多 Darwin/Linux 不相容，最後改用 Docker 建置，詳見第 11 項。`host-compat/` 已移除。

## 9. Linux kernel host tool 缺少 `byteswap.h`

問題：`scripts/mod/modpost.h` include `<byteswap.h>`，macOS 沒有該 GNU header。

解法：新增 `host-compat/byteswap.h`，以 macOS `libkern/OSByteOrder.h` 對應 `bswap_16/32/64`。

原因：modpost 只需要 byte-swap API；使用 macOS 對應 API 可以避免修改 Linux source。（已隨 Docker 方案移除。）

## 10. QEMU ARM64 host binary 驗證完成

驗證：

```sh
output/qemu/bin/qemu-system-aarch64 --version
output/qemu/bin/qemu-system-aarch64 -machine virt,help
```

結果：QEMU 9.2.0 與 ARM64 `virt` machine model 均可用。

## 11. macOS 原生編譯 Linux guest 失敗（modpost / 大小寫檔名碰撞）

問題：

- Linux 6.12 的 `scripts/mod/modpost.c`、`file2alias.c` 依賴 Linux/glibc 的 ELF relocation、UUID 定義，與 macOS Darwin headers（`uuid_t`、`gethostuuid()`）衝突，出現 `EI_MAG0`、`R_MIPS_LO16`、`ELF64_R_TYPE` 等未定義錯誤。
- macOS APFS 預設不分大小寫，Linux source 的 `xt_CONNMARK.h` / `xt_connmark.h` 等檔案發生碰撞。
- 因此 `output/guest/Image` 與 `rootfs.ext4` 無法產生。

解法：新增 `Dockerfile`（Ubuntu 24.04 ARM64 + `gcc-aarch64-linux-gnu`）與 `scripts/build-guest-docker.sh`，在 container 內下載並建置 Linux/Buildroot，source 與 build tree 放在 Docker volume（case-sensitive ext4）。

清理：

- 移除 macOS 上的 `build/linux`、`build/buildroot`、`host-compat/`。
- `build-guest.sh` 在 macOS 執行時改為提示使用 Docker 流程。
- `fetch-sources.sh` 在 macOS 只下載 QEMU，在 container 內只下載 Linux/Buildroot。
- `build-guest-docker.sh` 只把 `Image`、`rootfs.ext4` 複製回 macOS，不再複製 13 GB 的 Buildroot 中間檔。

結果：`output/guest/Image` 與 `output/guest/rootfs.ext4` 已產生。

## 12. QEMU 客製 Virtio 裝置缺少 `get_features` 回呼導致斷言失敗

問題：
以 `CUSTOM_DEVICE=1 ./scripts/run.sh` 啟動時發生 abort：
```text
Assertion failed: (vdc->get_features != NULL), function virtio_bus_device_plugged, file virtio-bus.c, line 65.
```
原因：QEMU 在將 virtio device 接上 virtio-bus 時，強制要求 `VirtioDeviceClass` 提供 `get_features` 回呼函式。初始骨架未設定此回呼。

解法：在 [virtio-custom.c](file:///Users/wangshaocheng/Desktop/VM/qemu-virtio-lab/build/qemu/hw/virtio/virtio-custom.c) 實作 `virtio_custom_get_features` 並指派給 `vdc->get_features`。重新編譯後開機成功。
