#include <mq.h>
#include <schead.h>
#include <pthread.h>
#include <scrunloop.h>

// 具体轮询器
struct srl {
	mq_t mq;				// 消息队列
	pthread_t th;			// 具体奔跑的线程
	die_f run;				// 每个消息都会调用 run(pop())
	volatile bool loop;		// true表示还在继续 
};

// 线程阻塞的毫秒数
#define _INT_SCRUNLOOP		(1)

void * _srl_loop(void * arg) {
	struct srl * s = arg;
	
	while (s->loop) {
		void * pop = mq_pop(s->mq);
		if (NULL == pop) {
			sh_sleep(_INT_SCRUNLOOP);
			continue;
		}

		// 开始处理消息
		s->run(pop);
	}

	return arg;
}

//
// srl_create - 创建轮询服务对象
// run		: 轮序处理每条消息体, 弹出消息体的时候执行
// return	: void 
//
srl_t
srl_create(die_f run) {
	struct srl * s = malloc(sizeof(struct srl));
	assert(s && run);
	s->mq = mq_create();
	s->loop = true;
	s->run = run;
	// 创建线程,并启动
	if (pthread_create(&s->th, NULL, _srl_loop, s)) {
		mq_delete(s->mq);
		free(s);
		CERR("pthread_create create error !!!");
		return NULL;
	}
	return s;
}

//
// srl_delete - 销毁轮询对象,回收资源
// srl		: 轮询对象
// return	: void 
//
void
srl_delete(srl_t srl) {
	if (srl) {
		srl->loop = false;
		// 等待线程结束, 然后退出
		pthread_join(srl->th, NULL);
		mq_delete(srl->mq);
		free(srl);
	}
}

//
// srl_push - 将消息压入到轮询器中
// msg		: 待加入的消息地址
// return	: void
// 
inline void 
srl_push(srl_t srl, void * msg) {
	assert(srl && msg);
	mq_push(srl->mq, msg);
}