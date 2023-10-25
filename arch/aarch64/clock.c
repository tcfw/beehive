#include <kernel/clock.h>
#include <kernel/tty.h>

void enableSystemInterruptMask(struct clocksource_t *cs, uint64_t mask);
void disableSystemInterruptMask(struct clocksource_t *cs, uint64_t mask);

uint64_t getSystemCounterValue(struct clocksource_t *cs)
{
	uint64_t value = 0;
	__asm__ volatile("MRS %0, CNTP_TVAL_EL0"
					 : "=r"(value));

	return value & 0xffffffff;
}

void enableSystemCounter(struct clocksource_t *cs)
{
	uint64_t cnt_ctl = 0;
	__asm__ volatile("MRS %0, CNTP_CTL_EL0"
					 : "=r"(cnt_ctl));

	cnt_ctl |= (1ULL << 0);

	__asm__ volatile("MSR CNTP_CTL_EL0, %0" ::"r"(cnt_ctl));
}

void disableSystemCounter(struct clocksource_t *cs)
{
	uint64_t cnt_ctl = 0;
	__asm__ volatile("MSR CNTP_CTL_EL0, %0" ::"r"(cnt_ctl));
}

uint64_t getSysCounterValue(struct clocksource_t *cs)
{
	uint64_t value = 0;
	__asm__ volatile("MRS %0, CNTPCT_EL0"
					 : "=r"(value));
	return value;
}

uint64_t getSystemCounterFreq(struct clocksource_t *cs)
{
	uint64_t freq = 0;
	__asm__ volatile("MRS %0, CNTFRQ_EL0"
					 : "=r"(freq));
	return (freq & 0xffffffff);
}

void setSystemCounterFreq(struct clocksource_t *cs, uint64_t hz)
{
	hz &= 0xffffffff;
	__asm__ volatile("MSR CNTFRQ_EL0, %0" ::"r"(hz));
}

void setSystemCounterValue(struct clocksource_t *cs, uint64_t value)
{
	// armv8-a only supports a 32bit counter
	value &= 0xffffffff;
	__asm__ volatile("MSR CNTP_TVAL_EL0, %0" ::"r"(value));
}

void setSystemCounterCompareValue(struct clocksource_t *cs, uint64_t value)
{
	__asm__ volatile("MSR CNTP_CVAL_EL0, %0" ::"r"(value));
}

void enableSystemInterruptMask(struct clocksource_t *cs, uint64_t mask)
{
	uint64_t cnt_ctl = 0;
	__asm__ volatile("MRS %0, CNTP_CTL_EL0"
					 : "=r"(cnt_ctl));

	cnt_ctl &= ~(1ULL << 1);

	__asm__ volatile("MSR CNTP_CTL_EL0, %0" ::"r"(cnt_ctl));
}

void disableSystemInterruptMask(struct clocksource_t *cs, uint64_t mask)
{
	uint64_t cnt_ctl = 0;
	__asm__ volatile("MRS %0, CNTP_CTL_EL0"
					 : "=r"(cnt_ctl));

	cnt_ctl |= (1ULL << 1);

	__asm__ volatile("MSR CNTP_CTL_EL0, %0" ::"r"(cnt_ctl));
}

static struct clocksource_t systemClock = {
	.type = CS_GLOBAL,
	.enable = enableSystemCounter,
	.disable = disableSystemCounter,
	.getFreq = getSystemCounterFreq,
	.setFreq = setSystemCounterFreq,
	.val = getSysCounterValue,
	.countNTicks = setSystemCounterValue,
	.countTo = setSystemCounterCompareValue,
	.enableIRQ = enableSystemInterruptMask,
	.disableIRQ = disableSystemInterruptMask,
};

static void enableLocalCounter(struct clocksource_t *cs);
static uint64_t getLocalCounterVal(struct clocksource_t *cs);
static void setLocalCounterVal(struct clocksource_t *cs, uint64_t);
static void enableLocalCounterInterruptMask(struct clocksource_t *cs);
static void disableLocalCounterInterruptMask(struct clocksource_t *cs);
static void setLocalCounterCompareValue(struct clocksource_t *cs, uint64_t);

static void enableLocalCounter(struct clocksource_t *cs)
{
	uint64_t cnt_ctl = 0x1;
	__asm__ volatile("MSR CNTV_CTL_EL0, %0" ::"r"(cnt_ctl));
}

static void disableLocalCounter(struct clocksource_t *cs)
{
	uint64_t cnt_ctl = 0x0;
	__asm__ volatile("MSR CNTV_CTL_EL0, %0" ::"r"(cnt_ctl));
}

static uint64_t getLocalCounterVal(struct clocksource_t *cs)
{
	uint64_t value = 0;
	__asm__ volatile("MRS %0, CNTVCT_EL0"
					 : "=r"(value));
	return value;
}

static void enableLocalCounterInterruptMask(struct clocksource_t *cs)
{
	uint64_t cnt_ctl = 0;
	__asm__ volatile("MRS %0, CNTV_CTL_EL0"
					 : "=r"(cnt_ctl));

	cnt_ctl &= ~(1ULL << 1);

	__asm__ volatile("MSR CNTV_CTL_EL0, %0" ::"r"(cnt_ctl));
}

static void disableLocalCounterInterruptMask(struct clocksource_t *cs)
{
	uint64_t cnt_ctl = 0;
	__asm__ volatile("MRS %0, CNTV_CTL_EL0"
					 : "=r"(cnt_ctl));

	cnt_ctl |= (1ULL << 1);

	__asm__ volatile("MSR CNTV_CTL_EL0, %0" ::"r"(cnt_ctl));
}

static void setLocalCounterCompareValue(struct clocksource_t *cs, uint64_t val)
{
	__asm__ volatile("MSR CNTV_CVAL_EL0, %0" ::"r"(val));
}

static void setLocalCounterTValue(struct clocksource_t *cs, uint64_t val)
{
	__asm__ volatile("MSR CNTV_TVAL_EL0, %0" ::"r"(val));
}

static struct clocksource_t localClock = {
	.type = CS_LOCAL,
	.enable = enableLocalCounter,
	.disable = disableLocalCounter,
	.getFreq = getSystemCounterFreq,
	.setFreq = setSystemCounterFreq,
	.val = getSystemCounterValue,
	.enableIRQ = enableLocalCounterInterruptMask,
	.disableIRQ = disableLocalCounterInterruptMask,
	.countTo = setLocalCounterCompareValue,
	.countNTicks = setLocalCounterTValue,
};

struct clocksource_t rtClock = {
	.type = CS_RTC,
};

void registerClocks(void)
{
	RegisterClockSource(&systemClock);
	RegisterClockSource(&localClock);
	RegisterClockSource(&rtClock);
}