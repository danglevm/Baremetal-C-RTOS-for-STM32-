# STM32 Baremetal RTOS 
![image](https://github.com/danglevm/2DJavaGame/assets/84720339/97dcd99c-9733-4fa2-a236-aee0da234665)

## 📄 Overview
Basic C-based kernel running on baremetal Cortex-M4 MCU with Earliest Deadline First pre-emptive scheduling and dynamic memory allocator using first-fit allocation scheme with heap memory.

![image](https://github.com/danglevm/2DJavaGame/assets/84720339/54814ca5-0f88-41e8-bff0-80267fe03b77)

## 💾 Source Files
**asm.s:** assembly code for handler functions (PendSV) and register operations

**k_mem.c:** Dynamic memory allocation code with first-fit allocation

**main.c:** Test cases and driver code

**system_stm32f4xx.c:** handler functions and system calls

![image](https://github.com/danglevm/2DJavaGame/assets/84720339/54814ca5-0f88-41e8-bff0-80267fe03b77)
## 📥 Installation
Download directly from repo or fork and clone:
1. Click Fork from top-right corner then clone the repo in top directory
```
git clone https://github.com/YOUR-USERNAME/baremetal-STM32-RTOS.git
```
2. Replace `Core/` folder from top project STM32 directory with newly forked `Core/` or import New Project with cloned C source files 
3. Build and download the binary onto the running STM32

![image](https://github.com/danglevm/2DJavaGame/assets/84720339/54814ca5-0f88-41e8-bff0-80267fe03b77)

## 📷 Screenshots and Video 
<img width="1241" height="966" alt="image" src="https://github.com/user-attachments/assets/7b3251da-0433-460e-91d2-2a86581bb40c" />




