# Open Issues

本檔記錄目前仍未解決的問題與後續優化方向。已解決的問題請見 [close_issue.md](close_issue.md)。

## 目前狀態

目前無未解決的阻斷性問題。以下端對端基礎流程已完成實作與驗證：

- **Virtio-9P 掛載與模組載入**：於 guest 內透過 `host_share` 掛載 `guest-driver/` 並成功 `insmod /mnt/virtio_custom.ko`。
- **Custom Virtio-PCI Device 綁定與 DMA 引擎**：以 `CUSTOM_DEVICE=1 ./scripts/run.sh` 啟動後，driver probe 成功綁定 `virtio-custom-pci`（`VIRTIO_ID_CUSTOM 43`，PCI ID `0x106b`），並自動執行 DMA self-test（`COPY` / `INVERT`）。
- **Sysfs 測試與硬體遙測介面**：`/sys/bus/virtio/devices/virtio*/dma_test` 隨選觸發自我測試與 `/sys/bus/virtio/devices/virtio*/dma_stats` 透過 `virtio_cread64()` 讀取 64-bit 硬體組態計數器（`write_count`、`dma_transfers`、`dma_bytes`）皆已驗證通過。

## 後續優化項目（Backlog）

1. **補齊測試覆蓋**：`virtio_custom_run_self_test()` 目前驗證 `VIRTIO_CUSTOM_DMA_COPY` 與 `VIRTIO_CUSTOM_DMA_INVERT`，可再補上 `VIRTIO_CUSTOM_DMA_FILL`、邊界／錯誤路徑（例如超過 `VIRTIO_CUSTOM_MAX_DMA_LEN` 16 MiB 回傳 `-EINVAL`、非法 GPA 回傳 `-EIO`）以及多執行緒併發 DMA 壓力測試。
2. **端對端自動化驗證**：加入非互動式測試腳本（例如透過 QEMU serial 自動登入、掛載 9P、`insmod` 並比對 `dma_test` 與 `dma_stats` 增量）。
3. **協定與邊界細節修正**：在 `hw/virtio/virtio-custom.c` 呼叫 `virtqueue_push()` 時改傳實際寫入 `in_sg` 的位元組數（當 `elem->in_num == 0` 時應為 `0`），並將共用 UAPI 結構（`include/uapi_virtio_custom.h`）改為顯式 little-endian 型別（`__le32` / `__le64` 與 QEMU `virtio_tswap*`）以符合 Virtio-1.0 跨端序規範。
