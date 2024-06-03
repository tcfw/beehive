#include <kernel/arch.h>
#include <kernel/devicetree.h>
#include <kernel/clock.h>
#include <kernel/cls.h>
#include <kernel/context.h>
#include <kernel/irq.h>
#include <kernel/list.h>
#include <kernel/mm.h>
#include <kernel/modules.h>
#include <kernel/panic.h>
#include <kernel/sched.h>
#include <kernel/skiplist.h>
#include <kernel/strings.h>
#include <kernel/sync.h>
#include <kernel/thread.h>
#include <kernel/wait.h>

static LIST_HEAD(pending);

static spinlock_t pending_lock;

static sched_class_t *sched_class_head;

static double prio_ratios[MAX_PRIO];

static int thread_deadline_comparator(void *n1, void *n2)
{

	if (n2 == NULL || n1 == NULL)
		return 1;

	thread_t *tn1 = (thread_t *)n1;
	thread_t *tn2 = (thread_t *)n2;

	if (tn1->process->pid == tn2->process->pid)
		return 0;

	uint64_t tn1e = tn1->sched_entity.deadline;
	uint64_t tn2e = tn2->sched_entity.deadline;

	if (tn1e == tn2e)
		return 0;
	else if (tn1e > tn2e)
		return 1;
	else
		return -1;
}

void sched_local_init(void)
{
	cls_t *cls = get_cls();
	spinlock_init(&cls->rq.lock);

	skl_init(&cls->rq.lrf, SKIPLIST_DEFAULT_LEVELS, thread_deadline_comparator, 0);
	INIT_WAITQUEUE(&cls->sleepq);
}

void schedule_start(void)
{
	cls_t *cls = get_cls();
	struct clocksource_t *cs = clock_first(CS_GLOBAL);

	spinlock_acquire(&cls->rq.lock);

	// check for any pending threads and claim it
	thread_t *t = sched_get_pending(sched_affinity(cls->id));
	if (t)
		t->sched_class->enqueue_thread(&cls->rq, t);

	sched_class_t *sc = sched_class_head;
	thread_t *next;
	while (sc)
	{
		next = sc->next_thread(&cls->rq);
		sc = sc->next;
		if (next)
		{
			// Set next tick
			uint64_t freq = cs->getFreq(cs);
			cs->countNTicks(cs, freq / 250);
			cs->enableIRQ(cs);
			cs->enable(cs);

			spinlock_release(&cls->rq.lock);
			set_current_thread(next);
			arch_thread_prep_switch(next);
			vm_set_table(next->process->vm.vm_table, next->process->pid);
			switch_to_context(&next->ctx);
			return;
		}
	}

	panic("schedule_start should always get a task");
}

thread_t *sched_get_pending(uint64_t affinity)
{
	int state = spinlock_acquire_irq(&pending_lock);

	if (list_empty(pending))
	{
		spinlock_release_irq(state, &pending_lock);
		return 0;
	}

	struct thread_list_entry_t *this;
	struct thread_list_entry_t *tmp;
	list_head_for_each_safe(this, tmp, &pending)
	{
		if ((this->thread->affinity & affinity) != 0)
		{
			thread_t *thread = this->thread;
			list_del((struct list_head *)this);
			kfree(this);
			spinlock_release_irq(state, &pending_lock);
			return thread;
		}
	}

	spinlock_release_irq(state, &pending_lock);
	return 0;
}

void sched_append_pending(thread_t *thread)
{
	int state = spinlock_acquire_irq(&pending_lock);

	struct thread_list_entry_t *entry = kmalloc(sizeof(struct thread_list_entry_t));
	entry->thread = thread;

	list_add_tail(&entry->list, &pending);

	spinlock_release_irq(state, &pending_lock);
}

uint64_t sched_affinity(uint64_t cpu_id)
{
	return 1 << cpu_id;
}

static inline int sched_should_tick(sched_rq_t *rq)
{
	struct clocksource_t *cs = clock_first(CS_GLOBAL);
	uint64_t v = cs->val(cs);
	return v - rq->last_tick > SCHED_MIN_TICK_DURATION;
}

sched_class_t *sched_get_class(enum Sched_Classes class)
{
	sched_class_t *cs = sched_class_head;
	while (cs != NULL)
	{
		if (cs->class == class)
			return cs;
		cs = cs->next;
	}

	return NULL;
}

int thread_is_running(thread_t *thread)
{
	cls_t *cur;
	int cc = devicetree_count_dev_type("cpu");
	for (int c = 0; c < cc; c++)
	{
		cur = get_core_cls(c);

		spinlock_acquire(&cur->rq.lock);

		if (cur->rq.current_thread == thread)
		{
			spinlock_release(&cur->rq.lock);
			return c;
		}

		spinlock_release(&cur->rq.lock);
	}

	return -1;
}

void thread_stop_core(unsigned int code)
{
	schedule();
}

void schedule(void)
{
	cls_t *cls = get_cls();

	int state = spinlock_acquire_irq(&cls->rq.lock);

	try_wake_waitqueue(&cls->sleepq);

	struct clocksource_t *clk = clock_first(CS_GLOBAL);
	uint64_t clkval = clk->val(clk);

	// check for any pending threads and claim it
	thread_t *p = sched_get_pending(sched_affinity(cls->id));
	if (p)
		p->sched_class->enqueue_thread(&cls->rq, p);

	thread_t *prev = cls->rq.current_thread;

	prev->timing.total_user += clkval - prev->timing.last_user;
	prev->timing.total_execution = prev->timing.total_system + prev->timing.total_user;
	prev->sched_entity.deadline -= clkval - prev->sched_entity.last_deadline;

	if (prev->state == THREAD_RUNNING)
		prev->sched_class->requeue_thread(&cls->rq, prev);
	else
		prev->sched_class->dequeue_thread(&cls->rq, prev);

	sched_class_t *sc = sched_class_head;
	if (sched_should_tick(&cls->rq))
	{
		while (sc != NULL)
		{
			if (sc->tick)
				sc->tick(&cls->rq);
			else
				sc = sc->next;
		}
	}

	sc = sched_class_head;
	thread_t *next;
	while (sc != NULL)
	{
		next = sc->next_thread(&cls->rq);
		sc = sc->next;
		if (next)
		{
			spinlock_release_irq(state, &cls->rq.lock);
			if (next == prev)
				return;

			prev->timing.last_wait = clkval;
			next->timing.last_user = clkval;
			next->timing.total_wait += clkval - next->timing.last_wait;
			next->sched_entity.last_deadline = clkval;

			cls->rq.current_thread = next;
			next->running_core = cls->id;
			arch_thread_prep_switch(next);
			return;
		}
	}

	spinlock_release_irq(state, &cls->rq.lock);
	panic("schedule had nothing to do");
}

void lrf_enqueue_thread(sched_rq_t *rq, thread_t *thread)
{
	skl_insert(&rq->lrf, thread);
}

void lrf_dequeue_thread(sched_rq_t *rq, thread_t *thread)
{
	skl_delete(&rq->lrf, thread);
}

void lrf_requeue_thread(sched_rq_t *rq, thread_t *thread)
{
	if (thread->sched_entity.deadline <= 0)
	{
		struct clocksource_t *cs = clock_first(CS_GLOBAL);
		thread->sched_entity.deadline = cs->val(cs) + prio_ratios[thread->sched_entity.prio];
	}

	skl_insert(&rq->lrf, thread);
}

thread_t *lrf_next_thread(sched_rq_t *rq)
{
	return skl_pull_first(&rq->lrf);
}

sched_class_t lrf = {
	.class = SCHED_CLASS_LRF,
	.enqueue_thread = lrf_enqueue_thread,
	.dequeue_thread = lrf_dequeue_thread,
	.requeue_thread = lrf_requeue_thread,
	.next_thread = lrf_next_thread,
};

static void idle_noop(sched_rq_t *rq, thread_t *thread) {}

static void wait_kthread()
{
	while (1)
		wait_task();
}

sched_class_t idle;

static thread_t *idle_next_task(sched_rq_t *rq)
{
	thread_t *idle_thread = rq->idle;
	if (!idle_thread)
	{
		char *buf = kmalloc(64);
		// ksprintf(buf, "[idle 0x%x]", get_cls()->id);
		ksprintf(buf, "[idle]");
		idle_thread = create_kthread(&wait_kthread, buf, 0);
		kfree(buf);

		idle_thread->running_core = get_cls()->id;
		idle_thread->sched_class = &idle;
		rq->idle = idle_thread;
	}

	return idle_thread;
}

sched_class_t idle = {
	.class = SCHED_CLASS_IDLE,
	.enqueue_thread = idle_noop,
	.dequeue_thread = idle_noop,
	.requeue_thread = idle_noop,
	.next_thread = idle_next_task,
};

void sched_init(void)
{
	spinlock_init(&pending_lock);
	INIT_LIST_HEAD(&pending);

	lrf.next = &idle;
	sched_class_head = &lrf;

	prio_ratios[0] = PRIO_INTERVAL_BASE;

	for (int i = 1; i < MAX_PRIO; i++)
	{
		prio_ratios[i] = prio_ratios[i - 1] * 11 / 10;
	}
}