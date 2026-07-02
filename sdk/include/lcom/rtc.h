#ifndef MACHINE_LAB_RTC_H
#define MACHINE_LAB_RTC_H

#include <lcom/lcom.h>

#define RTC_ADDR_REG 0x70u
#define RTC_DATA_REG 0x71u

#define RTC_REG_SECONDS 0x00u
#define RTC_REG_MINUTES 0x02u
#define RTC_REG_HOURS 0x04u
#define RTC_REG_DAY 0x07u
#define RTC_REG_MONTH 0x08u
#define RTC_REG_YEAR 0x09u
#define RTC_REG_A 0x0Au
#define RTC_REG_B 0x0Bu
#define RTC_REG_C 0x0Cu
#define RTC_REG_D 0x0Du

#define RTC_UIP BIT(7)
#define RTC_DM BIT(2)
#define RTC_24H BIT(1)

typedef struct {
  uint8_t day;
  uint8_t month;
  uint8_t year;
} lcom_rtc_date_t;

typedef struct {
  uint8_t second;
  uint8_t minute;
  uint8_t hour;
} lcom_rtc_time_t;

#endif
