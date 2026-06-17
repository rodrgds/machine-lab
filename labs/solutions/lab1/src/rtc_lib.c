#include "rtc_lib.h"

static uint8_t bcd_to_bin(uint8_t bcd) {
  return (uint8_t)(((bcd >> 4) * 10u) + (bcd & 0x0Fu));
}

static int rtc_read_reg(uint8_t reg, uint8_t *value) {
  if (lcom_port_write8(RTC_ADDR_REG, reg) != LCOM_OK) return LCOM_ERR;
  return lcom_port_read8(RTC_DATA_REG, value);
}

static int rtc_wait_ready(void) {
  for (int tries = 0; tries < 1000; tries++) {
    uint8_t reg_a = 0;
    if (rtc_read_reg(RTC_REG_A, &reg_a) != LCOM_OK) return LCOM_ERR;
    if ((reg_a & RTC_UIP) == 0) return LCOM_OK;
  }
  return LCOM_TIMEOUT;
}

int rtc_read_date(lcom_rtc_date_t *date) {
  if (date == 0 || rtc_wait_ready() != LCOM_OK) return LCOM_ERR;
  uint8_t reg_b = 0;
  if (rtc_read_reg(RTC_REG_B, &reg_b) != LCOM_OK) return LCOM_ERR;
  if (rtc_read_reg(RTC_REG_DAY, &date->day) != LCOM_OK) return LCOM_ERR;
  if (rtc_read_reg(RTC_REG_MONTH, &date->month) != LCOM_OK) return LCOM_ERR;
  if (rtc_read_reg(RTC_REG_YEAR, &date->year) != LCOM_OK) return LCOM_ERR;
  if ((reg_b & RTC_DM) == 0) {
    date->day = bcd_to_bin(date->day);
    date->month = bcd_to_bin(date->month);
    date->year = bcd_to_bin(date->year);
  }
  return LCOM_OK;
}

int rtc_read_time(lcom_rtc_time_t *time) {
  if (time == 0 || rtc_wait_ready() != LCOM_OK) return LCOM_ERR;
  uint8_t reg_b = 0;
  if (rtc_read_reg(RTC_REG_B, &reg_b) != LCOM_OK) return LCOM_ERR;
  if (rtc_read_reg(RTC_REG_SECONDS, &time->second) != LCOM_OK) return LCOM_ERR;
  if (rtc_read_reg(RTC_REG_MINUTES, &time->minute) != LCOM_OK) return LCOM_ERR;
  if (rtc_read_reg(RTC_REG_HOURS, &time->hour) != LCOM_OK) return LCOM_ERR;
  if ((reg_b & RTC_DM) == 0) {
    time->second = bcd_to_bin(time->second);
    time->minute = bcd_to_bin(time->minute);
    time->hour = bcd_to_bin(time->hour);
  }
  return LCOM_OK;
}
