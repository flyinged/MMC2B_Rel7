#include "ff.h"
#include "integer.h"
#include "mss_uart.h"
#include "mss_rtc.h"

unsigned long year_in_reference;
unsigned long month_in_reference;
unsigned long days_in_reference;
unsigned long hours_in_reference;
unsigned long mins_in_reference;
unsigned long secs_in_reference;


int xatoi (			/* 0:Failed, 1:Successful */
	char **str,		/* Pointer to pointer to the string */
	long *res		/* Pointer to the valiable to store the value */
)
{
	unsigned long val;
	unsigned char c, r, s = 0;


	*res = 0;

	while ((c = **str) == ' ') (*str)++;	/* Skip leading spaces */

	if (c == '-') {		/* negative? */
		s = 1;
		c = *(++(*str));
	}

	if (c == '0') {
		c = *(++(*str));
		switch (c) {
		case 'x':		/* hexdecimal */
			r = 16; c = *(++(*str));
			break;
		case 'b':		/* binary */
			r = 2; c = *(++(*str));
			break;
		default:
			if (c <= ' ') return 1;	/* single zero */
			if (c < '0' || c > '9') return 0;	/* invalid char */
			r = 8;		/* octal */
		}
	} else {
		if (c < '0' || c > '9') return 0;	/* EOL or invalid char */
		r = 10;			/* decimal */
	}

	val = 0;
	while (c > ' ') {
		if (c >= 'a') c -= 0x20;
		c -= '0';
		if (c >= 17) {
			c -= 7;
			if (c <= 9) return 0;	/* invalid char */
		}
		if (c >= r) return 0;		/* invalid char for current radix */
		val = val * r + c;
		c = *(++(*str));
	}
	if (s) val = 0 - val;			/* apply sign if needed */

	*res = val;
	return 1;
}


/*----------------------------------------------*/
/* Get a line from the input                    */
/*----------------------------------------------*/

int get_line (		/* 0:End of stream, 1:A line arrived */
	char* buff,		/* Pointer to the buffer */
	int len			/* Buffer length */
)
{
	int rv, i, rx_size, size = 0;
    uint8_t rx_buff[1]={0};
    
    rv = i = 0;
    //	for (;;) {
    //		c = xfunc_in();				/* Get a char from the incoming stream */
    while(*rx_buff!= '\n' && *rx_buff!= '\r') //ML84 added \n (PuTTY does not send \n on enter key)
    {
        rx_size = MSS_UART_get_rx ( &g_mss_uart0, rx_buff, sizeof(rx_buff) );

        if( rx_size > 0 )
        {
            buff[size] = *rx_buff;
            MSS_UART_polled_tx(&g_mss_uart0, (uint8_t*)(buff+size), 1); //ML84: display while typing
            size++;
        }
    }
    MSS_UART_polled_tx(&g_mss_uart0, (uint8_t*)"\n\r", 2); //ML84

#if 0
		while(! MSS_UART_get_rx ( &g_mss_uart0, &c, 1 ))
			;
		if (!c) return 0;			/* End of stream? */
		if (c == '\r') 
			break;		/* End of line? */
		if (c == '\b' && i) {		/* Back space? */
			i--;
			xputc(c);
			continue;
		}
		if (c >= ' ' && i < len - 1) {	/* Visible chars */
			buff[i++] = c;
//			xputc(c);
		}
	}
#endif	
	buff[size] = 0;	/* Terminate with zero */
//	xputc('\n');
	return 1;
}


/*---------------------------------------------------------*/
/* User Provided Timer Function for FatFs module           */
/*---------------------------------------------------------*/
/* This is a real time clock service to be called from     */
/* FatFs module. Any valid time must be returned even if   */
/* the system does not support a real time clock.          */
int isLeapYear(int year)
{
	if(year%400 ==0 || (year%100 != 0 && year%4 == 0))
	{
	    return 1;
	}
	else
	{
	    return 0;
    }
}

int set_fattime(char *Line)
{
    unsigned int num_of_days_in_non_leap_year[]={31,28,31,30,31,30,31,31,30,31,30,31};
    unsigned int num_of_days_in_leap_year[]={31,29,31,30,31,30,31,31,30,31,30,31};
    unsigned long tm_sec = 0, tm_min = 0, tm_hour = 0, tm_yday = 0, tm_year = 0;
    unsigned long month;
    unsigned long noOfLeapYears;
    long p1;
    char * ptr;

    //char Line[80];

    int isLeapFlag = 0, ii;
    //uint64_t rtcCount = 0;
    unsigned long time_in_sec =0;
#if 0
    MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "Enter the RTC time in: <M D YYYY Hr Min > Format\n\r");

    get_line(Line, sizeof(Line));
#endif
    ptr = Line;

    if (xatoi(&ptr, &p1)) {

        month = p1;
        if( xatoi(&ptr, &p1))
            tm_yday = p1;
        else
        {
            MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) " Please enter the correct time for RTC \n");
            MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "Enter the RTC time in: <M D YYYY Hr Min > Format\n");
            get_line(Line, sizeof(Line));
            set_fattime(Line);
        }

        if( xatoi(&ptr, &p1))
            tm_year = p1;
        else
        {
            MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) " Please enter the correct time for RTC \n");
            MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "Enter the RTC time in: <M D YYYY Hr Min > Format\n");
            get_line(Line, sizeof(Line));
            set_fattime(Line);
        }

        if( (month >=1)&&(month<=12) &&
                ((tm_yday>= 1) && (tm_yday <=31)) &&
                ((tm_year>= 1980) && (tm_year<=2080)))
        {


            isLeapFlag = isLeapYear(tm_year);
            tm_year -= 1980;
            //rtcYear = p1 - 1900;
            noOfLeapYears = tm_year/4;

            //xatoi(&ptr, &p1);
            //month = p1;

            for( ii = 0; ii < (month -1); ii++ )
            {
                if( isLeapFlag )
                {
                    tm_yday += num_of_days_in_leap_year[ii];
                }
                else
                {
                    tm_yday += num_of_days_in_non_leap_year[ii];
                }
            }

            //rtcMon = p1;
            //xatoi(&ptr, &p1);
            //tm_yday += p1;
            //Adding Leap Year Days
            tm_yday += noOfLeapYears;
            //Totoal No of Dyas
            tm_yday += tm_year * 365;

            // rtcMday = p1;
            if(xatoi(&ptr, &p1))
                tm_hour = p1;
            else
            {
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) " Please enter the correct time for RTC \n");
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "Enter the RTC time in: <M D YYYY Hr Min > Format\n");
                get_line(Line, sizeof(Line));
                set_fattime(Line);

            }

            //rtcHour = p1;
            if( xatoi(&ptr, &p1))
                tm_min = p1;
            else
            {
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) " Please enter the correct time for RTC \n");
                MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "Enter the RTC time in: <M D YYYY Hr Min > Format\n");
                get_line(Line, sizeof(Line));
                set_fattime(Line);

            }

            tm_sec = 0;

            time_in_sec = tm_sec + tm_min*60 + tm_hour*3600 + tm_yday*86400;
            MSS_RTC_set_seconds_count(time_in_sec);
        }
        else
        {
            MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) " Please enter the correct time for RTC \n");
            MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "Enter the RTC time in: <M D YYYY Hr Min > Format\n");
            get_line(Line, sizeof(Line));
            set_fattime(Line);

        }

    }
    else
    {
        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) " Please enter the correct time for RTC \n");
        MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "Enter the RTC time in: <M D YYYY Hr Min > Format\n");
        get_line(Line, sizeof(Line));
        set_fattime(Line);

    }
    //MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "RTC Secs SET  %d\n", time_in_sec);
    return 0; //ML84
}

uint32_t get_fattime ()
{
	uint32_t tmr, ii;
    unsigned int num_of_days_in_year[]={31,28,31,30,31,30,31,31,30,31,30,31};
//    unsigned int num_of_days_in_leap_year[]={31,29,31,30,31,30,31,31,30,31,30,31};
	uint32_t  noOfYears, noOfMonths, noOfDays,noOfHours, noOfMins, noOfSecs, residue;


	//set_imask_ccr(1);
	//tmr =	(uint32_t)MSS_RTC_get_rtc_count();
	tmr =	(uint32_t)MSS_RTC_get_seconds_count();
	

	//MSS_UART_polled_tx_string(&g_mss_uart0, (uint8_t*) "RTC Secs Get %d\n", tmr);
	
    /* Calculating the Number of Days from the Seconds Value */
    noOfDays = (tmr / 86400);
    residue = (tmr % 86400);

	    /* Calculating the Number of Hours */
	    noOfHours = (residue / 3600);
	    noOfSecs = (residue % 3600);

	    /* Calculating the Number of Minutes */
	    noOfMins = (noOfSecs / 60);
	    noOfSecs = (noOfSecs % 60);


	noOfYears = noOfDays /365 ;
	noOfDays = ((noOfDays % 365) - (noOfYears / 4));
	
	if ((noOfYears % 4) == 0)
	{
		num_of_days_in_year[1] += 1;
	}
	/*Months */
	for (ii= 0; ii < 12; ii++)
	{
        if (noOfDays > num_of_days_in_year[ii] )
        {
        	noOfDays -= num_of_days_in_year[ii];
        }
        else
        {

        	break;
        }
        
	}
	noOfMonths = ++ii;
	tmr =    ( ((noOfYears) << 25 ) |
	           ((noOfMonths) << 21) |
	           ((noOfDays) << 16) |
	           ((noOfHours) << 11) |
	           ((noOfMins) << 5) |
	           ((noOfSecs) >> 1));

	return tmr;
}

