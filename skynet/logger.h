#ifndef BEEHIVE_H__
#define BEEHIVE_H__

#include <strings.h>
#include <zlog.h>

/**
 * define logger macro
 */
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define DEBUG 0

// print message detail
#ifdef DETAIL_MESSAGE 
#define info(fmt, args...) \
    do {\
        fprintf(stderr, "[info][%s:%d:%s()] " fmt "\n", __FILENAME__, __LINE__, __func__, ##args);\
    } while(0)

#define debug(fmt, args...) \
    do {\
        if (DEBUG) fprintf(stderr, "[debug][%s:%d:%s()] " fmt "\n", __FILENAME__, __LINE__, __func__, ##args);\
    } while(0)

#define error(fmt, args...) \
    do {\
        fprintf(stderr, "[error][%s:%d:%s()] " fmt "\n", __FILENAME__, __LINE__, __func__, ##args);\
    } while(0)
#endif

// print simple message
#ifdef SIMPLE_MESSAGE 
#define info(fmt, args...) \
    do {\
        fprintf(stderr, "[info] " fmt "\n", ##args);\
    } while(0)

#define debug(fmt, args...) \
    do {\
        if (DEBUG) fprintf(stderr, "[debug] " fmt "\n", ##args);\
    } while(0)

#define error(fmt, args...) \
    do {\
        fprintf(stderr, "[error] " fmt "\n", ##args);\
    } while(0)
#endif

// using zlog lib
#ifdef ZLOG_MESSAGE
#define info dzlog_info
#define debug dzlog_debug
#define error dzlog_error
#endif

#endif