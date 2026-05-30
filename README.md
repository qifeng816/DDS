<div align="center">

# 🎛️ DDS 综合信号发生系统
# <sub><sup>DDS Comprehensive Signal Generator</sup></sub>

[![FPGA](https://img.shields.io/badge/FPGA-Cyclone%20IV%20EP4CE6F17C8-0071C5?logo=intel)](https://www.intel.com/)
[![MCU](https://img.shields.io/badge/MCU-MSPM0G3519-CC0000?logo=texas-instruments)](https://www.ti.com/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Quartus](https://img.shields.io/badge/IDE-Quartus%20Prime%2018.0-00A3E0)](https://www.intel.com/content/www/us/en/products/details/fpga/development-tools/quartus-prime.html)
[![Keil](https://img.shields.io/badge/IDE-Keil%20uVision-0091BD)](https://www.keil.com/)

> **基于 Cyclone IV FPGA + MSPM0G3519 MCU 的直接数字频率合成（DDS）平台**
>
> 支持正弦波、方波、AM、FM、ASK、PSK、扫频及李萨如图测试模式

[📖 快速开始](#-快速开始--quick-start) • [🔧 硬件架构](#-系统架构--system-architecture) • [📡 通信协议](#-通信协议--uart-protocol) • [📋 更新日志](#-更新日志--changelog)

</div>

---

## 📑 目录 | Table of Contents

- [项目概览 | Project Overview](#-项目概览--project-overview)
- [系统架构 | System Architecture](#-系统架构--system-architecture)
- [目录结构 | Repository Structure](#-目录结构--repository-structure)
- [核心原理 | Core Principles](#-核心原理--core-principles)
  - [DDS 基本原理](#31-dds-基本原理--dds-fundamentals)
  - [调制原理](#32-调制原理--modulation-principles)
  - [通信协议](#33-通信协议--uart-protocol)
- [功能特性 | Features](#-功能特性--features)
- [软硬件环境 | Requirements](#-软硬件环境--requirements)
- [快速开始 | Quick Start](#-快速开始--quick-start)
  - [FPGA 端](#71-fpga-端)
  - [MCU 端](#72-mcu-端)
  - [联调流程](#73-联调流程)
- [更新日志 | Changelog](#-更新日志--changelog)
- [优化建议 | Future Work](#-优化建议--future-work)
- [许可说明 | License](#-许可说明--license)

---

## 🌟 项目概览 | Project Overview

### 中文

本项目是一个完整的 **DDS（直接数字频率合成）信号发生器**，采用 **FPGA + MCU 双核架构**：

| 核心 | 职责 | 关键文件 |
|------|------|----------|
| **FPGA** (Cyclone IV) | 波形实时合成、调制运算、DAC 输出、数码管显示 | `DDS_FPGA/DDS_simple.v` |
| **MCU** (MSPM0G3519) | 人机交互、OLED UI、键盘扫描、参数下发 | `DDS_MCU/User/main.c` |

MCU 通过 **4×4 矩阵键盘** 和 **OLED 显示屏** 完成人机交互，经 **UART (115200bps)** 将模式与参数发送给 FPGA，由 FPGA 实时合成 14-bit 波形数据并驱动高速 DAC 输出。

### English

This project is a complete **DDS (Direct Digital Synthesis) signal generator** based on an **FPGA + MCU dual-core architecture**:

| Core | Responsibility | Key File |
|------|---------------|----------|
| **FPGA** (Cyclone IV) | Real-time waveform synthesis, modulation, DAC output, 7-seg display | `DDS_FPGA/DDS_simple.v` |
| **MCU** (MSPM0G3519) | HMI, OLED UI, keypad scanning, parameter dispatch | `DDS_MCU/User/main.c` |

The MCU handles user interaction via a **4×4 matrix keypad** and **OLED display**, then transmits mode/parameters to the FPGA through **UART (115200bps)**. The FPGA synthesizes 14-bit waveform data in real time and drives a high-speed DAC.

---

## 🏗️ 系统架构 | System Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              DDS 综合信号发生系统                              │
├─────────────────────────────┬───────────────────────────────────────────────┤
│        MCU 控制层            │              FPGA 信号合成层                    │
│   (MSPM0G3519 @ 80MHz)      │         (Cyclone IV @ 50MHz)                  │
│                             │                                               │
│  ┌─────────────┐            │    ┌─────────────┐      ┌─────────────────┐   │
│  │  4×4 键盘    │──┐         │    │  UART 接收   │◄────│  MCU UART TX    │   │
│  └─────────────┘  │         │    │  (MCU_uart)  │      │  (115200bps)    │   │
│                   │         │    └──────┬──────┘      └─────────────────┘   │
│  ┌─────────────┐  │         │           │                                   │
│  │  OLED 显示   │◄─┼─────────┤    ┌──────▼──────┐      ┌─────────────────┐   │
│  │ (128×64 SPI) │  │         │    │  模式解析器   │─────►│  相位累加器      │   │
│  └─────────────┘  │         │    │  (A/B/C/D/E) │      │  (Phase ACC)     │   │
│                   │         │    └──────┬──────┘      └────────┬────────┘   │
│  ┌─────────────┐  │         │           │                      │            │
│  │  W25Q64      │  │         │    ┌──────▼──────┐      ┌────────▼────────┐   │
│  │  (SPI Flash) │◄─┘         │    │  参数寄存器   │─────►│  正弦 ROM       │   │
│  └─────────────┘            │    │ (Fc/ma/Dev)  │      │ (sine_14bit_256)│   │
│                             │    └─────────────┘      └────────┬────────┘   │
└─────────────────────────────┘           │                      │            │
                                          │               ┌──────▼──────┐     │
                                          │               │   调制运算    │     │
                                          │               │  (AM/FM/ASK) │     │
                                          │               └──────┬──────┘     │
                                          │                      │            │
                                          │               ┌──────▼──────┐     │
                                          └──────────────►│  14-bit DAC │     │
                                                          │   输出接口   │─────┼──► RF OUT
                                                          └─────────────┘     │
                                                          ┌─────────────────┐ │
                                                          │  6位数码管显示   │─┘
                                                          │   (seg_display) │
                                                          └─────────────────┘
                                                          ┌─────────────────┐
                                                          │  PWM 输出        │
                                                          │  (李萨如图模式)  │──────► CH2
                                                          └─────────────────┘
```

---

## 📁 目录结构 | Repository Structure

```text
DDS/
│
├── 📂 DDS_FPGA/                    # FPGA 工程 (Quartus Prime)
│   ├── DDS_simple.qpf/.qsf        # 工程文件与引脚约束
│   ├── DDS_simple.v               # 🧠 顶层模块 (DDS + 调制 + 显示)
│   ├── MCU_uart.v                 # 📡 UART 接收与协议解析
│   ├── dds_freq_ctrlword.v        # 🎚️ 频率控制字计算
│   ├── seg_display.v              # 🔢 6 位数码管动态扫描
│   ├── dac904_pll.qip/.v          # ⏱️ DAC 时钟 PLL (165MHz)
│   ├── sine_14bit_256.mif/.v      # 📊 14-bit 正弦查找表 (256点)
│   ├── sintopwm.v                 # 📐 正弦转 PWM (李萨如第二通道)
│   └── db/                        # 编译数据库
│
├── 📂 DDS_MCU/                     # MCU 工程 (Keil uVision)
│   ├── User/
│   │   ├── main.c                 # 🎮 主流程与交互状态机
│   │   ├── config.syscfg          # TI SysConfig 配置源
│   │   ├── ti_msp_dl_config.*     # DriverLib 自动生成配置
│   │   └── delay.c/h              # 延时函数
│   ├── BSP/
│   │   ├── Keyboard.c/h           # ⌨️ 4×4 矩阵键盘扫描
│   │   ├── oled_spi_V0.2.c/h      # 🖥️ OLED SPI 驱动 (128×64)
│   │   ├── w25q64_spi.c/h         # 💾 W25Q64 SPI Flash 驱动
│   │   └── bsp.h                  # BSP 头文件聚合
│   ├── Project/
│   │   └── DDS.uvprojx            # Keil 工程文件
│   └── Source/third_party/        # CMSIS/DSP 等第三方库
│
├── LICENSE                         # 许可证文件
└── README.md                       # 📖 本文件
```

---

## 🔬 核心原理 | Core Principles

### 3.1 DDS 基本原理 | DDS Fundamentals

FPGA 内部采用经典 **相位累加器 + 正弦 ROM** 架构实现 DDS：

$$
f_{out} = \frac{f_{clk} \times M}{2^N}
$$

| 参数 | 说明 | 当前取值 |
|------|------|----------|
| $f_{clk}$ | 系统时钟频率 | 50 MHz |
| $N$ | 相位累加器位宽 | 32 bit |
| $M$ | 频率控制字 (FCW) | $freq_{target} \times 86$ |
| $f_{out}$ | 输出频率 | 0.1 Hz ~ 10 MHz |

相位累加器高 8 位作为 ROM 地址，查表输出 14-bit 波形数据送 DAC。ROM 采用 **256 点、14-bit 量化** 的正弦查找表，兼顾资源占用与波形纯度。

### 3.2 调制原理 | Modulation Principles

| 模式 | 字符 | 原理简述 | 可调参数 |
|------|------|----------|----------|
| **正弦波** | `A` | 基础 DDS 输出 | 频率 (Hz/kHz/MHz) |
| **方波** | `E` | 比较器 + 占空比控制 | 频率、占空比 (%) |
| **AM** | `B` | 载波 × 包络 (调制波) | 载频 $F_c$、调制度 $m_a$ |
| **FM** | `C` | 动态频率控制字扰动 | 载频 $F_c$、频偏 $\Delta f$ |
| **ASK** | 硬件开关 | 10kbps 基带键控 100kHz 载波 | — |
| **PSK** | 硬件开关 | 10kbps 基带相移键控 | — |
| **扫频** | `D` | 线性/对数频率扫描 | 起始/终止频率、步进、驻留时间 |
| **李萨如** | 硬件开关 | DAC 1kHz + PWM 2kHz 双通道 | — |

**AM 调制细节：**
```verilog
// 包络生成: envelope = 1 + ma * mod_wave
envelope = (1 <<< 14) + (ma * mod_data >>> 8);
am_out   = (rom_data * envelope) >>> 14;
```

**FM 调制细节：**
```verilog
// 动态相位步进: phase_step = fc_step + dev_step * mod_wave
phase_step = fm_fc_step + ((fm_dev_step * mod_data) >>> 14);
```

### 3.3 通信协议 | UART Protocol

```
波特率: 115200 bps
数据位: 8 bit
停止位: 1 bit
校验:   None
```

**帧格式：**

```
┌─────────┬──────────────────────┬──────────┐
│  模式字符  │      ASCII 数值        │  换行符   │
│ (1 byte) │   (变长, 十进制)        │  \\n (0x0A)  │
├─────────┼──────────────────────┼──────────┤
│  'A'/'B'  │   freq / fc / ma / dev  │   \\n    │
│  /'C'/'D' │   (根据模式上下文解析)   │         │
│  /'E'     │                        │         │
└─────────┴──────────────────────┴──────────┘
```

**模式定义：**

| 字符 | 模式 | 参数顺序 | 默认值 |
|------|------|----------|--------|
| `A` | 正弦波 | `A` + `频率(Hz)` | 1 kHz |
| `B` | AM 调制 | `B` + `Fc` + `ma` | Fc=1.0MHz, ma=10% |
| `C` | FM 调制 | `C` + `Fc` + `Dev` | Fc=100kHz, Dev=5kHz |
| `D` | 扫频 | `D` + `StartFreq` + `StopFreq` + `Step` + `Dwell` | — |
| `E` | 方波 | `E` + `Freq` + `Duty` | Freq=1kHz, Duty=50% |

> **注意：** 模式字符会重置 FPGA 内部的参数索引计数器 `data_idx`，确保多参数顺序写入不错位。

---

## ✨ 功能特性 | Features

### 🎵 波形生成
- [x] **正弦波 (Sine)** — 0.1 Hz ~ 10 MHz，支持 Hz / kHz / MHz 单位切换
- [x] **方波 (Square)** — 可编程占空比 (1% ~ 99%)，支持高频输出
- [x] **扫频 (Sweep)** — 线性/对数扫频模式，可设起始/终止频率、步进、驻留时间

### 📻 调制模式
- [x] **AM (幅度调制)** — 载波 + 调制波相乘，调制度 0% ~ 100%
- [x] **FM (频率调制)** — 动态相位步进扰动，频偏可切换 (5kHz / 10kHz)
- [x] **ASK (幅移键控)** — 10kbps 基带序列键控 100kHz 载波
- [x] **PSK (相移键控)** — 10kbps 基带序列相移键控

### 🔬 测试功能
- [x] **李萨如图模式** — DAC 输出 1kHz 正弦，`pwm_out` 输出 2kHz PWM，双通道示波器观测
- [x] **基带观测** — `baseband_out` 引脚输出原始基带比特流，便于联调分析

### 🖥️ 人机交互
- [x] **4×4 矩阵键盘** — 数字输入、模式切换、参数微调 (`*`/`#` 步进)
- [x] **OLED 菜单** — 128×64 分辨率，中英文状态显示
- [x] **6 位数码管** — FPGA 侧实时显示频率、调制度、频偏信息

### 💾 外设支持
- [x] **W25Q64 SPI Flash** — 波形数据/配置参数存储框架
- [x] **高速 DAC 接口** — 14-bit 并行输出，165MHz 时钟 PLL 驱动

---

## 🛠️ 软硬件环境 | Requirements

### 开发工具链

| 组件 | 版本/型号 | 用途 |
|------|-----------|------|
| **Quartus Prime** | 18.0 | FPGA 综合、布局布线、下载 |
| **Keil uVision** | 5.38+ | MCU 编译、调试、下载 |
| **TI MSPM0 SDK** | 2.08.00.03 | DriverLib 与 SysConfig |
| **J-Link** | V12+ | MCU 调试与程序烧录 |

### 目标硬件

| 器件 | 型号 | 功能 |
|------|------|------|
| **FPGA** | Intel Cyclone IV E `EP4CE6F17C8` | 波形合成与调制运算 |
| **MCU** | Texas Instruments MSPM0G3519 | 人机交互与参数控制 |
| **DAC** | AD904 / 等效高速 DAC | 14-bit 模拟波形输出 |
| **OLED** | 0.96" 128×64 SPI | 菜单与状态显示 |
| **键盘** | 4×4 矩阵键盘 | 参数输入与模式切换 |

---

## 🚀 快速开始 | Quick Start

### 7.1 FPGA 端

```bash
# 1. 进入 FPGA 工程目录
cd DDS_FPGA/

# 2. 用 Quartus Prime 打开工程
quartus DDS_simple.qpf

# 3. 检查引脚约束 (DDS_simple.qsf) 是否与硬件一致
#    重点核对: 时钟、UART RX、DAC 数据/时钟、数码管、拨码开关

# 4. 全编译 (Ctrl+L) 并下载到开发板 (Ctrl+J)
```

**拨码开关配置：**

| SW_ASK | SW_PSK | SW_LISS | 模式 |
|--------|--------|---------|------|
| 0 | 0 | 0 | 正常模式 (由 MCU 控制 A/B/C/D/E) |
| 1 | 0 | 0 | ASK 输出 |
| 0 | 1 | 0 | PSK 输出 |
| 0 | 0 | 1 | 李萨如图测试 (DAC 1kHz + PWM 2kHz) |

> ⚠️ ASK/PSK/李萨如模式为硬件直通，与 MCU 串口模式控制并行存在，注意避免状态冲突。

### 7.2 MCU 端

```bash
# 1. 进入 MCU 工程目录
cd DDS_MCU/Project/

# 2. 用 Keil 打开工程
UV4.exe DDS.uvprojx

# 3. 确认 SysConfig 预构建步骤已启用
#    (Options for Target -> User -> Run Before Build: syscfg.bat)

# 4. 编译 (F7) 并下载到 MSPM0G3519 (Ctrl+F5)
```

**MCU 键盘操作指南：**

```
┌─────┬─────┬─────┬─────┐
│  1   │  2   │  3   │  A   │  ← A: Sine 模式
├─────┼─────┼─────┼─────┤
│  4   │  5   │  6   │  B   │  ← B: AM 模式
├─────┼─────┼─────┼─────┤
│  7   │  8   │  9   │  C   │  ← C: FM 模式
├─────┼─────┼─────┼─────┤
│  *   │  0   │  #   │  D   │  ← D: Sweep 模式, *: 步进减, #: 步进加
└─────┴─────┴─────┴─────┘
```

### 7.3 联调流程

1. **先下载 FPGA 固件**，确认 `locked` 信号为高（PLL 锁定）
2. **再下载 MCU 固件**，上电后 OLED 显示主菜单
3. **接线检查**：MCU TX ↔ FPGA `uart_rx`，**必须共地**
4. 在 MCU 上选择模式并输入参数，观察：
   - 🎚️ FPGA DAC 输出波形（示波器）
   - 🔢 FPGA 数码管显示（频率/调制度）
   - 🖥️ MCU OLED 反馈（当前模式与参数）

---

## 📝 更新日志 | Changelog

### v2.1 (2025-05-30) — 方波与扫频增强

| 类别 | 变更内容 |
|------|----------|
| ✨ **新增** | 方波输出模式 (`E` 模式)，支持 1% ~ 99% 占空比可调 |
| ✨ **新增** | 扫频模式 (`D` 模式)，支持线性频率扫描 |
| ✨ **新增** | 频率自适应步进：根据当前频率自动调整 `*`/`#` 微调步长 (10Hz/100Hz/1kHz/10kHz/100kHz) |
| 🐛 **修复** | 正弦模式单位切换后显示与发送不同步问题 |
| 🐛 **修复** | AM/FM 参数索引错位问题，改用"模式字符 + 双参数"顺序下发 |
| 🐛 **修复** | 李萨如图模式双通道相位漂移，新增独立相位累加路径 |
| 💡 **优化** | FM 模式下数码管显示 `fm_dev / 1000` (kHz 量级)，便于直接观察频偏 |
| 🛡️ **优化** | 增加多版本备份文件管理机制 (`.bak.YYYYMMDD-特征`) |

### v2.0 (2025-05-23) — AM/FM 调制完善

| 类别 | 变更内容 |
|------|----------|
| ✨ **新增** | AM 调制模式 (载频 + 调制度) |
| ✨ **新增** | FM 调制模式 (载频 + 频偏) |
| ✨ **新增** | ASK/PSK 数字调制 (硬件拨码开关控制) |
| ✨ **新增** | 李萨如图测试模式 (DAC + PWM 双通道) |
| 🐛 **修复** | UART 协议帧解析鲁棒性增强 |
| 💡 **优化** | 频率控制字计算精度提升 |

### v1.0 (初始版本) — 基础 DDS 正弦波

- [x] 基础 DDS 相位累加器 + 正弦 ROM 架构
- [x] MCU UART 参数下发
- [x] 数码管动态扫描显示
- [x] 4×4 键盘 + OLED 基础交互

---

## 🔮 优化建议 | Future Work

| 优先级 | 方向 | 具体措施 |
|--------|------|----------|
| 🔴 **高** | 协议增强 | 增加帧头 (0xAA55)、长度字段、CRC16 校验、ACK 应答机制 |
| 🔴 **高** | 统一模式控制 | 将硬件拨码开关模式 (ASK/PSK/李萨如) 纳入 MCU 菜单统一管理 |
| 🟡 **中** | 参数保护 | MCU 与 FPGA 双侧增加频率/调制度/频偏范围限制与越界提示 |
| 🟡 **中** | 精度优化 | 将 `freq_target * 86` 定点系数升级为 64-bit 高精度除法，消除频率误差 |
| 🟡 **中** | 工程清理 | 分离生成文件、编译缓存、备份文件，添加 `.gitignore` 规则 |
| 🟢 **低** | 自动化验证 | 增加 ModelSim testbench：UART 输入序列仿真、AM/FM 频谱幅度自动检查 |
| 🟢 **低** | 文档完善 | 补充硬件接线图、电源拓扑、信号完整性建议、EMC 设计指南 |
| 🟢 **低** | 波形扩展 | 增加三角波、锯齿波、噪声、任意波形 (通过 W25Q64 加载) |

---

## 📄 许可说明 | License

本项目采用 **MIT License** 开源，详见 [LICENSE](LICENSE) 文件。

> 仓库中 `DDS_MCU/Source/third_party/` 目录包含 TI CMSIS/DSP 等第三方库，其版权归原版权所有者所有，使用时请遵循各自许可证条款。

---

<div align="center">

## 🤝 贡献者 | Contributors

<a href="https://github.com/shantu246/DDS/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=shantu246/DDS" alt="Contributors" />
</a>

---

**Made with ❤️ by the DDS Team**

*如果本项目对你有帮助，欢迎 Star ⭐ 支持！*

</div>
