#ifndef LCOM_NG_I8042_H
#define LCOM_NG_I8042_H

#include <lcom/lcom.h>

#define KBC_IRQ 1u
#define MOUSE_IRQ 12u

#define KBC_OUT_BUF 0x60u
#define KBC_IN_BUF 0x60u
#define KBC_ST_REG 0x64u
#define KBC_CMD_REG 0x64u

#define KBC_ST_OBF BIT(0)
#define KBC_ST_IBF BIT(1)
#define KBC_ST_AUX BIT(5)
#define KBC_ST_TIMEOUT BIT(6)
#define KBC_ST_PARITY BIT(7)
#define KBC_ST_ERR (KBC_ST_PARITY | KBC_ST_TIMEOUT)

#define KBC_CMD_READ_CB 0x20u
#define KBC_CMD_WRITE_CB 0x60u
#define KBC_CMD_DISABLE 0xADu
#define KBC_CMD_ENABLE 0xAEu
#define KBC_CMD_WRITE_MOUSE 0xD4u

#define KBC_CB_INT BIT(0)
#define KBC_CB_INT2 BIT(1)
#define KBC_CB_DIS BIT(4)
#define KBC_CB_DIS2 BIT(5)

#define ESC_BREAK 0x81u
#define KBD_TWO_BYTE_PFX 0xE0u
#define KBD_BREAK_BIT BIT(7)

#define MOUSE_CMD_DISABLE_DR 0xF5u
#define MOUSE_CMD_ENABLE_DR 0xF4u
#define MOUSE_CMD_SET_STREAM 0xEAu
#define MOUSE_CMD_SET_REMOTE 0xF0u
#define MOUSE_CMD_READ_DATA 0xEBu
#define MOUSE_CMD_SET_DEFAULT 0xF6u
#define MOUSE_CMD_RESET 0xFFu

#define MOUSE_ACK 0xFAu
#define MOUSE_NACK 0xFEu
#define MOUSE_ERR 0xFCu
#define MOUSE_SYNC_BIT BIT(3)

#endif
