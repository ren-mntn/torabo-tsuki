SRC += trackball.c eeconfig_kb.c
# MCU name
MCU = cortex-m4
MCU_LDSCRIPT = nrf52840_bmp
PLATFORM_KEY = bmp
BOOTLOADER = custom
EEPROM_DRIVER  = custom
POINTING_DEVICE_DRIVER = custom
CUSTOM_MATRIX = yes
BOOTMAGIC_ENABLE = yes

MOUSE_SHARED_EP = no
MOUSEKEY_ENABLE = yes       # Mouse keys
EXTRAKEY_ENABLE = yes       # Audio control and System control
CONSOLE_ENABLE = yes        # Console for debug

ENCODER_ENABLE = yes
ENCODER_MAP_ENABLE = yes