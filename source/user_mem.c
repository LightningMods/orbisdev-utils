#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "user_mem.h"
#include <fcntl.h>
#ifndef OO_PS4_TOOLCHAIN
#include <orbis/libSceLibcInternal.h>
#else
#include "mspace.h"
#endif
#include <orbis/libkernel.h>
static OrbisMspace s_mspace = 0;
static OrbisMallocManagedSize s_mmsize;
static void *s_mem_start = 0;
static size_t s_mem_size = MEM_SIZE;

off_t phyAddr = 0;

#define MB(x)   ((size_t) (x) << 20)
#define KB(x)   ((size_t) (x) << 10)

static size_t maxheap = MB(512);
#define SCE_OK 0
#define SCE_KERNEL_WB_ONION 0

void log_for_memory(char * format, ...) {
  //Check the length of the format string.
  // If it is too long, return immediately.
  if (strlen(format) > 1000) {
    return;
  }

  // Initialize a buffer to hold the formatted log message.
  char buff[1024];

  // Initialize a va_list to hold the variable arguments passed to the function.
  va_list args;
  va_start(args, format);

  // Format the log message using the format string and the variable arguments.
  vsprintf(buff, format, args);

  // Clean up the va_list.
  va_end(args);

  // Append a newline character to the log message.
  strcat(buff, "\n");

  // Output the log message using sceKernelDebugOutText.
  sceKernelDebugOutText(0, buff);

  int fd = sceKernelOpen("/data/memory.log", O_WRONLY | O_CREAT | O_APPEND, 0777);
  if (fd >= 0) {
    sceKernelWrite(fd, & buff[0], strlen( & buff[0]));
    sceKernelClose(fd);
  }
}

int malloc_init(void) {
  int res;

  if (s_mspace)
    return 0;

  res = sceKernelReserveVirtualRange(&s_mem_start, MEM_SIZE, 0, MEM_ALIGN);
  if (res < 0) {
    log_for_memory("sceKernelReserveVirtualRange failed with error code 0x%X", res);
    return 1;
  }

  res = sceKernelMapNamedSystemFlexibleMemory(&s_mem_start, MEM_SIZE, SCE_KERNEL_PROT_CPU_RW, SCE_KERNEL_MAP_FIXED, "User Mem");
  if (res < 0) {
    //log_for_memory("sceKernelMapNamedSystemFlexibleMemory failed with error code 0x%X", res);
    log_for_memory("trying backup mspace");
    int32_t res = SCE_OK;
    if (SCE_OK != (res = sceKernelAllocateMainDirectMemory(maxheap, KB(64), SCE_KERNEL_WB_ONION, &phyAddr))) {
      log_for_memory("sceKernelAllocateMainDirectMemory() Failed = 0x%X", res); // with 0x%08X \n", (unat)res);
      return 1;
    }
    if (SCE_OK != (res = sceKernelMapDirectMemory( & s_mem_start, maxheap, SCE_KERNEL_PROT_CPU_RW, 0, phyAddr, KB(64)))) {
      log_for_memory("sceKernelMapDirectMemory() Failed = 0x%X", res); // with 0x%08X \n", (unat)res);
      return 1;
    }
    s_mem_size = maxheap;
  }

  s_mspace = sceLibcMspaceCreate("User Mspace", s_mem_start, s_mem_size, 0);
  if (!s_mspace) {
    log_for_memory("sceLibcMspaceCreate() Failed = 0x%X", s_mspace);
    return 1;
  } else
    log_for_memory("sceLibcMspaceCreate() successful");

  s_mmsize.sz = sizeof(s_mmsize);
  s_mmsize.ver = 1;
  res = sceLibcMspaceMallocStatsFast(s_mspace, &s_mmsize);
  log_for_memory("sceLibcMspaceMallocStatsFast() returned %i", res);
  return 0;
}		
int malloc_finalize(void)
{
	int res;

	if (s_mspace)
	{
		res = sceLibcMspaceDestroy(s_mspace);
		if (res != 0)
			return 1;
	}

	res = sceKernelReleaseFlexibleMemory(s_mem_start, s_mem_size);
	if (res < 0)
		return 1;

	res = sceKernelMunmap(s_mem_start, s_mem_size);
	if (res < 0)
		return 1;

	return 0;
}

void *malloc(size_t size)
{
	if (!s_mspace)
		malloc_init();

	return sceLibcMspaceMalloc(s_mspace, size);
}

void free(void *ptr)
{

	if (!ptr || !s_mspace)
		return;

	sceLibcMspaceFree(s_mspace, ptr);
}

void *calloc(size_t nelem, size_t size)
{
	if (!s_mspace)
		malloc_init();
	
	return sceLibcMspaceCalloc(s_mspace, nelem, size);
}

void *realloc(void *ptr, size_t size)
{
	if (!s_mspace)
		malloc_init();

	return sceLibcMspaceRealloc(s_mspace, ptr, size);
}

void *memalign(size_t boundary, size_t size)
{
	if (!s_mspace)
		malloc_init();

	return sceLibcMspaceMemalign(s_mspace, boundary, size);
}

int posix_memalign(void **ptr, size_t boundary, size_t size)
{
	if (!s_mspace)
		malloc_init();

	return sceLibcMspacePosixMemalign(s_mspace, ptr, boundary, size);
}

void *reallocalign(void *ptr, size_t size, size_t boundary)
{
	if (!s_mspace)
		malloc_init();

	return sceLibcMspaceReallocalign(s_mspace, ptr, boundary, size);
}

int malloc_stats(OrbisMallocManagedSize *mmsize)
{
	if (!s_mspace)
		malloc_init();

	return sceLibcMspaceMallocStats(s_mspace, mmsize);
}

int malloc_stats_fast(OrbisMallocManagedSize *mmsize)
{
	if (!s_mspace)
		malloc_init();

	return sceLibcMspaceMallocStatsFast(s_mspace, mmsize);
}

size_t malloc_usable_size(void *ptr)
{
	if (!ptr)
		return 0;

	return sceLibcMspaceMallocUsableSize(ptr);
}

int vasprintf(char **bufp, const char *format, va_list ap)
{
	va_list ap1;
	int bytes;
	char *p;

	va_copy(ap1, ap);

	bytes = vsnprintf(NULL, 0, format, ap1) + 1;
	va_end(ap1);

	*bufp = p = malloc(bytes);
	if (!p)
		return -1;

	return vsnprintf(p, bytes, format, ap);
}

int asprintf(char **bufp, const char *format, ...)
{
	va_list ap, ap1;
	int rv;
	int bytes;
	char *p;

	va_start(ap, format);
	va_copy(ap1, ap);

	bytes = vsnprintf(NULL, 0, format, ap1) + 1;
	va_end(ap1);

	*bufp = p = malloc(bytes);
	if (!p)
		return -1;

	rv = vsnprintf(p, bytes, format, ap);
	va_end(ap);

	return rv;
}

char *strdup(const char *s)
{
	size_t len = strlen(s) + 1;
	void *new_s = malloc(sizeof(char) * len);

	if (!new_s)
		return NULL;

	return (char *)memcpy(new_s, s, len);
}

char *strndup(const char *s, size_t n)
{
	if (!s)
		return NULL;

	char *result;
	size_t len = strnlen(s, n);

	result = (char *)malloc(sizeof(char) * (len + 1));
	if (!result)
		return 0;

	result[len] = '\0';
	return (char *)memcpy(result, s, len);
}

void get_user_mem_size(size_t *max_mem, size_t *cur_mem)
{

	s_mmsize.sz = sizeof(s_mmsize);
	s_mmsize.ver = 1;
	sceLibcMspaceMallocStatsFast(s_mspace, &s_mmsize);
	*max_mem += s_mmsize.curSysSz;
	*cur_mem += s_mmsize.curSysSz - s_mmsize.curUseSz;
}