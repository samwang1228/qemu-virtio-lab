# Open Issues

本檔記錄目前仍未解決的問題。已解決的問題請見 [close_issue.md](close_issue.md)。

## 待完成

- 以 `./scripts/run.sh` 啟動 guest，在 guest 內掛載 9p 共享目錄並載入客製驅動模組進行測試：
  ```sh
  mkdir -p /mnt
  mount -t 9p -o trans=virtio host_share /mnt
  insmod /mnt/virtio_custom.ko
  dmesg | tail -n 10
  ```
- 驗證 custom Virtio device (`CUSTOM_DEVICE=1 ./scripts/run.sh`) 與 driver probe 成功綁定。
