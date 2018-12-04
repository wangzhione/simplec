#ifndef _H_SIMPLEC_SCATOM
#define _H_SIMPLEC_SCATOM

/*
 * 作者 : wz
 * 描述 : 简单的原子操作,目前只考虑 VS(CL) 和 gcc
 */

// 如果 是 VS 编译器
#if defined(_MSC_VER)

#include <windows.h>

#pragma warning(push)
//忽略 warning C4047: “==”:“void *”与“LONG”的间接级别不同
#pragma warning(disable:4047)

// v 和 a 都是 long 这样数据
#define ATOM_ADD(v, a)      InterlockedAdd((LONG volatile *)&(v), (LONG)(a))
#define ATOM_SET(v, a)      InterlockedExchange((LONG volatile *)&(v), (LONG)(a))
#define ATOM_AND(v, a)      (InterlockedAnd((volatile LONG *)&(v), (LONG)(a)), (LONG)(v))
#define ATOM_INC(v)         InterlockedIncrement((LONG volatile *)&(v))
#define ATOM_DEC(v)         InterlockedDecrement((LONG volatile *)&(v))
//
// 对于 InterlockedCompareExchange(v, c, a) 等价于下面
// long tmp = v ; v == a ? v = c : ; return tmp;
//
// 咱们的 ATOM_CAS(v, c, a) 等价于下面
// long tmp = v ; v == c ? v = a : ; return tmp;
//
#define ATOM_CAS(v, c, a)   ((LONG)(c) == InterlockedCompareExchange((LONG volatile *)&(v), (LONG)(a), (LONG)(c)))

#pragma warning(pop)

// 保证代码不乱序优化后执行
#define ATOM_SYNC()         MemoryBarrier()

#define ATOM_UNLOCK(v)      ATOM_SET(v, 0)

// 否则 如果是 gcc 编译器
#elif defined(__GNUC__)

// v += a ; return v;
#define ATOM_ADD(v, a)      __sync_add_and_fetch(&(v), (a))
// type tmp = v ; v = a; return tmp;
#define ATOM_SET(v, a)      __sync_lock_test_and_set(&(v), (a))
// v &= a; return v;
#define ATOM_AND(v, a)      __sync_and_and_fetch(&(v), (a))
// return ++v;
#define ATOM_INC(v)         __sync_add_and_fetch(&(v), 1)
// return --v;
#define ATOM_DEC(v)         __sync_sub_and_fetch(&(v), 1)
// bool b = v == c; b ? v=a : ; return b;
#define ATOM_CAS(v, c, a)   __sync_bool_compare_and_swap(&(v), (c), (a))

 // 保证代码不乱序
#define ATOM_SYNC()         __sync_synchronize()

// 对ATOM_LOCK 解锁, 当然 直接调用相当于 v = 0;
#define ATOM_UNLOCK(v)      __sync_lock_release(&(v))

#endif // !_MSC_VER && !__GNUC__

/*
 * 试图加锁, 用法举例
 
	 if(ATOM_TRYLOCK(v)) {
		 // 已经有人加锁了, 处理返回事件
		...
	 }
 
	 // 得到锁资源, 开始处理
	 ...
 
	 ATOM_UNLOCK(v);
 
 * 返回1表示已经有人加锁了, 竞争锁失败.
 * 返回0表示得到锁资源, 竞争锁成功
 */
#define ATOM_TRYLOCK(v)     (!ATOM_SET(v, 1))

//
// 使用方式:
//  int lock = 0;
//  ATOM_LOCK(lock);
//  ...
//  ATOM_UNLOCK(lock);
//
#define ATOM_LOCK(v)        while(ATOM_SET(v, 1))

#endif // !_H_SIMPLEC_SCATOM