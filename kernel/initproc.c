#include <elf_arch.h>
#include <errno.h>
#include <kernel/elf.h>
#include <kernel/initproc.h>
#include <kernel/mm.h>
#include <kernel/sched.h>
#include <kernel/strings.h>
#include <kernel/thread.h>
#include <kernel/tty.h>
#include <kernel/vm.h>

int load_initproc()
{
	struct elf_hdr_64_t *initproc = initproc_elf_hdr();

	if (!validate_elf(initproc))
		return -ERRINVAL;

	process_t *proc = (process_t *)page_alloc_s(sizeof(process_t));
	if (!proc)
		return -ERRNOMEM;
	memset(proc, 0, sizeof(*proc));

	init_proc(proc, "init");

	proc->pid = 1;

	proc->uid = 0;
	proc->euid = 0;
	proc->gid = 0;
	proc->egid = 0;

	int ret = load_elf(&proc->vm, initproc);
	if (ret < 0)
		goto free_proc_ret;

	void *stack = (void *)page_alloc_s((size_t)USER_STACK_SIZE);
	if (!stack)
	{
		ret = -ERRNOMEM;
		goto free_proc_ret;
	}

	uintptr_t stack_paddr = vm_va_to_pa(vm_get_current_table(), (uintptr_t)stack);

	// TODO(tcfw) stack address randomization
	uintptr_t stack_vaddr_top = USER_STACK_BASE;
	uintptr_t stack_vaddr_bottom = USER_STACK_BASE - USER_STACK_SIZE;
	int stack_flags = MEMORY_TYPE_USER | MEMORY_NON_EXEC | MEMORY_PERM_W;
	vm_map_region(proc->vm.vm_table, stack_paddr, stack_vaddr_bottom, USER_STACK_SIZE - 1, stack_flags);

	vm_mapping *map = kmalloc(sizeof(vm_mapping));
	if (!map)
	{
		ret = -ERRNOMEM;
		goto free_proc_ret;
	}
	map->flags = VM_MAP_FLAG_PHY_KERNEL;
	map->phy_addr = stack_paddr;
	map->vm_addr = stack_vaddr_top;
	map->length = USER_STACK_SIZE;
	map->page = (struct page *)stack;
	list_add(&map->list, &proc->vm.vm_maps);

	// TODO(tcfw) load VDSO

	proc->vm.start_stack = stack_vaddr_top;

	thread_t *thread = (thread_t *)page_alloc_s(sizeof(thread_t));
	if (!thread)
	{
		ret = -ERRNOMEM;
		goto free_proc_ret;
	}

	thread->process = proc;
	init_thread(thread);
	thread->ctx.pc = initproc->e_entry;
	thread->ctx.sp = stack_vaddr_top - 0xA0;
	thread_enable_single_step(&thread->ctx);

	thread_list_entry_t *tle = kmalloc(sizeof(thread_list_entry_t));
	if (!tle)
	{
		ret = -ERRNOMEM;
		goto free_proc_ret;
	}

	tle->thread = thread;
	list_add(&tle->list, &proc->threads);

	thread->state = THREAD_RUNNING;
	proc->state = RUNNING;

	sched_append_pending(thread);

	return 0;

free_thread_ret:
	free_thread(thread);

free_proc_ret:
	free_process(proc);

	return ret;
}