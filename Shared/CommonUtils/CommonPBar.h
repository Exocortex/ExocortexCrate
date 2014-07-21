#ifndef __COMMON_PBAR_H
#define __COMMON_PBAR_H

	#include <string>

	class CommonProgressBar
	{
	public:
		inline void init(int range) { init(0, range, 1); }
		virtual void init(int min, int max, int incr) = 0;
		virtual void start(void) = 0;
		virtual void stop(void) = 0;
		virtual void incr(int step=1) = 0;
		virtual bool isCancelled(void) = 0;
		virtual void setCaption(std::string& caption){}
		virtual int getUpdateCount() const{ return 20; }
	};

#endif
