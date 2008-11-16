/*
 * Copyright (C) 2007-2008 Paolo Ambrosio, <blues@data-14.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "d14flt.h"

/* bitmap multiplier */
#define BITMAP_MULT_BITS (PAGE_SHIFT+3)
#define BITMAP_MULT (1 << BITMAP_MULT_BITS)
#define BITMAP_MULT_MASK (~(BITMAP_MULT-1))
#define INLINE_BITMAP_SIZE (sizeof(struct d14flt_msg) - offsetof(struct d14flt_msg, m_body.m_data.inline_bitmap))
#define bitmap_size(size) (((size+BITMAP_MULT*PAGE_SIZE-1) >> BITMAP_MULT_BITS) & ~(PAGE_SIZE-1))
#define prev_byte(bit_off) ((bit_off) >> 3)
#define next_byte(bit_off) (((bit_off) + 7) >> 3)
#define bitmask(bit_off) (0xFF >> ((bit_off) & 0x7))
#define inv_bitmask(bit_off) (~bitmask(bit_off))
#define prevpag_bit(off) ((off) >> PAGE_SHIFT)
#define nextpag_bit(off) (((off)+PAGE_SIZE-1) >> PAGE_SHIFT)

#define memset_range(bitmap, start, end, val) { while (start < end) { bitmap[start++] = val; } }

/*
 * zeroes the bitmap from the next bit from start to the last _byte_ of end
 */
void d14flt_zero_bitmap(char *bitmap,
		size_t bitmap_size, size_t start_bit)
{
	size_t cursor = prev_byte(start_bit);

	if (cursor < bitmap_size)
		bitmap[cursor++] &= inv_bitmask(start_bit);
	if (cursor < bitmap_size)
		memset_range(bitmap, cursor, bitmap_size, 0x00);
}

/*
 * copies a bitmap to another bitmap zeroing the last part
 */
void d14flt_copy_bitmap(char *new_bitmap, char *old_bitmap,
		size_t new_bitmap_size, size_t copy_size)
{
	memcpy(new_bitmap, old_bitmap, next_byte(copy_size));
	d14flt_zero_bitmap(new_bitmap, new_bitmap_size, copy_size);
}

/*
 * fills the bitmap with ones, from bit to bit
 */
void d14flt_write_range_bitmap(char *bitmap, size_t start_bit, size_t end_bit)
{
	size_t cursor = prev_byte(start_bit);

	if (prev_byte(start_bit) == prev_byte(end_bit)) {
		bitmap[cursor++] |= bitmask(start_bit) & inv_bitmask(end_bit);
	} else {
		bitmap[cursor++] |= bitmask(start_bit);
		if (cursor < prev_byte(end_bit)) {
			memset_range(bitmap, cursor, prev_byte(end_bit), 0xFF);
		}
		if (cursor < next_byte(end_bit)) {
			bitmap[cursor++] |= inv_bitmask(end_bit);
		}
	}
}

/*
 * inits the bitmap with a bit range (top extreme excluded), zeroing the rest
 *   @bitmap_size in bytes
 *   @start_bit in bit
 *   @end_bit in bit 
 */
void d14flt_init_range_bitmap(char *bitmap, size_t bitmap_size, size_t start_bit, size_t end_bit)
{
	size_t cursor = 0;

	if (cursor < prev_byte(start_bit)) {
		memset_range(bitmap, cursor, prev_byte(start_bit), 0x00);
	}
	if (prev_byte(start_bit) == prev_byte(end_bit)) {
		bitmap[cursor++] = bitmask(start_bit) & inv_bitmask(end_bit);
	} else {
		bitmap[cursor++] = bitmask(start_bit);
		if (cursor < prev_byte(end_bit)) {
			memset_range(bitmap, cursor, prev_byte(end_bit), 0xFF);
		}
		if (cursor < next_byte(end_bit)) {
			bitmap[cursor++] = inv_bitmask(end_bit);
		}
	}
	if (cursor < bitmap_size) {
		memset_range(bitmap, cursor, bitmap_size, 0x00);
	}
}

void d14flt_bitmap_resize(struct d14flt_msg *msg, struct inode *inode)
{
	size_t new_size;
	void *new_bitmap;

	if (inode->i_size <= (INLINE_BITMAP_SIZE << BITMAP_MULT_BITS)) {
		new_bitmap = msg->m_body.m_data.inline_bitmap;
		new_size = INLINE_BITMAP_SIZE;
	} else {
		new_size = bitmap_size(inode->i_size);
		if (new_size != msg->m_body.m_data.m_args.bitmap_size) {
			new_bitmap = kmalloc(new_size, GFP_KERNEL);
		} else {
			new_bitmap = msg->m_xargs;
		}
	}

	if (msg->m_body.m_data.m_args.bitmap_size == 0) { /* new bitmap */
		d14flt_init_range_bitmap(new_bitmap, new_size,
				prevpag_bit(msg->m_body.m_data.m_args.start),
				nextpag_bit(msg->m_body.m_data.m_args.end));
		msg->m_body.m_data.m_args.start = -1;
		msg->m_body.m_data.m_args.end = -1;
		msg->m_body.m_data.m_args.bitmap_size = new_size;
		msg->m_xargs = new_bitmap;
	} else if (new_bitmap != msg->m_xargs) { /* bitmap changed */
		d14flt_copy_bitmap(new_bitmap, msg->m_xargs,
				new_size, nextpag_bit(msg->m_body.m_data.m_args.i_size < inode->i_size ? msg->m_body.m_data.m_args.i_size : inode->i_size));
		if (msg->m_xargs != msg->m_body.m_data.inline_bitmap)
			kfree(msg->m_xargs);
		if (inode->i_size != 0)
			msg->m_body.m_data.m_args.bitmap_size = new_size;
		else
			msg->m_body.m_data.m_args.bitmap_size = 0;
		msg->m_xargs = new_bitmap;
	} else if (inode->i_size < msg->m_body.m_data.m_args.i_size) {
		/* zero-fill of the hole */
		d14flt_zero_bitmap(msg->m_xargs, next_byte(nextpag_bit(msg->m_body.m_data.m_args.i_size)), nextpag_bit(inode->i_size));
	}
	msg->m_body.m_data.m_args.i_size = inode->i_size;
}

void d14flt_i_msg_write(struct d14flt_msg *msg, struct inode *inode, loff_t start, loff_t end)
{
	if (!msg->m_body.m_data.m_args.bitmap_size) {
		if (msg->m_body.m_data.m_args.start == -1) {
			msg->m_body.m_data.m_args.start = start;
			msg->m_body.m_data.m_args.end = end;
			return;
		}
		if ((start <= msg->m_body.m_data.m_args.end) && (end >= msg->m_body.m_data.m_args.start)) {
			if (start < msg->m_body.m_data.m_args.start)
				msg->m_body.m_data.m_args.start = start;
			if (end > msg->m_body.m_data.m_args.end)
				msg->m_body.m_data.m_args.end = end;
		} else { /* switch to bitmap */
			d14flt_bitmap_resize(msg, inode);
			/* fill bitmap with range */
			d14flt_write_range_bitmap(msg->m_xargs,
					prevpag_bit(start), nextpag_bit(end));
		}
	} else { /* write with bitmap used */
		/* resize bitmap (if necessary) only if size changed */
		if (msg->m_body.m_data.m_args.i_size != inode->i_size)
			d14flt_bitmap_resize(msg, inode);
		d14flt_write_range_bitmap(msg->m_xargs,
				prevpag_bit(start), nextpag_bit(end));
	}
}

/*
 * sets a single bit in the bitmap
 */
void d14flt_i_writepage(struct d14flt_msg *msg, size_t bit)
{
	char *bitmap = msg->m_xargs;

	bitmap[prev_byte(bit)] |= 1 >> (bit & 7);
}

void d14flt_i_msg_truncate(struct d14flt_msg *msg, struct inode *inode)
{
	if (!msg->m_body.m_data.m_args.bitmap_size) {
		if (msg->m_body.m_data.m_args.start == -1)
			return;
		if (msg->m_body.m_data.m_args.start >= inode->i_size) {
			msg->m_body.m_data.m_args.start = -1;
			msg->m_body.m_data.m_args.end = -1;
			return;
		}
		if (msg->m_body.m_data.m_args.end > inode->i_size) {
			msg->m_body.m_data.m_args.end = inode->i_size;
			return;
		}
	} else {
		d14flt_bitmap_resize(msg, inode);
	}
}
