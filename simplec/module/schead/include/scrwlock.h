#ifndef _H_SIMPLEC_SCRWLOCK
#define _H_SIMPLEC_SCRWLOCK

#include <scatom.h>

/*
 * 这里构建 simple write and read lock
 * struct rwlock need zero.
 * is scatom ext
 */

// init need all is 0
struct rwlock {
	int rlock;
	int wlock;
};

// add read lock
extern void rwlock_rlock(struct rwlock * lock);
// add write lock
extern void rwlock_wlock(struct rwlock * lock);

// add write lock
extern void rwlock_runlock(struct rwlock * lock);
// unlock write
extern void rwlock_wunlock(struct rwlock * lock);

#endif // !_H_SIMPLEC_SCRWLOCK