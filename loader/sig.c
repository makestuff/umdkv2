#include <makestuff.h>
#ifdef WIN32
	#include <windows.h>
#else
	#define _POSIX_SOURCE
	#include <signal.h>
#endif

static bool m_sigint = false;

bool sigIsRaised(void) {
	return m_sigint;
}

#ifdef WIN32
	static BOOL sigHandler(DWORD signum) {
		if ( signum == CTRL_C_EVENT ) {
			m_sigint = true;
			return TRUE;
		}
		return FALSE;
	}
#else
	static void sigHandler(int signum) {
		(void)signum;
		m_sigint = true;
	}
#endif

void sigRegisterHandler(void) {
	#ifdef WIN32
		SetConsoleCtrlHandler((PHANDLER_ROUTINE)sigHandler, TRUE);
	#else
		struct sigaction newAction, oldAction;
		newAction.sa_handler = sigHandler;
		sigemptyset(&newAction.sa_mask);
		newAction.sa_flags = 0;
		sigaction(SIGINT, NULL, &oldAction);
		if ( oldAction.sa_handler != SIG_IGN ) {
			sigaction(SIGINT, &newAction, NULL);
		}
	#endif
}
