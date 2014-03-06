#ifndef __TARGET_STRING__
#  error "You must define __TARGET_STRING__"
#endif

#if defined(__LED_V1__)
    const char *__target_build = "LED_V1 " __TARGET_STRING__ " " __DATE__" "__TIME__;
#else
   	const char *__target_build = "LED " __TARGET_STRING__ " " __DATE__" "__TIME__;
#endif


const char *Version(void) {
	return __target_build;
}
