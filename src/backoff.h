#ifndef TICLE_BACKOFF_H
#define TICLE_BACKOFF_H

#define TICLE_BACKOFF_MAX_SECONDS 30U

unsigned int ticle_backoff_seconds(unsigned int attempt);

#endif
