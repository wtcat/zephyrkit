
/* auto-generated by gen_syscalls.py, don't edit */
#ifndef Z_INCLUDE_SYSCALLS_DMA_H
#define Z_INCLUDE_SYSCALLS_DMA_H


#ifndef _ASMLANGUAGE

#include <syscall_list.h>
#include <syscall.h>

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern int z_impl_dma_start(const struct device * dev, uint32_t channel);
static inline int dma_start(const struct device * dev, uint32_t channel)
{
#ifdef CONFIG_USERSPACE
	if (z_syscall_trap()) {
		/* coverity[OVERRUN] */
		return (int) arch_syscall_invoke2(*(uintptr_t *)&dev, *(uintptr_t *)&channel, K_SYSCALL_DMA_START);
	}
#endif
	compiler_barrier();
	return z_impl_dma_start(dev, channel);
}


extern int z_impl_dma_stop(const struct device * dev, uint32_t channel);
static inline int dma_stop(const struct device * dev, uint32_t channel)
{
#ifdef CONFIG_USERSPACE
	if (z_syscall_trap()) {
		/* coverity[OVERRUN] */
		return (int) arch_syscall_invoke2(*(uintptr_t *)&dev, *(uintptr_t *)&channel, K_SYSCALL_DMA_STOP);
	}
#endif
	compiler_barrier();
	return z_impl_dma_stop(dev, channel);
}


extern int z_impl_dma_request_channel(const struct device * dev, void * filter_param);
static inline int dma_request_channel(const struct device * dev, void * filter_param)
{
#ifdef CONFIG_USERSPACE
	if (z_syscall_trap()) {
		/* coverity[OVERRUN] */
		return (int) arch_syscall_invoke2(*(uintptr_t *)&dev, *(uintptr_t *)&filter_param, K_SYSCALL_DMA_REQUEST_CHANNEL);
	}
#endif
	compiler_barrier();
	return z_impl_dma_request_channel(dev, filter_param);
}


extern void z_impl_dma_release_channel(const struct device * dev, uint32_t channel);
static inline void dma_release_channel(const struct device * dev, uint32_t channel)
{
#ifdef CONFIG_USERSPACE
	if (z_syscall_trap()) {
		/* coverity[OVERRUN] */
		arch_syscall_invoke2(*(uintptr_t *)&dev, *(uintptr_t *)&channel, K_SYSCALL_DMA_RELEASE_CHANNEL);
		return;
	}
#endif
	compiler_barrier();
	z_impl_dma_release_channel(dev, channel);
}


extern int z_impl_dma_chan_filter(const struct device * dev, int channel, void * filter_param);
static inline int dma_chan_filter(const struct device * dev, int channel, void * filter_param)
{
#ifdef CONFIG_USERSPACE
	if (z_syscall_trap()) {
		/* coverity[OVERRUN] */
		return (int) arch_syscall_invoke3(*(uintptr_t *)&dev, *(uintptr_t *)&channel, *(uintptr_t *)&filter_param, K_SYSCALL_DMA_CHAN_FILTER);
	}
#endif
	compiler_barrier();
	return z_impl_dma_chan_filter(dev, channel, filter_param);
}


#ifdef __cplusplus
}
#endif

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif

#endif
#endif /* include guard */
