#include "backoff.h"

unsigned int ticle_backoff_seconds(unsigned int attempt) {
    unsigned int seconds = 1U;
    while (attempt > 0 && seconds < TICLE_BACKOFF_MAX_SECONDS) {
        if (seconds > TICLE_BACKOFF_MAX_SECONDS / 2U)
            return TICLE_BACKOFF_MAX_SECONDS;
        seconds *= 2U;
        --attempt;
    }
    return seconds > TICLE_BACKOFF_MAX_SECONDS ? TICLE_BACKOFF_MAX_SECONDS : seconds;
}
