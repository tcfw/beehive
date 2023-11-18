#include <tests/tests.h>
#include <kernel/mm.h>
#include <kernel/slub.h>

TEST("slub alloc")
{
	slub_t *slub = DEFINE_DYN_SLUB(8U);

	void *addr = slub_alloc(slub);
	void *aa_first = slub->per_cpu_partial[0]->first;
	void *addr2 = slub_alloc(slub);

	assert_neq_msg(addr, 0, "slub alloc addr should not be zero");
	assert_eq_msg(slub->object_count, 2, "slub alloc object count should be 1");
	assert_eq_msg(slub->per_cpu_partial[0]->first, aa_first + slub->object_size, "slub cache first should have shifted sizeof(object)");
	assert_eq_msg(addr2, aa_first, "addr2 should be addr + sizeof(object)");

	// cleanup
	page_free(slub->per_cpu_partial[0]);
	page_free(slub);
}

TEST("slub free")
{
	slub_t *slub = DEFINE_DYN_SLUB(8U);
	void *addr = slub_alloc(slub);
	void *addr2 = slub_alloc(slub);
	slub_free(addr);
	void *addr3 = slub_alloc(slub);

	assert_eq_msg(addr, addr3, "addr3 should have same allocd same address as addr");
	assert_eq_msg(slub->object_count, 2, "slub should only have 2 objects");

	page_free(slub->per_cpu_partial[0]);
	page_free(slub);
}

TEST("slub huge alloc")
{
	slub_t *slub = DEFINE_DYN_SLUB(3800U);
	void *addr = slub_alloc(slub);

	assert_neq(addr, NULL);
	assert_neq_msg(addr, NULL, "addr should not be null");
	assert_eq_msg(slub->per_cpu_partial[0], NULL, "slub per_cpu cache should be full");

	slub_free(addr);

	assert_list_empty_msg(&slub->partial, "slub partial should not be empty");
	assert_eq_msg(slub->per_cpu_partial[0], NULL, "per-cpu cache should be empty");

	void *addr2 = slub_alloc(slub);
	assert_neq(addr2, NULL);
	assert_eq_msg(slub->per_cpu_partial[0], NULL, "per-cpu cache should still be empty");
	assert_list_empty_msg(&slub->partial, "slub partial should still be empty");

	slub_free(addr2);

	page_free(slub);
}