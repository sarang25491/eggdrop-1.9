/* 
 * date.c -- tcl date parsing, as seen in "clock scan"
 *
 * Taken from Tcl 8.4.7, generic/tclDate.c with minor modifications.
 *
 *
 *
 * Copyright (c) 1992-1995 Karl Lehenbauer and Mark Diekhans.
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * Copyright (c) 2004 Eggdev
 *
 * See the file "tcl.license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <eggdrop/eggdrop.h>

#define EPOCH           1970
#define START_OF_TIME   1902
#define END_OF_TIME     2037

/*
 * The offset of tm_year of struct tm returned by localtime, gmtime, etc.
 * I don't know how universal this is; K&R II, the NetBSD manpages, and
 * ../compat/strftime.c all agree that tm_year is the year-1900.  However,
 * some systems may have a different value.  This #define should be the
 * same as in ../compat/strftime.c.
 */
#define TM_YEAR_BASE 1900

#define HOUR(x)         ((int) (60 * x))
#define SECSPERDAY      (24L * 60L * 60L)
#define IsLeapYear(x)   ((x % 4 == 0) && (x % 100 != 0 || x % 400 == 0))

/*
 *  An entry in the lexical lookup table.
 */
typedef struct _TABLE {
    char        *name;
    int         type;
    time_t      value;
} TABLE;


/*
 *  Daylight-savings mode:  on, off, or not yet known.
 */
typedef enum _DSTMODE {
    DSTon, DSToff, DSTmaybe
} DSTMODE;

/*
 *  Meridian:  am, pm, or 24-hour style.
 */
typedef enum _MERIDIAN {
    MERam, MERpm, MER24
} MERIDIAN;


/*
 *  Global variables.  We could get rid of most of these by using a good
 *  union as the yacc stack.  (This routine was originally written before
 *  yacc had the %union construct.)  Maybe someday; right now we only use
 *  the %union very rarely.
 */
static char     *date_DateInput;
static DSTMODE  date_DateDSTmode;
static time_t   date_DateDayOrdinal;
static time_t   date_DateDayNumber;
static time_t   date_DateMonthOrdinal;
static int      date_DateHaveDate;
static int      date_DateHaveDay;
static int      date_DateHaveOrdinalMonth;
static int      date_DateHaveRel;
static int      date_DateHaveTime;
static int      date_DateHaveZone;
static time_t   date_DateTimezone;
static time_t   date_DateDay;
static time_t   date_DateHour;
static time_t   date_DateMinutes;
static time_t   date_DateMonth;
static time_t   date_DateSeconds;
static time_t   date_DateYear;
static MERIDIAN date_DateMeridian;
static time_t   date_DateRelMonth;
static time_t   date_DateRelDay;
static time_t   date_DateRelSeconds;
static time_t  *date_DateRelPointer;

/*
 * Prototypes of internal functions.
 */
static void	date_Dateerror(char *s);
static time_t	ToSeconds(time_t Hours, time_t Minutes,
		    time_t Seconds, MERIDIAN Meridian);
static int	Convert(time_t Month, time_t Day, time_t Year,
		    time_t Hours, time_t Minutes, time_t Seconds,
		    MERIDIAN Meridia, DSTMODE DSTmode, time_t *TimePtr);
static time_t	DSTcorrect(time_t Start, time_t Future);
static time_t	NamedDay(time_t Start, time_t DayOrdinal,
		    time_t DayNumber);
static time_t   NamedMonth(time_t Start, time_t MonthOrdinal,
                    time_t MonthNumber);
static int	RelativeMonth(time_t Start, time_t RelMonth,
		    time_t *TimePtr);
static int	RelativeDay(time_t Start, time_t RelDay,
		    time_t *TimePtr);
static int	LookupWord(char *buff);
static int	date_Datelex(void);

int
date_Dateparse(void);
typedef union
#ifdef __cplusplus
	YYSTYPE
#endif
 {
    time_t              Number;
    enum _MERIDIAN      Meridian;
} YYSTYPE;
# define tAGO 257
# define tDAY 258
# define tDAYZONE 259
# define tID 260
# define tMERIDIAN 261
# define tMINUTE_UNIT 262
# define tMONTH 263
# define tMONTH_UNIT 264
# define tSTARDATE 265
# define tSEC_UNIT 266
# define tSNUMBER 267
# define tUNUMBER 268
# define tZONE 269
# define tEPOCH 270
# define tDST 271
# define tISOBASE 272
# define tDAY_UNIT 273
# define tNEXT 274




#if defined(__cplusplus) || defined(__STDC__)

#if defined(__cplusplus) && defined(__EXTERN_C__)
extern "C" {
#endif
#ifndef date_Dateerror
#if defined(__cplusplus)
	void date_Dateerror(char *);
#endif
#endif
#ifndef date_Datelex
	int date_Datelex(void);
#endif
	int date_Dateparse(void);
#if defined(__cplusplus) && defined(__EXTERN_C__)
}
#endif

#endif

#define date_Dateclearin date_Datechar = -1
#define date_Dateerrok date_Dateerrflag = 0
extern int date_Datechar;
extern int date_Dateerrflag;
YYSTYPE date_Datelval;
YYSTYPE date_Dateval;
typedef int date_Datetabelem;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
#if YYMAXDEPTH > 0
int date_Date_date_Dates[YYMAXDEPTH], *date_Dates = date_Date_date_Dates;
YYSTYPE date_Date_date_Datev[YYMAXDEPTH], *date_Datev = date_Date_date_Datev;
#else	/* user does initial allocation */
int *date_Dates;
YYSTYPE *date_Datev;
#endif
static int date_Datemaxdepth = YYMAXDEPTH;
# define YYERRCODE 256


/*
 * Month and day table.
 */
static TABLE    MonthDayTable[] = {
    { "january",        tMONTH,  1 },
    { "february",       tMONTH,  2 },
    { "march",          tMONTH,  3 },
    { "april",          tMONTH,  4 },
    { "may",            tMONTH,  5 },
    { "june",           tMONTH,  6 },
    { "july",           tMONTH,  7 },
    { "august",         tMONTH,  8 },
    { "september",      tMONTH,  9 },
    { "sept",           tMONTH,  9 },
    { "october",        tMONTH, 10 },
    { "november",       tMONTH, 11 },
    { "december",       tMONTH, 12 },
    { "sunday",         tDAY, 0 },
    { "monday",         tDAY, 1 },
    { "tuesday",        tDAY, 2 },
    { "tues",           tDAY, 2 },
    { "wednesday",      tDAY, 3 },
    { "wednes",         tDAY, 3 },
    { "thursday",       tDAY, 4 },
    { "thur",           tDAY, 4 },
    { "thurs",          tDAY, 4 },
    { "friday",         tDAY, 5 },
    { "saturday",       tDAY, 6 },
    { NULL }
};

/*
 * Time units table.
 */
static TABLE    UnitsTable[] = {
    { "year",           tMONTH_UNIT,    12 },
    { "month",          tMONTH_UNIT,     1 },
    { "fortnight",      tDAY_UNIT,      14 },
    { "week",           tDAY_UNIT,       7 },
    { "day",            tDAY_UNIT,       1 },
    { "hour",           tSEC_UNIT, 60 * 60 },
    { "minute",         tSEC_UNIT,      60 },
    { "min",            tSEC_UNIT,      60 },
    { "second",         tSEC_UNIT,       1 },
    { "sec",            tSEC_UNIT,       1 },
    { NULL }
};

/*
 * Assorted relative-time words.
 */
static TABLE    OtherTable[] = {
    { "tomorrow",       tDAY_UNIT,      1 },
    { "yesterday",      tDAY_UNIT,     -1 },
    { "today",          tDAY_UNIT,      0 },
    { "now",            tSEC_UNIT,      0 },
    { "last",           tUNUMBER,      -1 },
    { "this",           tSEC_UNIT,      0 },
    { "next",           tNEXT,          1 },
#if 0
    { "first",          tUNUMBER,       1 },
    { "second",         tUNUMBER,       2 },
    { "third",          tUNUMBER,       3 },
    { "fourth",         tUNUMBER,       4 },
    { "fifth",          tUNUMBER,       5 },
    { "sixth",          tUNUMBER,       6 },
    { "seventh",        tUNUMBER,       7 },
    { "eighth",         tUNUMBER,       8 },
    { "ninth",          tUNUMBER,       9 },
    { "tenth",          tUNUMBER,       10 },
    { "eleventh",       tUNUMBER,       11 },
    { "twelfth",        tUNUMBER,       12 },
#endif
    { "ago",            tAGO,   1 },
    { "epoch",          tEPOCH,   0 },
    { "stardate",       tSTARDATE, 0},
    { NULL }
};

/*
 * The timezone table.  (Note: This table was modified to not use any floating
 * point constants to work around an SGI compiler bug).
 */
static TABLE    TimezoneTable[] = {
    { "gmt",    tZONE,     HOUR( 0) },      /* Greenwich Mean */
    { "ut",     tZONE,     HOUR( 0) },      /* Universal (Coordinated) */
    { "utc",    tZONE,     HOUR( 0) },
    { "uct",    tZONE,     HOUR( 0) },      /* Universal Coordinated Time */
    { "wet",    tZONE,     HOUR( 0) },      /* Western European */
    { "bst",    tDAYZONE,  HOUR( 0) },      /* British Summer */
    { "wat",    tZONE,     HOUR( 1) },      /* West Africa */
    { "at",     tZONE,     HOUR( 2) },      /* Azores */
#if     0
    /* For completeness.  BST is also British Summer, and GST is
     * also Guam Standard. */
    { "bst",    tZONE,     HOUR( 3) },      /* Brazil Standard */
    { "gst",    tZONE,     HOUR( 3) },      /* Greenland Standard */
#endif
    { "nft",    tZONE,     HOUR( 7/2) },    /* Newfoundland */
    { "nst",    tZONE,     HOUR( 7/2) },    /* Newfoundland Standard */
    { "ndt",    tDAYZONE,  HOUR( 7/2) },    /* Newfoundland Daylight */
    { "ast",    tZONE,     HOUR( 4) },      /* Atlantic Standard */
    { "adt",    tDAYZONE,  HOUR( 4) },      /* Atlantic Daylight */
    { "est",    tZONE,     HOUR( 5) },      /* Eastern Standard */
    { "edt",    tDAYZONE,  HOUR( 5) },      /* Eastern Daylight */
    { "cst",    tZONE,     HOUR( 6) },      /* Central Standard */
    { "cdt",    tDAYZONE,  HOUR( 6) },      /* Central Daylight */
    { "mst",    tZONE,     HOUR( 7) },      /* Mountain Standard */
    { "mdt",    tDAYZONE,  HOUR( 7) },      /* Mountain Daylight */
    { "pst",    tZONE,     HOUR( 8) },      /* Pacific Standard */
    { "pdt",    tDAYZONE,  HOUR( 8) },      /* Pacific Daylight */
    { "yst",    tZONE,     HOUR( 9) },      /* Yukon Standard */
    { "ydt",    tDAYZONE,  HOUR( 9) },      /* Yukon Daylight */
    { "hst",    tZONE,     HOUR(10) },      /* Hawaii Standard */
    { "hdt",    tDAYZONE,  HOUR(10) },      /* Hawaii Daylight */
    { "cat",    tZONE,     HOUR(10) },      /* Central Alaska */
    { "ahst",   tZONE,     HOUR(10) },      /* Alaska-Hawaii Standard */
    { "nt",     tZONE,     HOUR(11) },      /* Nome */
    { "idlw",   tZONE,     HOUR(12) },      /* International Date Line West */
    { "cet",    tZONE,    -HOUR( 1) },      /* Central European */
    { "cest",   tDAYZONE, -HOUR( 1) },      /* Central European Summer */
    { "met",    tZONE,    -HOUR( 1) },      /* Middle European */
    { "mewt",   tZONE,    -HOUR( 1) },      /* Middle European Winter */
    { "mest",   tDAYZONE, -HOUR( 1) },      /* Middle European Summer */
    { "swt",    tZONE,    -HOUR( 1) },      /* Swedish Winter */
    { "sst",    tDAYZONE, -HOUR( 1) },      /* Swedish Summer */
    { "fwt",    tZONE,    -HOUR( 1) },      /* French Winter */
    { "fst",    tDAYZONE, -HOUR( 1) },      /* French Summer */
    { "eet",    tZONE,    -HOUR( 2) },      /* Eastern Europe, USSR Zone 1 */
    { "bt",     tZONE,    -HOUR( 3) },      /* Baghdad, USSR Zone 2 */
    { "it",     tZONE,    -HOUR( 7/2) },    /* Iran */
    { "zp4",    tZONE,    -HOUR( 4) },      /* USSR Zone 3 */
    { "zp5",    tZONE,    -HOUR( 5) },      /* USSR Zone 4 */
    { "ist",    tZONE,    -HOUR(11/2) },    /* Indian Standard */
    { "zp6",    tZONE,    -HOUR( 6) },      /* USSR Zone 5 */
#if     0
    /* For completeness.  NST is also Newfoundland Stanard, nad SST is
     * also Swedish Summer. */
    { "nst",    tZONE,    -HOUR(13/2) },    /* North Sumatra */
    { "sst",    tZONE,    -HOUR( 7) },      /* South Sumatra, USSR Zone 6 */
#endif  /* 0 */
    { "wast",   tZONE,    -HOUR( 7) },      /* West Australian Standard */
    { "wadt",   tDAYZONE, -HOUR( 7) },      /* West Australian Daylight */
    { "jt",     tZONE,    -HOUR(15/2) },    /* Java (3pm in Cronusland!) */
    { "cct",    tZONE,    -HOUR( 8) },      /* China Coast, USSR Zone 7 */
    { "jst",    tZONE,    -HOUR( 9) },      /* Japan Standard, USSR Zone 8 */
    { "cast",   tZONE,    -HOUR(19/2) },    /* Central Australian Standard */
    { "cadt",   tDAYZONE, -HOUR(19/2) },    /* Central Australian Daylight */
    { "east",   tZONE,    -HOUR(10) },      /* Eastern Australian Standard */
    { "eadt",   tDAYZONE, -HOUR(10) },      /* Eastern Australian Daylight */
    { "gst",    tZONE,    -HOUR(10) },      /* Guam Standard, USSR Zone 9 */
    { "nzt",    tZONE,    -HOUR(12) },      /* New Zealand */
    { "nzst",   tZONE,    -HOUR(12) },      /* New Zealand Standard */
    { "nzdt",   tDAYZONE, -HOUR(12) },      /* New Zealand Daylight */
    { "idle",   tZONE,    -HOUR(12) },      /* International Date Line East */
    /* ADDED BY Marco Nijdam */
    { "dst",    tDST,     HOUR( 0) },       /* DST on (hour is ignored) */
    /* End ADDED */
    {  NULL  }
};

/*
 * Military timezone table.
 */
static TABLE    MilitaryTable[] = {
    { "a",      tZONE,  HOUR(  1) },
    { "b",      tZONE,  HOUR(  2) },
    { "c",      tZONE,  HOUR(  3) },
    { "d",      tZONE,  HOUR(  4) },
    { "e",      tZONE,  HOUR(  5) },
    { "f",      tZONE,  HOUR(  6) },
    { "g",      tZONE,  HOUR(  7) },
    { "h",      tZONE,  HOUR(  8) },
    { "i",      tZONE,  HOUR(  9) },
    { "k",      tZONE,  HOUR( 10) },
    { "l",      tZONE,  HOUR( 11) },
    { "m",      tZONE,  HOUR( 12) },
    { "n",      tZONE,  HOUR(- 1) },
    { "o",      tZONE,  HOUR(- 2) },
    { "p",      tZONE,  HOUR(- 3) },
    { "q",      tZONE,  HOUR(- 4) },
    { "r",      tZONE,  HOUR(- 5) },
    { "s",      tZONE,  HOUR(- 6) },
    { "t",      tZONE,  HOUR(- 7) },
    { "u",      tZONE,  HOUR(- 8) },
    { "v",      tZONE,  HOUR(- 9) },
    { "w",      tZONE,  HOUR(-10) },
    { "x",      tZONE,  HOUR(-11) },
    { "y",      tZONE,  HOUR(-12) },
    { "z",      tZONE,  HOUR(  0) },
    { NULL }
};


/*
 * Dump error messages in the bit bucket.
 */
static void
date_Dateerror(s)
    char  *s;
{
}

int date_timezone(time_t time)
{
	struct tm *tm = localtime(&time);
	int timezone = -(tm->tm_gmtoff/60);

	/* Adjust for daylight savings. */
	if (tm->tm_isdst) timezone += 60;

	return(timezone);
}

static time_t
ToSeconds(Hours, Minutes, Seconds, Meridian)
    time_t      Hours;
    time_t      Minutes;
    time_t      Seconds;
    MERIDIAN    Meridian;
{
    if (Minutes < 0 || Minutes > 59 || Seconds < 0 || Seconds > 59)
        return -1;
    switch (Meridian) {
    case MER24:
        if (Hours < 0 || Hours > 23)
            return -1;
        return (Hours * 60L + Minutes) * 60L + Seconds;
    case MERam:
        if (Hours < 1 || Hours > 12)
            return -1;
        return ((Hours % 12) * 60L + Minutes) * 60L + Seconds;
    case MERpm:
        if (Hours < 1 || Hours > 12)
            return -1;
        return (((Hours % 12) + 12) * 60L + Minutes) * 60L + Seconds;
    }
    return -1;  /* Should never be reached */
}

/*
 *-----------------------------------------------------------------------------
 *
 * Convert --
 *
 *      Convert a {month, day, year, hours, minutes, seconds, meridian, dst}
 *      tuple into a clock seconds value.
 *
 * Results:
 *      0 or -1 indicating success or failure.
 *
 * Side effects:
 *      Fills TimePtr with the computed value.
 *
 *-----------------------------------------------------------------------------
 */
static int
Convert(Month, Day, Year, Hours, Minutes, Seconds, Meridian, DSTmode, TimePtr)
    time_t      Month;
    time_t      Day;
    time_t      Year;
    time_t      Hours;
    time_t      Minutes;
    time_t      Seconds;
    MERIDIAN    Meridian;
    DSTMODE     DSTmode;
    time_t     *TimePtr;
{
    static int  DaysInMonth[12] = {
        31, 0, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };
    time_t tod;
    time_t Julian;
    int i;

    /* Figure out how many days are in February for the given year.
     * Every year divisible by 4 is a leap year.
     * But, every year divisible by 100 is not a leap year.
     * But, every year divisible by 400 is a leap year after all.
     */
    DaysInMonth[1] = IsLeapYear(Year) ? 29 : 28;

    /* Check the inputs for validity */
    if (Month < 1 || Month > 12
	    || Year < START_OF_TIME || Year > END_OF_TIME
	    || Day < 1 || Day > DaysInMonth[(int)--Month])
        return -1;

    /* Start computing the value.  First determine the number of days
     * represented by the date, then multiply by the number of seconds/day.
     */
    for (Julian = Day - 1, i = 0; i < Month; i++)
        Julian += DaysInMonth[i];
    if (Year >= EPOCH) {
        for (i = EPOCH; i < Year; i++)
            Julian += 365 + IsLeapYear(i);
    } else {
        for (i = Year; i < EPOCH; i++)
            Julian -= 365 + IsLeapYear(i);
    }
    Julian *= SECSPERDAY;

    /* Add the timezone offset ?? */
    Julian += date_DateTimezone * 60L;

    /* Add the number of seconds represented by the time component */
    if ((tod = ToSeconds(Hours, Minutes, Seconds, Meridian)) < 0)
        return -1;
    Julian += tod;

    /* Perform a preliminary DST compensation ?? */
    if (DSTmode == DSTon
     || (DSTmode == DSTmaybe && localtime(&Julian)->tm_isdst))
        Julian -= 60 * 60;
    *TimePtr = Julian;
    return 0;
}


static time_t
DSTcorrect(Start, Future)
    time_t      Start;
    time_t      Future;
{
    time_t      StartDay;
    time_t      FutureDay;
    StartDay = (localtime(&Start)->tm_hour + 1) % 24;
    FutureDay = (localtime(&Future)->tm_hour + 1) % 24;
    return (Future - Start) + (StartDay - FutureDay) * 60L * 60L;
}


static time_t
NamedDay(Start, DayOrdinal, DayNumber)
    time_t      Start;
    time_t      DayOrdinal;
    time_t      DayNumber;
{
    struct tm   *tm;
    time_t      now;

    now = Start;
    tm = localtime(&now);
    now += SECSPERDAY * ((DayNumber - tm->tm_wday + 7) % 7);
    now += 7 * SECSPERDAY * (DayOrdinal <= 0 ? DayOrdinal : DayOrdinal - 1);
    return DSTcorrect(Start, now);
}

static time_t
NamedMonth(Start, MonthOrdinal, MonthNumber)
    time_t Start;
    time_t MonthOrdinal;
    time_t MonthNumber;
{
    struct tm *tm;
    time_t now;
    int result;
    
    now = Start;
    tm = localtime(&now);
    /* To compute the next n'th month, we use this alg:
     * add n to year value
     * if currentMonth < requestedMonth decrement year value by 1 (so that
     *  doing next february from january gives us february of the current year)
     * set day to 1, time to 0
     */
    tm->tm_year += MonthOrdinal;
    if (tm->tm_mon < MonthNumber - 1) {
	tm->tm_year--;
    }
    result = Convert(MonthNumber, (time_t) 1, tm->tm_year + TM_YEAR_BASE,
	    (time_t) 0, (time_t) 0, (time_t) 0, MER24, DSTmaybe, &now);
    if (result < 0) {
	return 0;
    }
    return DSTcorrect(Start, now);
}

static int
RelativeMonth(Start, RelMonth, TimePtr)
    time_t Start;
    time_t RelMonth;
    time_t *TimePtr;
{
    struct tm *tm;
    time_t Month;
    time_t Year;
    time_t Julian;
    int result;

    if (RelMonth == 0) {
        *TimePtr = 0;
        return 0;
    }
    tm = localtime(&Start);
    Month = 12 * (tm->tm_year + TM_YEAR_BASE) + tm->tm_mon + RelMonth;
    Year = Month / 12;
    Month = Month % 12 + 1;
    result = Convert(Month, (time_t) tm->tm_mday, Year,
	    (time_t) tm->tm_hour, (time_t) tm->tm_min, (time_t) tm->tm_sec,
	    MER24, DSTmaybe, &Julian);

    /*
     * The Julian time returned above is behind by one day, if "month" 
     * or "year" is used to specify relative time and the GMT flag is true.
     * This problem occurs only when the current time is closer to
     * midnight, the difference being not more than its time difference
     * with GMT. For example, in US/Pacific time zone, the problem occurs
     * whenever the current time is between midnight to 8:00am or 7:00amDST.
     * See Bug# 413397 for more details and sample script.
     * To resolve this bug, we simply add the number of seconds corresponding
     * to timezone difference with GMT to Julian time, if GMT flag is true.
     */

    if (date_DateTimezone == 0) {
        Julian += date_timezone((unsigned long) Start) * 60L;
    }

    /*
     * The following iteration takes into account the case were we jump
     * into a "short month".  Far example, "one month from Jan 31" will
     * fail because there is no Feb 31.  The code below will reduce the
     * day and try converting the date until we succed or the date equals
     * 28 (which always works unless the date is bad in another way).
     */

    while ((result != 0) && (tm->tm_mday > 28)) {
	tm->tm_mday--;
	result = Convert(Month, (time_t) tm->tm_mday, Year,
		(time_t) tm->tm_hour, (time_t) tm->tm_min, (time_t) tm->tm_sec,
		MER24, DSTmaybe, &Julian);
    }
    if (result != 0) {
	return -1;
    }
    *TimePtr = DSTcorrect(Start, Julian);
    return 0;
}


/*
 *-----------------------------------------------------------------------------
 *
 * RelativeDay --
 *
 *      Given a starting time and a number of days before or after, compute the
 *      DST corrected difference between those dates.
 *
 * Results:
 *     1 or -1 indicating success or failure.
 *
 * Side effects:
 *      Fills TimePtr with the computed value.
 *
 *-----------------------------------------------------------------------------
 */

static int
RelativeDay(Start, RelDay, TimePtr)
    time_t Start;
    time_t RelDay;
    time_t *TimePtr;
{
    time_t new;

    new = Start + (RelDay * 60 * 60 * 24);
    *TimePtr = DSTcorrect(Start, new);
    return 1;
}

static int
LookupWord(buff)
    char                *buff;
{
    register char *p;
    register char *q;
    register TABLE *tp;
    int i;
    int abbrev;

    /*
     * Make it lowercase.
     */

    str_tolower(buff);

    if (strcmp(buff, "am") == 0 || strcmp(buff, "a.m.") == 0) {
        date_Datelval.Meridian = MERam;
        return tMERIDIAN;
    }
    if (strcmp(buff, "pm") == 0 || strcmp(buff, "p.m.") == 0) {
        date_Datelval.Meridian = MERpm;
        return tMERIDIAN;
    }

    /*
     * See if we have an abbreviation for a month.
     */
    if (strlen(buff) == 3) {
        abbrev = 1;
    } else if (strlen(buff) == 4 && buff[3] == '.') {
        abbrev = 1;
        buff[3] = '\0';
    } else {
        abbrev = 0;
    }

    for (tp = MonthDayTable; tp->name; tp++) {
        if (abbrev) {
            if (strncmp(buff, tp->name, 3) == 0) {
                date_Datelval.Number = tp->value;
                return tp->type;
            }
        } else if (strcmp(buff, tp->name) == 0) {
            date_Datelval.Number = tp->value;
            return tp->type;
        }
    }

    for (tp = TimezoneTable; tp->name; tp++) {
        if (strcmp(buff, tp->name) == 0) {
            date_Datelval.Number = tp->value;
            return tp->type;
        }
    }

    for (tp = UnitsTable; tp->name; tp++) {
        if (strcmp(buff, tp->name) == 0) {
            date_Datelval.Number = tp->value;
            return tp->type;
        }
    }

    /*
     * Strip off any plural and try the units table again.
     */
    i = strlen(buff) - 1;
    if (buff[i] == 's') {
        buff[i] = '\0';
        for (tp = UnitsTable; tp->name; tp++) {
            if (strcmp(buff, tp->name) == 0) {
                date_Datelval.Number = tp->value;
                return tp->type;
            }
	}
    }

    for (tp = OtherTable; tp->name; tp++) {
        if (strcmp(buff, tp->name) == 0) {
            date_Datelval.Number = tp->value;
            return tp->type;
        }
    }

    /*
     * Military timezones.
     */
    if (buff[1] == '\0' && !(*buff & 0x80)
	    && isalpha(*buff)) {	/* INTL: ISO only */
        for (tp = MilitaryTable; tp->name; tp++) {
            if (strcmp(buff, tp->name) == 0) {
                date_Datelval.Number = tp->value;
                return tp->type;
            }
	}
    }

    /*
     * Drop out any periods and try the timezone table again.
     */
    for (i = 0, p = q = buff; *q; q++)
        if (*q != '.') {
            *p++ = *q;
        } else {
            i++;
	}
    *p = '\0';
    if (i) {
        for (tp = TimezoneTable; tp->name; tp++) {
            if (strcmp(buff, tp->name) == 0) {
                date_Datelval.Number = tp->value;
                return tp->type;
            }
	}
    }
    
    return tID;
}


static int
date_Datelex()
{
    register char       c;
    register char       *p;
    char                buff[20];
    int                 Count;

    for ( ; ; ) {
        while (isspace(*date_DateInput)) {
            date_DateInput++;
	}

        if (isdigit((c = *date_DateInput))) { /* INTL: digit */
	    /* convert the string into a number; count the number of digits */
	    Count = 0;
            for (date_Datelval.Number = 0;
		    isdigit((c = *date_DateInput++)); ) { /* INTL: digit */
                date_Datelval.Number = 10 * date_Datelval.Number + c - '0';
		Count++;
	    }
            date_DateInput--;
	    /* A number with 6 or more digits is considered an ISO 8601 base */
	    if (Count >= 6) {
		return tISOBASE;
	    } else {
		return tUNUMBER;
	    }
        }
        if (!(c & 0x80) && isalpha(c)) {	/* INTL: ISO only. */
            for (p = buff; isalpha((c = *date_DateInput++)) /* INTL: ISO only. */
		     || c == '.'; ) {
                if (p < &buff[sizeof buff - 1]) {
                    *p++ = c;
		}
	    }
            *p = '\0';
            date_DateInput--;
            return LookupWord(buff);
        }
        if (c != '(') {
            return *date_DateInput++;
	}
        Count = 0;
        do {
            c = *date_DateInput++;
            if (c == '\0') {
                return c;
	    } else if (c == '(') {
                Count++;
	    } else if (c == ')') {
                Count--;
	    }
        } while (Count > 0);
    }
}

/*
 * Specify zone is of -50000 to force GMT.  (This allows BST to work).
 */

int date_scan(char *timestr, time_t *now, int zone, time_t *timeptr)
{
    struct tm *tm;
    time_t Start;
    time_t Time;
    time_t tod;
    int thisyear;

    /* Look up timezone if required. */
    if (zone == EGG_TIMEZONE_LOOKUP) zone = date_timezone(0);

    date_DateInput = timestr;
    /* now has to be cast to a time_t for 64bit compliance */
    Start = *now;
    tm = localtime(&Start);
    thisyear = tm->tm_year + TM_YEAR_BASE;
    date_DateYear = thisyear;
    date_DateMonth = tm->tm_mon + 1;
    date_DateDay = tm->tm_mday;
    date_DateTimezone = zone;
    if (zone == -50000) {
        date_DateDSTmode = DSToff;  /* assume GMT */
        date_DateTimezone = 0;
    } else {
        date_DateDSTmode = DSTmaybe;
    }
    date_DateHour = 0;
    date_DateMinutes = 0;
    date_DateSeconds = 0;
    date_DateMeridian = MER24;
    date_DateRelSeconds = 0;
    date_DateRelMonth = 0;
    date_DateRelDay = 0;
    date_DateRelPointer = NULL;

    date_DateHaveDate = 0;
    date_DateHaveDay = 0;
    date_DateHaveOrdinalMonth = 0;
    date_DateHaveRel = 0;
    date_DateHaveTime = 0;
    date_DateHaveZone = 0;

    if (date_Dateparse() || date_DateHaveTime > 1 || date_DateHaveZone > 1 || date_DateHaveDate > 1 ||
	    date_DateHaveDay > 1 || date_DateHaveOrdinalMonth > 1) {
        return -1;
    }
    
    if (date_DateHaveDate || date_DateHaveTime || date_DateHaveDay) {
	if (date_DateYear < 0) {
	    date_DateYear = -date_DateYear;
	}
	/*
	 * The following line handles years that are specified using
	 * only two digits.  The line of code below implements a policy
	 * defined by the X/Open workgroup on the millinium rollover.
	 * Note: some of those dates may not actually be valid on some
	 * platforms.  The POSIX standard startes that the dates 70-99
	 * shall refer to 1970-1999 and 00-38 shall refer to 2000-2038.
	 * This later definition should work on all platforms.
	 */

	if (date_DateYear < 100) {
	    if (date_DateYear >= 69) {
		date_DateYear += 1900;
	    } else {
		date_DateYear += 2000;
	    }
	}
	if (Convert(date_DateMonth, date_DateDay, date_DateYear, date_DateHour, date_DateMinutes, date_DateSeconds,
		date_DateMeridian, date_DateDSTmode, &Start) < 0) {
            return -1;
	}
    } else {
        Start = *now;
        if (!date_DateHaveRel) {
            Start -= ((tm->tm_hour * 60L * 60L) +
		    tm->tm_min * 60L) +	tm->tm_sec;
	}
    }

    Start += date_DateRelSeconds;
    if (RelativeMonth(Start, date_DateRelMonth, &Time) < 0) {
        return -1;
    }
    Start += Time;

    if (RelativeDay(Start, date_DateRelDay, &Time) < 0) {
	return -1;
    }
    Start += Time;
    
    if (date_DateHaveDay && !date_DateHaveDate) {
        tod = NamedDay(Start, date_DateDayOrdinal, date_DateDayNumber);
        Start += tod;
    }

    if (date_DateHaveOrdinalMonth) {
	tod = NamedMonth(Start, date_DateMonthOrdinal, date_DateMonth);
	Start += tod;
    }
    
    *timeptr = Start;
    return 0;
}
static date_Datetabelem date_Dateexca[] ={
-1, 1,
	0, -1,
	-2, 0,
	};
# define YYNPROD 56
# define YYLAST 261
static date_Datetabelem date_Dateact[]={

    24,    40,    23,    36,    54,    81,    41,    28,    53,    26,
    37,    42,    58,    38,    56,    28,    27,    26,    28,    33,
    26,    32,    61,    50,    27,    80,    76,    27,    51,    75,
    74,    73,    30,    72,    71,    70,    69,    52,    49,    48,
    47,    45,    39,    62,    78,    46,    79,    68,    25,    65,
    60,    67,    66,    55,    44,    21,    63,    11,    10,     9,
     8,    35,     7,     6,     5,     4,     3,    43,     2,     1,
    20,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,    57,     0,     0,    59,    77,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,    19,    14,     0,     0,     0,
    16,    28,    22,    26,     0,    12,    13,    17,     0,    15,
    27,    18,    31,     0,     0,    29,     0,    34,    28,     0,
    26,     0,     0,     0,     0,     0,     0,    27,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,    64,
    64 };
static date_Datetabelem date_Datepact[]={

-10000000,   -43,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,
-10000000,-10000000,   -26,  -268,-10000000,  -259,  -226,-10000000,  -257,    10,
  -227,  -212,  -228,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,
  -229,-10000000,  -230,  -240,  -231,-10000000,-10000000,  -264,-10000000,     9,
-10000000,-10000000,  -249,-10000000,-10000000,  -246,-10000000,     4,    -2,     2,
     7,     6,-10000000,-10000000,   -11,  -232,-10000000,-10000000,-10000000,-10000000,
  -233,-10000000,  -234,  -235,-10000000,  -237,  -238,  -239,  -242,-10000000,
-10000000,-10000000,    -1,-10000000,-10000000,-10000000,   -12,-10000000,  -243,  -263,
-10000000,-10000000 };
static date_Datetabelem date_Datepgo[]={

     0,    48,    70,    22,    69,    68,    66,    65,    64,    63,
    62,    60,    59,    58,    57,    55 };
static date_Datetabelem date_Dater1[]={

     0,     4,     4,     5,     5,     5,     5,     5,     5,     5,
     5,     5,     6,     6,     6,     6,     6,     7,     7,     7,
    10,    10,    10,    10,    10,     8,     8,     8,     8,     8,
     8,     8,     8,     8,     8,     9,     9,    12,    12,    12,
    13,    11,    11,    15,    15,    15,    15,    15,     2,     2,
     1,     1,     1,    14,     3,     3 };
static date_Datetabelem date_Dater2[]={

     0,     0,     4,     3,     3,     3,     3,     3,     3,     3,
     3,     2,     5,     9,    11,    13,    15,     5,     3,     3,
     3,     5,     5,     7,     5,     7,    11,     3,    11,    11,
     5,     9,     5,     3,     7,     5,     7,     7,    15,     5,
     9,     5,     2,     7,     5,     5,     7,     3,     3,     3,
     3,     3,     3,     3,     1,     3 };
static date_Datetabelem date_Datechk[]={

-10000000,    -4,    -5,    -6,    -7,    -8,    -9,   -10,   -11,   -12,
   -13,   -14,   268,   269,   259,   272,   263,   270,   274,   258,
    -2,   -15,   265,    45,    43,    -1,   266,   273,   264,   261,
    58,   258,    47,    45,   263,    -1,   271,   269,   272,   268,
   258,   263,   268,    -1,    44,   268,   257,   268,   268,   268,
   263,   268,   268,   272,   268,    44,   263,    -1,   258,    -1,
    46,    -3,    45,    58,   261,    47,    45,    45,    58,   268,
   268,   268,   268,   268,   268,   268,   268,    -3,    45,    58,
   268,   268 };
static date_Datetabelem date_Datedef[]={

     1,    -2,     2,     3,     4,     5,     6,     7,     8,     9,
    10,    11,    53,    18,    19,    27,     0,    33,     0,    20,
     0,    42,     0,    48,    49,    47,    50,    51,    52,    12,
     0,    22,     0,     0,    32,    44,    17,     0,    39,    30,
    24,    35,     0,    45,    21,     0,    41,     0,    54,    25,
     0,     0,    34,    37,     0,     0,    36,    46,    23,    43,
     0,    13,     0,     0,    55,     0,     0,     0,     0,    31,
    40,    14,    54,    26,    28,    29,     0,    15,     0,     0,
    16,    38 };
typedef struct
#ifdef __cplusplus
	date_Datetoktype
#endif
{ char *t_name; int t_val; } date_Datetoktype;
#ifndef YYDEBUG
#	define YYDEBUG	0	/* don't allow debugging */
#endif

#if YYDEBUG

date_Datetoktype date_Datetoks[] =
{
	"tAGO",	257,
	"tDAY",	258,
	"tDAYZONE",	259,
	"tID",	260,
	"tMERIDIAN",	261,
	"tMINUTE_UNIT",	262,
	"tMONTH",	263,
	"tMONTH_UNIT",	264,
	"tSTARDATE",	265,
	"tSEC_UNIT",	266,
	"tSNUMBER",	267,
	"tUNUMBER",	268,
	"tZONE",	269,
	"tEPOCH",	270,
	"tDST",	271,
	"tISOBASE",	272,
	"tDAY_UNIT",	273,
	"tNEXT",	274,
	"-unknown-",	-1	/* ends search */
};

char * date_Datereds[] =
{
	"-no such reduction-",
	"spec : /* empty */",
	"spec : spec item",
	"item : time",
	"item : zone",
	"item : date",
	"item : ordMonth",
	"item : day",
	"item : relspec",
	"item : iso",
	"item : trek",
	"item : number",
	"time : tUNUMBER tMERIDIAN",
	"time : tUNUMBER ':' tUNUMBER o_merid",
	"time : tUNUMBER ':' tUNUMBER '-' tUNUMBER",
	"time : tUNUMBER ':' tUNUMBER ':' tUNUMBER o_merid",
	"time : tUNUMBER ':' tUNUMBER ':' tUNUMBER '-' tUNUMBER",
	"zone : tZONE tDST",
	"zone : tZONE",
	"zone : tDAYZONE",
	"day : tDAY",
	"day : tDAY ','",
	"day : tUNUMBER tDAY",
	"day : sign tUNUMBER tDAY",
	"day : tNEXT tDAY",
	"date : tUNUMBER '/' tUNUMBER",
	"date : tUNUMBER '/' tUNUMBER '/' tUNUMBER",
	"date : tISOBASE",
	"date : tUNUMBER '-' tMONTH '-' tUNUMBER",
	"date : tUNUMBER '-' tUNUMBER '-' tUNUMBER",
	"date : tMONTH tUNUMBER",
	"date : tMONTH tUNUMBER ',' tUNUMBER",
	"date : tUNUMBER tMONTH",
	"date : tEPOCH",
	"date : tUNUMBER tMONTH tUNUMBER",
	"ordMonth : tNEXT tMONTH",
	"ordMonth : tNEXT tUNUMBER tMONTH",
	"iso : tISOBASE tZONE tISOBASE",
	"iso : tISOBASE tZONE tUNUMBER ':' tUNUMBER ':' tUNUMBER",
	"iso : tISOBASE tISOBASE",
	"trek : tSTARDATE tUNUMBER '.' tUNUMBER",
	"relspec : relunits tAGO",
	"relspec : relunits",
	"relunits : sign tUNUMBER unit",
	"relunits : tUNUMBER unit",
	"relunits : tNEXT unit",
	"relunits : tNEXT tUNUMBER unit",
	"relunits : unit",
	"sign : '-'",
	"sign : '+'",
	"unit : tSEC_UNIT",
	"unit : tDAY_UNIT",
	"unit : tMONTH_UNIT",
	"number : tUNUMBER",
	"o_merid : /* empty */",
	"o_merid : tMERIDIAN",
};
#endif /* YYDEBUG */
/*
 * Copyright (c) 1993 by Sun Microsystems, Inc.
 */


/*
** Skeleton parser driver for yacc output
*/

/*
** yacc user known macros and defines
*/
#define YYERROR		goto date_Dateerrlab
#define YYACCEPT	return(0)
#define YYABORT		return(1)
#define YYBACKUP( newtoken, newvalue )\
{\
	if ( date_Datechar >= 0 || ( date_Dater2[ date_Datetmp ] >> 1 ) != 1 )\
	{\
		date_Dateerror( "syntax error - cannot backup" );\
		goto date_Dateerrlab;\
	}\
	date_Datechar = newtoken;\
	date_Datestate = *date_Dateps;\
	date_Datelval = newvalue;\
	goto date_Datenewstate;\
}
#define YYRECOVERING()	(!!date_Dateerrflag)
#define YYNEW(type)	malloc(sizeof(type) * date_Datenewmax)
#define YYCOPY(to, from, type) \
	(type *) memcpy(to, (char *) from, date_Datemaxdepth * sizeof (type))
#define YYENLARGE( from, type) \
	(type *) realloc((char *) from, date_Datenewmax * sizeof(type))
#ifndef YYDEBUG
#	define YYDEBUG	1	/* make debugging available */
#endif

/*
** user known globals
*/
int date_Datedebug;			/* set to 1 to get debugging */

/*
** driver internal defines
*/
#define YYFLAG		(-10000000)

/*
** global variables used by the parser
*/
YYSTYPE *date_Datepv;			/* top of value stack */
int *date_Dateps;			/* top of state stack */

int date_Datestate;			/* current state */
int date_Datetmp;			/* extra var (lasts between blocks) */

int date_Datenerrs;			/* number of errors */
int date_Dateerrflag;			/* error recovery flag */
int date_Datechar;			/* current input token number */



#ifdef YYNMBCHARS
#define YYLEX()		date_Datecvtok(date_Datelex())
/*
** date_Datecvtok - return a token if i is a wchar_t value that exceeds 255.
**	If i<255, i itself is the token.  If i>255 but the neither 
**	of the 30th or 31st bit is on, i is already a token.
*/
#if defined(__STDC__) || defined(__cplusplus)
int date_Datecvtok(int i)
#else
int date_Datecvtok(i) int i;
#endif
{
	int first = 0;
	int last = YYNMBCHARS - 1;
	int mid;
	wchar_t j;

	if(i&0x60000000){/*Must convert to a token. */
		if( date_Datembchars[last].character < i ){
			return i;/*Giving up*/
		}
		while ((last>=first)&&(first>=0)) {/*Binary search loop*/
			mid = (first+last)/2;
			j = date_Datembchars[mid].character;
			if( j==i ){/*Found*/ 
				return date_Datembchars[mid].tvalue;
			}else if( j<i ){
				first = mid + 1;
			}else{
				last = mid -1;
			}
		}
		/*No entry in the table.*/
		return i;/* Giving up.*/
	}else{/* i is already a token. */
		return i;
	}
}
#else/*!YYNMBCHARS*/
#define YYLEX()		date_Datelex()
#endif/*!YYNMBCHARS*/

/*
** date_Dateparse - return 0 if worked, 1 if syntax error not recovered from
*/
#if defined(__STDC__) || defined(__cplusplus)
int date_Dateparse(void)
#else
int date_Dateparse()
#endif
{
	register YYSTYPE *date_Datepvt = 0;	/* top of value stack for $vars */

#if defined(__cplusplus) || defined(lint)
/*
	hacks to please C++ and lint - goto's inside
	switch should never be executed
*/
	static int __yaccpar_lint_hack__ = 0;
	switch (__yaccpar_lint_hack__)
	{
		case 1: goto date_Dateerrlab;
		case 2: goto date_Datenewstate;
	}
#endif

	/*
	** Initialize externals - date_Dateparse may be called more than once
	*/
	date_Datepv = &date_Datev[-1];
	date_Dateps = &date_Dates[-1];
	date_Datestate = 0;
	date_Datetmp = 0;
	date_Datenerrs = 0;
	date_Dateerrflag = 0;
	date_Datechar = -1;

#if YYMAXDEPTH <= 0
	if (date_Datemaxdepth <= 0)
	{
		if ((date_Datemaxdepth = YYEXPAND(0)) <= 0)
		{
			date_Dateerror("yacc initialization error");
			YYABORT;
		}
	}
#endif

	{
		register YYSTYPE *date_Date_pv;	/* top of value stack */
		register int *date_Date_ps;		/* top of state stack */
		register int date_Date_state;		/* current state */
		register int  date_Date_n;		/* internal state number info */
	goto date_Datestack;	/* moved from 6 lines above to here to please C++ */

		/*
		** get globals into registers.
		** branch to here only if YYBACKUP was called.
		*/
		date_Date_pv = date_Datepv;
		date_Date_ps = date_Dateps;
		date_Date_state = date_Datestate;
		goto date_Date_newstate;

		/*
		** get globals into registers.
		** either we just started, or we just finished a reduction
		*/
	date_Datestack:
		date_Date_pv = date_Datepv;
		date_Date_ps = date_Dateps;
		date_Date_state = date_Datestate;

		/*
		** top of for (;;) loop while no reductions done
		*/
	date_Date_stack:
		/*
		** put a state and value onto the stacks
		*/
#if YYDEBUG
		/*
		** if debugging, look up token value in list of value vs.
		** name pairs.  0 and negative (-1) are special values.
		** Note: linear search is used since time is not a real
		** consideration while debugging.
		*/
		if ( date_Datedebug )
		{
			register int date_Date_i;

			printf( "State %d, token ", date_Date_state );
			if ( date_Datechar == 0 )
				printf( "end-of-file\n" );
			else if ( date_Datechar < 0 )
				printf( "-none-\n" );
			else
			{
				for ( date_Date_i = 0; date_Datetoks[date_Date_i].t_val >= 0;
					date_Date_i++ )
				{
					if ( date_Datetoks[date_Date_i].t_val == date_Datechar )
						break;
				}
				printf( "%s\n", date_Datetoks[date_Date_i].t_name );
			}
		}
#endif /* YYDEBUG */
		if ( ++date_Date_ps >= &date_Dates[ date_Datemaxdepth ] )	/* room on stack? */
		{
			/*
			** reallocate and recover.  Note that pointers
			** have to be reset, or bad things will happen
			*/
			long date_Dateps_index = (date_Date_ps - date_Dates);
			long date_Datepv_index = (date_Date_pv - date_Datev);
			long date_Datepvt_index = (date_Datepvt - date_Datev);
			int date_Datenewmax;
#ifdef YYEXPAND
			date_Datenewmax = YYEXPAND(date_Datemaxdepth);
#else
			date_Datenewmax = 2 * date_Datemaxdepth;	/* double table size */
			if (date_Datemaxdepth == YYMAXDEPTH)	/* first time growth */
			{
				char *newdate_Dates = (char *)YYNEW(int);
				char *newdate_Datev = (char *)YYNEW(YYSTYPE);
				if (newdate_Dates != 0 && newdate_Datev != 0)
				{
					date_Dates = YYCOPY(newdate_Dates, date_Dates, int);
					date_Datev = YYCOPY(newdate_Datev, date_Datev, YYSTYPE);
				}
				else
					date_Datenewmax = 0;	/* failed */
			}
			else				/* not first time */
			{
				date_Dates = YYENLARGE(date_Dates, int);
				date_Datev = YYENLARGE(date_Datev, YYSTYPE);
				if (date_Dates == 0 || date_Datev == 0)
					date_Datenewmax = 0;	/* failed */
			}
#endif
			if (date_Datenewmax <= date_Datemaxdepth)	/* tables not expanded */
			{
				date_Dateerror( "yacc stack overflow" );
				YYABORT;
			}
			date_Datemaxdepth = date_Datenewmax;

			date_Date_ps = date_Dates + date_Dateps_index;
			date_Date_pv = date_Datev + date_Datepv_index;
			date_Datepvt = date_Datev + date_Datepvt_index;
		}
		*date_Date_ps = date_Date_state;
		*++date_Date_pv = date_Dateval;

		/*
		** we have a new state - find out what to do
		*/
	date_Date_newstate:
		if ( ( date_Date_n = date_Datepact[ date_Date_state ] ) <= YYFLAG )
			goto date_Datedefault;		/* simple state */
#if YYDEBUG
		/*
		** if debugging, need to mark whether new token grabbed
		*/
		date_Datetmp = date_Datechar < 0;
#endif
		if ( ( date_Datechar < 0 ) && ( ( date_Datechar = YYLEX() ) < 0 ) )
			date_Datechar = 0;		/* reached EOF */
#if YYDEBUG
		if ( date_Datedebug && date_Datetmp )
		{
			register int date_Date_i;

			printf( "Received token " );
			if ( date_Datechar == 0 )
				printf( "end-of-file\n" );
			else if ( date_Datechar < 0 )
				printf( "-none-\n" );
			else
			{
				for ( date_Date_i = 0; date_Datetoks[date_Date_i].t_val >= 0;
					date_Date_i++ )
				{
					if ( date_Datetoks[date_Date_i].t_val == date_Datechar )
						break;
				}
				printf( "%s\n", date_Datetoks[date_Date_i].t_name );
			}
		}
#endif /* YYDEBUG */
		if ( ( ( date_Date_n += date_Datechar ) < 0 ) || ( date_Date_n >= YYLAST ) )
			goto date_Datedefault;
		if ( date_Datechk[ date_Date_n = date_Dateact[ date_Date_n ] ] == date_Datechar )	/*valid shift*/
		{
			date_Datechar = -1;
			date_Dateval = date_Datelval;
			date_Date_state = date_Date_n;
			if ( date_Dateerrflag > 0 )
				date_Dateerrflag--;
			goto date_Date_stack;
		}

	date_Datedefault:
		if ( ( date_Date_n = date_Datedef[ date_Date_state ] ) == -2 )
		{
#if YYDEBUG
			date_Datetmp = date_Datechar < 0;
#endif
			if ( ( date_Datechar < 0 ) && ( ( date_Datechar = YYLEX() ) < 0 ) )
				date_Datechar = 0;		/* reached EOF */
#if YYDEBUG
			if ( date_Datedebug && date_Datetmp )
			{
				register int date_Date_i;

				printf( "Received token " );
				if ( date_Datechar == 0 )
					printf( "end-of-file\n" );
				else if ( date_Datechar < 0 )
					printf( "-none-\n" );
				else
				{
					for ( date_Date_i = 0;
						date_Datetoks[date_Date_i].t_val >= 0;
						date_Date_i++ )
					{
						if ( date_Datetoks[date_Date_i].t_val
							== date_Datechar )
						{
							break;
						}
					}
					printf( "%s\n", date_Datetoks[date_Date_i].t_name );
				}
			}
#endif /* YYDEBUG */
			/*
			** look through exception table
			*/
			{
				register int *date_Datexi = date_Dateexca;

				while ( ( *date_Datexi != -1 ) ||
					( date_Datexi[1] != date_Date_state ) )
				{
					date_Datexi += 2;
				}
				while ( ( *(date_Datexi += 2) >= 0 ) &&
					( *date_Datexi != date_Datechar ) )
					;
				if ( ( date_Date_n = date_Datexi[1] ) < 0 )
					YYACCEPT;
			}
		}

		/*
		** check for syntax error
		*/
		if ( date_Date_n == 0 )	/* have an error */
		{
			/* no worry about speed here! */
			switch ( date_Dateerrflag )
			{
			case 0:		/* new error */
				date_Dateerror( "syntax error" );
				goto skip_init;
				/*
				** get globals into registers.
				** we have a user generated syntax type error
				*/
				date_Date_pv = date_Datepv;
				date_Date_ps = date_Dateps;
				date_Date_state = date_Datestate;
			skip_init:
				date_Datenerrs++;
				/* FALLTHRU */
			case 1:
			case 2:		/* incompletely recovered error */
					/* try again... */
				date_Dateerrflag = 3;
				/*
				** find state where "error" is a legal
				** shift action
				*/
				while ( date_Date_ps >= date_Dates )
				{
					date_Date_n = date_Datepact[ *date_Date_ps ] + YYERRCODE;
					if ( date_Date_n >= 0 && date_Date_n < YYLAST &&
						date_Datechk[date_Dateact[date_Date_n]] == YYERRCODE)					{
						/*
						** simulate shift of "error"
						*/
						date_Date_state = date_Dateact[ date_Date_n ];
						goto date_Date_stack;
					}
					/*
					** current state has no shift on
					** "error", pop stack
					*/
#if YYDEBUG
#	define _POP_ "Error recovery pops state %d, uncovers state %d\n"
					if ( date_Datedebug )
						printf( _POP_, *date_Date_ps,
							date_Date_ps[-1] );
#	undef _POP_
#endif
					date_Date_ps--;
					date_Date_pv--;
				}
				/*
				** there is no state on stack with "error" as
				** a valid shift.  give up.
				*/
				YYABORT;
			case 3:		/* no shift yet; eat a token */
#if YYDEBUG
				/*
				** if debugging, look up token in list of
				** pairs.  0 and negative shouldn't occur,
				** but since timing doesn't matter when
				** debugging, it doesn't hurt to leave the
				** tests here.
				*/
				if ( date_Datedebug )
				{
					register int date_Date_i;

					printf( "Error recovery discards " );
					if ( date_Datechar == 0 )
						printf( "token end-of-file\n" );
					else if ( date_Datechar < 0 )
						printf( "token -none-\n" );
					else
					{
						for ( date_Date_i = 0;
							date_Datetoks[date_Date_i].t_val >= 0;
							date_Date_i++ )
						{
							if ( date_Datetoks[date_Date_i].t_val
								== date_Datechar )
							{
								break;
							}
						}
						printf( "token %s\n",
							date_Datetoks[date_Date_i].t_name );
					}
				}
#endif /* YYDEBUG */
				if ( date_Datechar == 0 )	/* reached EOF. quit */
					YYABORT;
				date_Datechar = -1;
				goto date_Date_newstate;
			}
		}/* end if ( date_Date_n == 0 ) */
		/*
		** reduction by production date_Date_n
		** put stack tops, etc. so things right after switch
		*/
#if YYDEBUG
		/*
		** if debugging, print the string that is the user's
		** specification of the reduction which is just about
		** to be done.
		*/
		if ( date_Datedebug )
			printf( "Reduce by (%d) \"%s\"\n",
				date_Date_n, date_Datereds[ date_Date_n ] );
#endif
		date_Datetmp = date_Date_n;			/* value to switch over */
		date_Datepvt = date_Date_pv;			/* $vars top of value stack */
		/*
		** Look in goto table for next state
		** Sorry about using date_Date_state here as temporary
		** register variable, but why not, if it works...
		** If date_Dater2[ date_Date_n ] doesn't have the low order bit
		** set, then there is no action to be done for
		** this reduction.  So, no saving & unsaving of
		** registers done.  The only difference between the
		** code just after the if and the body of the if is
		** the goto date_Date_stack in the body.  This way the test
		** can be made before the choice of what to do is needed.
		*/
		{
			/* length of production doubled with extra bit */
			register int date_Date_len = date_Dater2[ date_Date_n ];

			if ( !( date_Date_len & 01 ) )
			{
				date_Date_len >>= 1;
				date_Dateval = ( date_Date_pv -= date_Date_len )[1];	/* $$ = $1 */
				date_Date_state = date_Datepgo[ date_Date_n = date_Dater1[ date_Date_n ] ] +
					*( date_Date_ps -= date_Date_len ) + 1;
				if ( date_Date_state >= YYLAST ||
					date_Datechk[ date_Date_state =
					date_Dateact[ date_Date_state ] ] != -date_Date_n )
				{
					date_Date_state = date_Dateact[ date_Datepgo[ date_Date_n ] ];
				}
				goto date_Date_stack;
			}
			date_Date_len >>= 1;
			date_Dateval = ( date_Date_pv -= date_Date_len )[1];	/* $$ = $1 */
			date_Date_state = date_Datepgo[ date_Date_n = date_Dater1[ date_Date_n ] ] +
				*( date_Date_ps -= date_Date_len ) + 1;
			if ( date_Date_state >= YYLAST ||
				date_Datechk[ date_Date_state = date_Dateact[ date_Date_state ] ] != -date_Date_n )
			{
				date_Date_state = date_Dateact[ date_Datepgo[ date_Date_n ] ];
			}
		}
					/* save until reenter driver code */
		date_Datestate = date_Date_state;
		date_Dateps = date_Date_ps;
		date_Datepv = date_Date_pv;
	}
	/*
	** code supplied by user is placed in this switch
	*/
	switch( date_Datetmp )
	{
		
case 3:{
            date_DateHaveTime++;
        } break;
case 4:{
            date_DateHaveZone++;
        } break;
case 5:{
            date_DateHaveDate++;
        } break;
case 6:{
            date_DateHaveOrdinalMonth++;
        } break;
case 7:{
            date_DateHaveDay++;
        } break;
case 8:{
            date_DateHaveRel++;
        } break;
case 9:{
	    date_DateHaveTime++;
	    date_DateHaveDate++;
	} break;
case 10:{
	    date_DateHaveTime++;
	    date_DateHaveDate++;
	    date_DateHaveRel++;
        } break;
case 12:{
            date_DateHour = date_Datepvt[-1].Number;
            date_DateMinutes = 0;
            date_DateSeconds = 0;
            date_DateMeridian = date_Datepvt[-0].Meridian;
        } break;
case 13:{
            date_DateHour = date_Datepvt[-3].Number;
            date_DateMinutes = date_Datepvt[-1].Number;
            date_DateSeconds = 0;
            date_DateMeridian = date_Datepvt[-0].Meridian;
        } break;
case 14:{
            date_DateHour = date_Datepvt[-4].Number;
            date_DateMinutes = date_Datepvt[-2].Number;
            date_DateMeridian = MER24;
            date_DateDSTmode = DSToff;
            date_DateTimezone = (date_Datepvt[-0].Number % 100 + (date_Datepvt[-0].Number / 100) * 60);
        } break;
case 15:{
            date_DateHour = date_Datepvt[-5].Number;
            date_DateMinutes = date_Datepvt[-3].Number;
            date_DateSeconds = date_Datepvt[-1].Number;
            date_DateMeridian = date_Datepvt[-0].Meridian;
        } break;
case 16:{
            date_DateHour = date_Datepvt[-6].Number;
            date_DateMinutes = date_Datepvt[-4].Number;
            date_DateSeconds = date_Datepvt[-2].Number;
            date_DateMeridian = MER24;
            date_DateDSTmode = DSToff;
            date_DateTimezone = (date_Datepvt[-0].Number % 100 + (date_Datepvt[-0].Number / 100) * 60);
        } break;
case 17:{
            date_DateTimezone = date_Datepvt[-1].Number;
            date_DateDSTmode = DSTon;
        } break;
case 18:{
            date_DateTimezone = date_Datepvt[-0].Number;
            date_DateDSTmode = DSToff;
        } break;
case 19:{
            date_DateTimezone = date_Datepvt[-0].Number;
            date_DateDSTmode = DSTon;
        } break;
case 20:{
            date_DateDayOrdinal = 1;
            date_DateDayNumber = date_Datepvt[-0].Number;
        } break;
case 21:{
            date_DateDayOrdinal = 1;
            date_DateDayNumber = date_Datepvt[-1].Number;
        } break;
case 22:{
            date_DateDayOrdinal = date_Datepvt[-1].Number;
            date_DateDayNumber = date_Datepvt[-0].Number;
        } break;
case 23:{
            date_DateDayOrdinal = date_Datepvt[-2].Number * date_Datepvt[-1].Number;
            date_DateDayNumber = date_Datepvt[-0].Number;
        } break;
case 24:{
            date_DateDayOrdinal = 2;
            date_DateDayNumber = date_Datepvt[-0].Number;
        } break;
case 25:{
            date_DateMonth = date_Datepvt[-2].Number;
            date_DateDay = date_Datepvt[-0].Number;
        } break;
case 26:{
            date_DateMonth = date_Datepvt[-4].Number;
            date_DateDay = date_Datepvt[-2].Number;
            date_DateYear = date_Datepvt[-0].Number;
        } break;
case 27:{
	    date_DateYear = date_Datepvt[-0].Number / 10000;
	    date_DateMonth = (date_Datepvt[-0].Number % 10000)/100;
	    date_DateDay = date_Datepvt[-0].Number % 100;
	} break;
case 28:{
	    date_DateDay = date_Datepvt[-4].Number;
	    date_DateMonth = date_Datepvt[-2].Number;
	    date_DateYear = date_Datepvt[-0].Number;
	} break;
case 29:{
            date_DateMonth = date_Datepvt[-2].Number;
            date_DateDay = date_Datepvt[-0].Number;
            date_DateYear = date_Datepvt[-4].Number;
        } break;
case 30:{
            date_DateMonth = date_Datepvt[-1].Number;
            date_DateDay = date_Datepvt[-0].Number;
        } break;
case 31:{
            date_DateMonth = date_Datepvt[-3].Number;
            date_DateDay = date_Datepvt[-2].Number;
            date_DateYear = date_Datepvt[-0].Number;
        } break;
case 32:{
            date_DateMonth = date_Datepvt[-0].Number;
            date_DateDay = date_Datepvt[-1].Number;
        } break;
case 33:{
	    date_DateMonth = 1;
	    date_DateDay = 1;
	    date_DateYear = EPOCH;
	} break;
case 34:{
            date_DateMonth = date_Datepvt[-1].Number;
            date_DateDay = date_Datepvt[-2].Number;
            date_DateYear = date_Datepvt[-0].Number;
        } break;
case 35:{
	    date_DateMonthOrdinal = 1;
	    date_DateMonth = date_Datepvt[-0].Number;
	} break;
case 36:{
	    date_DateMonthOrdinal = date_Datepvt[-1].Number;
	    date_DateMonth = date_Datepvt[-0].Number;
	} break;
case 37:{
            if (date_Datepvt[-1].Number != HOUR(- 7)) YYABORT;
	    date_DateYear = date_Datepvt[-2].Number / 10000;
	    date_DateMonth = (date_Datepvt[-2].Number % 10000)/100;
	    date_DateDay = date_Datepvt[-2].Number % 100;
	    date_DateHour = date_Datepvt[-0].Number / 10000;
	    date_DateMinutes = (date_Datepvt[-0].Number % 10000)/100;
	    date_DateSeconds = date_Datepvt[-0].Number % 100;
        } break;
case 38:{
            if (date_Datepvt[-5].Number != HOUR(- 7)) YYABORT;
	    date_DateYear = date_Datepvt[-6].Number / 10000;
	    date_DateMonth = (date_Datepvt[-6].Number % 10000)/100;
	    date_DateDay = date_Datepvt[-6].Number % 100;
	    date_DateHour = date_Datepvt[-4].Number;
	    date_DateMinutes = date_Datepvt[-2].Number;
	    date_DateSeconds = date_Datepvt[-0].Number;
        } break;
case 39:{
	    date_DateYear = date_Datepvt[-1].Number / 10000;
	    date_DateMonth = (date_Datepvt[-1].Number % 10000)/100;
	    date_DateDay = date_Datepvt[-1].Number % 100;
	    date_DateHour = date_Datepvt[-0].Number / 10000;
	    date_DateMinutes = (date_Datepvt[-0].Number % 10000)/100;
	    date_DateSeconds = date_Datepvt[-0].Number % 100;
        } break;
case 40:{
            /*
	     * Offset computed year by -377 so that the returned years will
	     * be in a range accessible with a 32 bit clock seconds value
	     */
            date_DateYear = date_Datepvt[-2].Number/1000 + 2323 - 377;
            date_DateDay  = 1;
	    date_DateMonth = 1;
	    date_DateRelDay += ((date_Datepvt[-2].Number%1000)*(365 + IsLeapYear(date_DateYear)))/1000;
	    date_DateRelSeconds += date_Datepvt[-0].Number * 144 * 60;
        } break;
case 41:{
	    date_DateRelSeconds *= -1;
	    date_DateRelMonth *= -1;
	    date_DateRelDay *= -1;
	} break;
case 43:{ *date_DateRelPointer += date_Datepvt[-2].Number * date_Datepvt[-1].Number * date_Datepvt[-0].Number; } break;
case 44:{ *date_DateRelPointer += date_Datepvt[-1].Number * date_Datepvt[-0].Number; } break;
case 45:{ *date_DateRelPointer += date_Datepvt[-0].Number; } break;
case 46:{ *date_DateRelPointer += date_Datepvt[-1].Number * date_Datepvt[-0].Number; } break;
case 47:{ *date_DateRelPointer += date_Datepvt[-0].Number; } break;
case 48:{ date_Dateval.Number = -1; } break;
case 49:{ date_Dateval.Number =  1; } break;
case 50:{ date_Dateval.Number = date_Datepvt[-0].Number; date_DateRelPointer = &date_DateRelSeconds; } break;
case 51:{ date_Dateval.Number = date_Datepvt[-0].Number; date_DateRelPointer = &date_DateRelDay; } break;
case 52:{ date_Dateval.Number = date_Datepvt[-0].Number; date_DateRelPointer = &date_DateRelMonth; } break;
case 53:{
	if (date_DateHaveTime && date_DateHaveDate && !date_DateHaveRel) {
	    date_DateYear = date_Datepvt[-0].Number;
	} else {
	    date_DateHaveTime++;
	    if (date_Datepvt[-0].Number < 100) {
		date_DateHour = date_Datepvt[-0].Number;
		date_DateMinutes = 0;
	    } else {
		date_DateHour = date_Datepvt[-0].Number / 100;
		date_DateMinutes = date_Datepvt[-0].Number % 100;
	    }
	    date_DateSeconds = 0;
	    date_DateMeridian = MER24;
	}
    } break;
case 54:{
            date_Dateval.Meridian = MER24;
        } break;
case 55:{
            date_Dateval.Meridian = date_Datepvt[-0].Meridian;
        } break;
	}
	goto date_Datestack;		/* reset registers in driver code */
}

