# Open Issues

本檔記錄目前仍未解決的問題。已解決的問題請見 [close_issue.md](close_issue.md)。

目前沒有阻擋 guest kernel/rootfs 建置的問題（macOS 原生編譯問題已改用 Docker 解決，見 close_issue.md 第 11 項）。

## 待完成

- 以 `./scripts/run.sh` 驗證 guest 開機到 `/ #`。
- 套用 `patches/` 的客製 Virtio device 至 `build/qemu`，重新執行 `./scripts/build-qemu.sh`。
- 編譯 `guest-driver/virtio_custom.c` 並放入 rootfs，以 `CUSTOM_DEVICE=1 ./scripts/run.sh` 進行整合驗證。
