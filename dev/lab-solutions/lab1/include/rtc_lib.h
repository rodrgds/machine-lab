#ifndef LAB1_RTC_LIB_H
#define LAB1_RTC_LIB_H

#include <lcom/rtc.h>

int rtc_read_date(lcom_rtc_date_t *date);
int rtc_read_time(lcom_rtc_time_t *time);

#endif
