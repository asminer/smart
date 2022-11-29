
/**
    A centralized object for systemy stuff
    like output, error, reporting streams,
    catching signals, exiting cleanly.
*/

#ifndef SIGMAN_H
#define SIGMAN_H

class signal_manager {
        /*
         * Centralized signal catching mechanism
         */
        int sigx;

        /*
         * Should we wait to terminate (to process a signal)
         */
        bool wait_to_terminate;

        signal_manager();
        ~signal_manager();

    public:

        /*
         * Call this to indicate that termination due to
         * a caught signal should be delayed for cleanup first.
         */
        inline void waitTermination() { wait_to_terminate = true; }

        /*
         * What was the last signal raised,
         * in case some computation needs to stop early.
         */
        inline int caughtSignal() const { return sigx; }

        /*
         * Our registered signal catcher will call this.
         */
        inline void raiseSignal(int s) {
            if (sigx) return;   // already processing a signal
            sigx = s;
            if (!wait_to_terminate) resumeTermination();
        }

        /*
         * Call this to indicate that signals should cause
         * immediate termination.  If any signal has been
         * caught since a call to waitTermination(),
         * we will terminate now.
         */
        void resumeTermination();

        /*
         * Clean exit, either due to an internal error,
         * caught signal, user function call, or end of input.
         */
        static void clean_exit(int code);

        static signal_manager& theSigMan();

    private:


};


#endif

