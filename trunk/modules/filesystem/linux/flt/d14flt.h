#ifndef D14FLT_H_
#define D14FLT_H_

#include <linux/fs.h>
#include <linux/file.h>
#include <linux/version.h>
#include <asm/semaphore.h>
#include "redirfs.h"
#include "comm.h"
#include "trace.h"


extern rfs_filter d14flt_flt;

struct d14flt_i_data {
	struct rfs_priv_data rfs_data;
	atomic_t mmapped;
	struct semaphore msg_lock;
	struct d14flt_msg *msg;
};

#define rfs_to_d14flt_i_data(ptr) container_of(ptr, struct d14flt_i_data, rfs_data)

extern struct kmem_cache *d14flt_i_data_cache;

int d14flt_rfs_data_cache_init(void);
void d14flt_rfs_data_cache_exit(void);

void d14flt_i_msg_put(struct d14flt_i_data *data);
void d14flt_i_msg_queue(struct d14flt_i_data *data);
struct d14flt_i_data *d14flt_i_msg_get(struct inode *inode, int create);

void d14flt_i_msg_write(struct d14flt_msg *msg, struct inode *inode, loff_t start, loff_t end);
void d14flt_i_writepage(struct d14flt_msg *msg, size_t bit);
void d14flt_i_msg_truncate(struct d14flt_msg *msg, struct inode *inode);



extern char *d14flt_privdir;

#define ONE_NAME_INLINE_LEN_MIN DNAME_INLINE_LEN_MIN
#define TWO_NAMES_INLINE_LEN_MIN (DNAME_INLINE_LEN_MIN*2)
#define NAME_TARGET_INLINE_LEN_MIN 92
#define BITMAP_INLINE_SIZE_MIN 1

/* kernel message structure */

struct d14flt_msg {
	struct list_head list;
	struct file *file;
	__kernel_size_t m_args_len;
	__kernel_size_t m_xargs_len;
	void *m_xargs; /* if 0 or ptr to this structure, it should not be freed */
	struct {
		int m_id;
		int m_type;
	} m_head;
	union {
		struct {
			m_create_t m_args;
			unsigned char inline_names[NAME_TARGET_INLINE_LEN_MIN];
		} m_create;
		struct {
			m_link_t m_args;
			unsigned char inline_name[ONE_NAME_INLINE_LEN_MIN];
		} m_link;
		struct {
			m_rename_t m_args;
			unsigned char inline_names[TWO_NAMES_INLINE_LEN_MIN];
		} m_rename;
		struct {
			m_delete_t m_args;
			unsigned char inline_name[ONE_NAME_INLINE_LEN_MIN];
		} m_delete;
		struct {
			m_attr_t m_args;
		} m_attr;
		struct {
			m_data_t m_args;
			unsigned char inline_bitmap[BITMAP_INLINE_SIZE_MIN];
		} m_data;
	} m_body;
};

/* d14flt_dev */
int d14flt_dev_init(void);
void d14flt_dev_exit(void);

/* d14flt_evt */
int d14flt_msg_init(void);
void d14flt_msg_exit(void);
struct d14flt_msg *d14flt_alloc_msg(void);
void d14flt_free_msg(struct d14flt_msg *msg);
void d14flt_queue_msg(struct d14flt_msg *msg);
void d14flt_dequeue_msg(struct d14flt_msg *msg);
inline void d14flt_wait_msg(void);
struct d14flt_msg *d14flt_first_msg(void);
struct d14flt_msg *d14flt_get_msg(struct inode *inode);
void d14flt_release_msg(struct inode *inode, struct d14flt_msg *msg);

/* events */
void d14flt_on_create(struct inode *dir, struct dentry *dentry, const char *oldname);
void d14flt_on_link(struct inode *dir, struct dentry *dentry);
void d14flt_on_rename(struct inode *old_dir, struct dentry *old_dentry,
		struct inode *new_dir, struct dentry *new_dentry);
void d14flt_on_unlink(struct inode *dir, struct dentry *dentry);
void d14flt_on_delete(struct inode *dir, struct dentry *dentry);

/* d14flt_rfs */
int d14flt_rfs_init(void);
void d14flt_rfs_exit(void);

#endif /* D14FLT_H_ */
