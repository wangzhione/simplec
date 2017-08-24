#include <scrwlock.h>

// add read lock
void 
rwlock_rlock(struct rwlock * lock) {
	for (;;) {
		// 看是否有人在试图读, 得到并防止代码位置优化
		while (lock->wlock)
			ATOM_SYNC();

		ATOM_INC(lock->rlock);
		// 没有写占用, 开始读了
		if (!lock->wlock)
			break;

		// 还是有写, 删掉添加的读
		ATOM_DEC(lock->rlock);
	}
}

// add write lock
void 
rwlock_wlock(struct rwlock * lock) {
	ATOM_LOCK(lock->wlock);
	// 等待读占用锁
	while (lock->rlock)
		ATOM_SYNC();
}

// unlock write lock
inline void 
rwlock_unwlock(struct rwlock * lock) {
	ATOM_UNLOCK(lock->wlock);
}

// unlock read lock
inline void 
rwlock_unrlock(struct rwlock * lock) {
	ATOM_DEC(lock->rlock);
}