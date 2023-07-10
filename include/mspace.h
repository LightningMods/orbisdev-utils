#ifndef _LIBC_INT
#define _LIBC_INT

typedef void* OrbisMspace;
typedef struct OrbisMallocManagedSize 
{
	unsigned short sz;
	unsigned short ver;
	unsigned int reserv;
	size_t maxSysSz;
	size_t curSysSz;
	size_t maxUseSz;
	size_t curUseSz;
}OrbisMallocManagedSize;

OrbisMspace sceLibcMspaceCreate(char *, void *, size_t, unsigned int);
int sceLibcMspaceDestroy(OrbisMspace);
void * sceLibcMspaceMalloc(OrbisMspace, size_t size);
int sceLibcMspaceFree(OrbisMspace, void *);
void * sceLibcMspaceCalloc(OrbisMspace, size_t, size_t);
void * sceLibcMspaceMemalign(OrbisMspace, size_t, size_t);
void * sceLibcMspaceRealloc(OrbisMspace, void *, size_t);
void * sceLibcMspaceReallocalign(OrbisMspace, void *, size_t, size_t);
int sceLibcMspacePosixMemalign(OrbisMspace, void **, size_t, size_t);
int sceLibcMspaceMallocStats(OrbisMspace, OrbisMallocManagedSize *);
int sceLibcMspaceMallocStatsFast(OrbisMspace, OrbisMallocManagedSize *);
size_t sceLibcMspaceMallocUsableSize(void *);
int sceLibcMspaceMspaceisHeapEmpty(OrbisMspace);
#endif // _LIBC_INT