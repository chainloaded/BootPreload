# UEFI BootPreloader

This project is a small UEFI bootloader designed as a simple learning resource for getting into UEFI development. It also serves as a flexible template for future, more advanced projects, such as a planned boot configuration protection application utilizing TPM/FIDO2 and DRTM.

## Features

- **Setup Mode:** Includes a console-based interface with commands to manage boot entries.

- **Chainloading:** Capable of loading and executing other UEFI applications directly from the EFI System Partition (ESP).

- **Secure Boot Awareness:** Boot entries can be individually configured to require Secure Boot; the loader checks the firmware's `SecureBoot` global variable before execution and denies access if it is disabled.

- **NVRAM Configuration Storage:** Persists the boot entry database and auto-load settings using custom, non-volatile UEFI variables (`BootEntryDatabase` and `AppConfiguration`).

- **Auto-Boot:** Allows for configuring primary boot entries and automatic boot paths.


## Architecture

The bootloader is written in C and organized into modular components:

- **Entry Point (`BpMain.c`):** Initializes the application context and routes execution to either the automatic loader or the interactive setup mode depending on configuration and user input.

- **Configuration Manager (`BpConfiguration.c`):** Handles reads and writes of the application's boot database and primary state to UEFI NVRAM.

- **Loader (`BpLoader.c`):** Constructs proper EFI device paths, loads the target EFI image into memory, and transfers execution.

- **Setup UI (`BpSetupMode.c`):** Provides the interactive command-line interface, keyboard input polling, string tokenization, and command dispatching.

- **Utilities (`BpMemory.c`, `BpString.c`):** Implements lightweight memory helpers and basic wide-character string manipulation helpers.