#ifndef CTIMEOUT_H
#define CTIMEOUT_H

class CTimeout
{
	public:
		CTimeout() : timeout(0) {}
		CTimeout(int pTimeout) : timeout(pTimeout) {}

		void setTimeout(int pTimeout)		{ timeout = pTimeout; }
		int getTimeout() const				{ return timeout; }
		int doTimeout()						{ return (timeout > 0 ? --timeout : -1); }

	private:
		int timeout;
};

#endif
