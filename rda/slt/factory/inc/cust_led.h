
#ifndef FTM_CUST_LED_H
#define FTM_CUST_LED_H
/*
#ifdef FTM_LED_SUPPORTED
#error "invalid defined micro FTM_LED_SUPPORTED"
#endif
*/
//#define CUST_LED_HAVE_RED
//#define CUST_LED_HAVE_GREEN
//#define CUST_LED_HAVE_BLUE
/*
#if ((defined _TGT_AP_LED_BLUE) && (defined _TGT_AP_LED_BLUE_KB))
*/
#define CUST_LED_HAVE_BUTTON_BACKLIGHT
/*
#undef FTM_LED_SUPPORTED
#define FTM_LED_SUPPORTED
#endif
*/
#endif /* FTM_CUST_LED_H */
