# Sound-Voice-Activated-Camera

A collaborative project to build a camera that takes a photo when a loud sound (like a clap) is detected and saves it to an SD card.

![All Components Cleaned](images/AllComponentsCleaned_bb_png.png)

## Requirements

- **Arduino IDE**: Needed to upload code to both Arduino Uno and ESP32-CAM boards.
- **Arduino Uno**: Used for the Arduino part of the project (e.g., sound detection, sensor handling).
- **ESP32-CAM**: Used for capturing photos and saving them to the SD card. Make sure you have the **ESP32 Arduino core version 3.3.5** installed via Board Manager.
- **Libraries**: The ESP32-CAM code uses the following libraries included with the ESP32 Arduino core:
  - `esp_camera.h`
  - `FS.h`
  - `SD_MMC.h`
  - `Preferences.h`
  - `soc/soc.h`
  - `soc/rtc_cntl_reg.h`

## Setup & Resources

- **Fritzing software installation**: https://www.youtube.com/watch?v=Q2UuuFEXK8fI&t=111s
- **Downloading and using special parts in Fritzing**: https://www.youtube.com/watch?v=zFE9JE5JRXM

## References

1. ESP32-CAM example project on GitHub: https://github.com/theinfoflux/ESP32-CAM-take-photo-and-save-to-SD-card/tree/main  
2. Project-related files on Google Drive: https://drive.google.com/drive/folders/1-2DN0V-q17inVsm0gXS6E6vmfvTE58fD
