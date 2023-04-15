# USB test v1.1

## Dependencies

- FFTW3

  ```bash
  # fftw 3.3.8
  sudo apt-get install libfftw3-dev
  ```

  在 CMakeLists.txt 中使用 fftw3 連結

- spdlog
- [fast-cpp-csv-parser](https://github.com/ben-strasser/fast-cpp-csv-parser.git)
- libudev
- libusb-1.0
- libserial

## WSL 安裝

- WSL 中需要安裝 https://learn.microsoft.com/en-us/windows/wsl/connect-usb 的 usbipd-win 才能使用 USB 設備
- 安裝完後重開機
- `usbipd wsl list` in Powershell
- `sudo usbipd wsl attach --busid 1-1 --auto-attach` in Powershell if you install gsudo
- `lsusb` in WSL terminal or `ll /sys/class/tty/ttyACM* `
- 如果要用 Serial open 的話，STM 應該會出現於 ttyACM?

## 如果遇到 open permission denied 問題

- [WSL2 中連接 USB 的疑難雜症](https://hackmd.io/@DennisLiu16/rk72brjg2)

## WSL 內 STM32 Debug CDC USB 流程

1. usbipd wsl attach --busid 1-1 --auto-attach (Win11)
2. 插上 USB，讓 STM32 進入 Debug Mode，斷點不能出現(要記得跳掉 HAL_Init)
3. sudo chmod 666 /dev/ttyACM0 (WSL)
4. run your porgram
