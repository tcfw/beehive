#include <kernel/sync.h>
#include <kernel/irq.h>
#include <stdint.h>
#include "unistd.h"

#define PL011_DR_OFFSET 0x000
#define PL011_FR_OFFSET 0x018
#define PL011_IBRD_OFFSET 0x024
#define PL011_FBRD_OFFSET 0x028
#define PL011_LCR_OFFSET 0x02c
#define PL011_CR_OFFSET 0x030
#define PL011_IMSC_OFFSET 0x038
#define PL011_ICR 0x044
#define PL011_DMACR_OFFSET 0x048

#define PL011_FR_BUSY (1 << 5)

#define PL011_CR_TXEN (1 << 8)
#define PL011_CR_RXEN (1 << 9)
#define PL011_CR_UARTEN (1 << 0)

#define PL011_LCR_FEN (1 << 4)
#define PL011_LCR_STP2 (1 << 3)

// PL011 UART driver
struct pl011
{
    spinlock_t lock;
    uint64_t base_address;
    uint64_t base_clock;
    uint32_t baudrate;
    uint32_t data_bits;
    uint32_t stop_bits;
};

// Write to a specific register given the offset
static void pl011_regwrite(const struct pl011 *dev, uint32_t offset, uint32_t data)
{
    volatile uint32_t *addr = (void *)dev->base_address + offset;
    *addr = data;
}

// Read from a specific register given the offset
static uint32_t pl011_regread(const struct pl011 *dev, uint32_t offset)
{
    volatile uint64_t *addr = (void *)dev->base_address + offset;

    return *addr;
}

// Wait for the busy state to clear
static void pl011_wait_tx_complete(const struct pl011 *dev)
{
    while ((pl011_regread(dev, PL011_FR_OFFSET) & PL011_FR_BUSY) != 0)
    {
    }
}

// Calculate the clock rate divisors
static void pl011_calculate_divisors(const struct pl011 *dev, uint32_t *integer, uint32_t *fractional)
{
    // 64 * F_UARTCLK / (16 * B) = 4 * F_UARTCLK / B
    const uint32_t div = 4 * dev->base_clock / dev->baudrate;

    *fractional = div & 0x3f;
    *integer = (div >> 6) & 0xffff;
}

// Reset the PL011 device
// Enables interrupts for recieve only
static int pl011_reset(const struct pl011 *dev)
{
    uint32_t lcr = pl011_regread(dev, PL011_LCR_OFFSET);
    uint32_t ibrd, fbrd;

    // Disable UART before anything else
    // pl011_regwrite(dev, PL011_CR_OFFSET, cr & PL011_CR_UARTEN);

    // Wait for any ongoing transmissions to complete
    pl011_wait_tx_complete(dev);

    // Flush FIFOs
    pl011_regwrite(dev, PL011_LCR_OFFSET, lcr & ~PL011_LCR_FEN);

    // Set frequency divisors (UARTIBRD and UARTFBRD) to configure the speed
    pl011_calculate_divisors(dev, &ibrd, &fbrd);
    pl011_regwrite(dev, PL011_IBRD_OFFSET, ibrd);
    pl011_regwrite(dev, PL011_FBRD_OFFSET, fbrd);

    // Configure data frame format according to the parameters (UARTLCR_H).
    // We don't actually use all the possibilities, so this part of the code
    // can be simplified.
    lcr = 0x0;
    // WLEN part of UARTLCR_H, you can check that this calculation does the
    // right thing for yourself
    lcr |= ((dev->data_bits - 1) & 0x3) << 5;
    // Configure the number of stop bits
    if (dev->stop_bits == 2)
        lcr |= PL011_LCR_STP2;

    // Mask all interrupts by setting corresponding bits to 1
    pl011_regwrite(dev, PL011_IMSC_OFFSET, ((1 << 4))); // 0x7ff

    // Disable DMA by setting all bits to 0
    pl011_regwrite(dev, PL011_DMACR_OFFSET, 0x0);

    // Enable TX & RC
    pl011_regwrite(dev, PL011_CR_OFFSET, PL011_CR_TXEN | PL011_CR_RXEN);

    // Finally enable UART
    pl011_regwrite(dev, PL011_CR_OFFSET, PL011_CR_TXEN | PL011_CR_RXEN | PL011_CR_UARTEN);

    return 0;
}

// Send a string of length size to the data register
static int pl011_send(const struct pl011 *dev, const char *data, size_t size)
{
    // spinlock_acquire(&dev->lock);

    // make sure that there is no outstanding transfer just in case
    pl011_wait_tx_complete(dev);

    for (size_t i = 0; i < size; ++i)
    {
        if (data[i] == '\n')
        {
            pl011_regwrite(dev, PL011_DR_OFFSET, '\r');
            pl011_wait_tx_complete(dev);
        }
        pl011_regwrite(dev, PL011_DR_OFFSET, data[i]);
        pl011_wait_tx_complete(dev);
    }

    // spinlock_release(&dev->lock);

    return 0;
}

// Init the PL011 UART
static int pl011_setup(struct pl011 *dev, uint64_t base_address, uint64_t base_clock)
{
    dev->base_address = base_address;
    dev->base_clock = base_clock;

    dev->baudrate = 115200;
    dev->data_bits = 8;
    dev->stop_bits = 1;

    spinlock_init(&dev->lock);
    return pl011_reset(dev);
}

static struct pl011 serial;

// Terminal receive callback
void terminal_cb(unsigned int _)
{
    char ch = pl011_regread(&serial, PL011_DR_OFFSET);
    pl011_send(&serial, &ch, 1);
    pl011_regwrite(&serial, PL011_ICR, (1 << 6) | (1 << 4));
}

// Set up the terminal and map interrupt 33
void terminal_initialize(void)
{
    pl011_setup(&serial, 0x09000000, 24000000);
    assign_irq_hook(33, &terminal_cb);
}

// Print a single char to the terminal
void terminal_putchar(const char c)
{
    pl011_send(&serial, &c, 1);
}

// Write a null-terminated string to the terminal
void terminal_writestring(char *str)
{
    while (*str)
        terminal_putchar(*str++);
}

// Write a string given the specific length
void terminal_write(const char *data, size_t size)
{
    pl011_send(&serial, data, size);
}