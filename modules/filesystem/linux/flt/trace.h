#ifndef D14FTL_DBG_H_
#define D14FTL_DBG_H_

/* need to define flexible messages levels and cathegories */

#define d14flt_mark() { \
	printk(KERN_DEBUG "d14flt: %s() MARK\n",  __FUNCTION__); \
}

#define d14flt_debug(format, args...) { \
	printk(KERN_DEBUG "d14flt: %s() " format "\n",  __FUNCTION__, ## args); \
}

#define d14flt_debug_if(cond, format, args...) { \
	if (cond) \
		printk(KERN_DEBUG "d14flt: %s() " format "\n",  __FUNCTION__, ## args); \
}

#define d14flt_err(format, args...) { \
	printk(KERN_ERR "d14flt: %s() " format "\n",  __FUNCTION__, ## args); \
}

#endif /*D14FTL_DBG_H_*/
