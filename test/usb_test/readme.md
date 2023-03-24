# WSL 安裝
- WSL 中需要安裝 https://learn.microsoft.com/en-us/windows/wsl/connect-usb 的 usbipd-win 才能使用 USB 設備
- 安裝完後重開機
- ```usbipd wsl list``` in Powershell
- ```usbipd wsl attach --busid 1-1 --auto-attach``` in Powershell if you install gsudo
- ```lsusb``` in WSL terminal or ``` ll /sys/class/tty/ttyACM*  ```
- 如果要用 Serial open 的話，STM 應該會出現於 ttyACM?

## 如果遇到 open permission denied 問題
1. 確認使用者是否在 dialout 群組 (```groups ${USER}```)
  - [serial permission denied](https://askubuntu.com/questions/210177/serial-port-terminal-cannot-open-dev-ttys0-permission-denied)
2. 真的不行使出 sudo chmod 666 /dev/ttyACM0

## WSL 內 STM32 Debug CDC USB 流程
1. usbipd wsl attach --busid 1-1 --auto-attach (Win11)
2. 插上 USB，讓 STM32 進入 Debug Mode，斷點不能出現(要記得跳掉 HAL_Init)
3. sudo chmod 666 /dev/ttyACM0 (WSL)
4. run your porgram