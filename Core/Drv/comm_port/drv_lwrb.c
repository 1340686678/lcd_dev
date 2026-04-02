/**
 * \file            lwrb.c
 * \brief           Lightweight ring buffer
 */

/*
 * Copyright (c) 2023 Tilen MAJERLE
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * This file is part of LwRB - Lightweight ring buffer library.
 *
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 * Version:         v3.0.0-rc1
 */
#include "drv_lwrb.h"

/*内存设置和复制功能*/
#define BUF_MEMSET memset
#define BUF_MEMCPY memcpy

//缓存区是否有效
#define BUF_IS_VALID(b) ((b) != NULL && (b)->p_buff != NULL && (b)->size > 0)
//两个数中的最小值
#define BUF_MIN(x, y)   ((x) < (y) ? (x) : (y))
//两个数中的最打值
#define BUF_MAX(x, y)   ((x) > (y) ? (x) : (y))
//事件回调
#define BUF_SEND_EVT(b, type, bp)							\
	do {																				\
		if ((b)->evt_fn != NULL)									\
		{																					\
			(b)->evt_fn((void*)(b), (type), (bp));	\
		}																					\
	} while (0)

/* 可选原子操作 */
#ifdef LWRB_DISABLE_ATOMIC
#define LWRB_INIT(var, val)					(var) = (val)
#define LWRB_LOAD(var, type)				(var)
#define LWRB_STORE(var, val, type)	(var) = (val)
#else
#define LWRB_INIT(var, val)					atomic_init(&(var), (val))
#define LWRB_LOAD(var, type)				atomic_load_explicit(&(var), (type))
#define LWRB_STORE(var, val, type)	atomic_store_explicit(&(var), (val), (type))
#endif

/***************************************************************
 * Name:				lwrb_init()
 * Input :			p_buff 句柄 p_buff_buff 缓存区地址 size 大小
 * Output:			void
 * Return:			0 失败 1 成功
 * Author:			
 * Revise:			V1.0
 * Description:	初始化句柄
 ***************************************************************/
uint8_t lwrb_init(lwrb_t* p_buff, void* p_buff_buff, size_t size)
{
	/*有效性判断*/
	if (p_buff == NULL || p_buff_buff == NULL || size == 0)
	{
		return 0;
	}

	/*初始化接口*/
	p_buff->evt_fn = NULL;
	p_buff->size = size;
	p_buff->p_buff = p_buff_buff;
	LWRB_INIT(p_buff->w, 0);
	LWRB_INIT(p_buff->r, 0);
	return 1;
}

/***************************************************************
 * Name:				lwrb_is_ready()
 * Input :			p_buff 句柄
 * Output:			void
 * Return:			0 不可以 1 可以
 * Author:			
 * Revise:			V1.0
 * Description:	判断句柄是否可以使用
 ***************************************************************/
uint8_t lwrb_is_ready(lwrb_t* p_buff)
{
	return BUF_IS_VALID(p_buff);
}

/***************************************************************
 * Name:				lwrb_free()
 * Input :			p_buff 句柄
 * Output:			void
 * Return:			void
 * Author:			
 * Revise:			V1.0
 * Description:	释放句柄
 ***************************************************************/
void lwrb_free(lwrb_t* p_buff)
{
	if (BUF_IS_VALID(p_buff))
	{
		p_buff->p_buff = NULL;
		p_buff->size = 0;
	}
}

/***************************************************************
 * Name:				lwrb_reset()
 * Input :			p_buff 句柄
 * Output:			void
 * Return:			void
 * Author:			
 * Revise:			V1.0
 * Description:	重置
 ***************************************************************/
void lwrb_reset(lwrb_t* p_buff)
{
	if (BUF_IS_VALID(p_buff))
	{
		LWRB_STORE(p_buff->w, 0, memory_order_release);
		LWRB_STORE(p_buff->r, 0, memory_order_release);
		BUF_SEND_EVT(p_buff, LWRB_EVT_RESET, 0);
	}
}

/***************************************************************
 * Name:				lwrb_set_evt_fn()
 * Input :			p_buff 句柄 fn 事件回调函数
 * Output:			void
 * Return:			void
 * Author:			
 * Revise:			V1.0
 * Description:	设置事件回调函数
 ***************************************************************/
void lwrb_set_evt_fn(lwrb_t* p_buff, lwrb_evt_fn fn)
{
	if (BUF_IS_VALID(p_buff))
	{
		p_buff->evt_fn = fn;
	}
}

/***************************************************************
 * Name:				lwrb_write()
 * Input :			p_buff 句柄 c_p_data 写入数据 btw 数据大小 
 * Output:			void
 * Return:			写入的长度
 * Author:			
 * Revise:			V1.0
 * Description:	写入数据
 ***************************************************************/
size_t lwrb_write(lwrb_t* p_buff, const void* c_p_data, size_t btw)
{
	size_t tocopy, free, buff_w_ptr;
	const uint8_t* c_p_d = c_p_data;

	/*判断有效性*/
	if (!BUF_IS_VALID(p_buff) || c_p_data == NULL || btw == 0) 
	{
		return 0;
	}

	/*计算可写入的最大字节数*/
	free = lwrb_get_free(p_buff);
	btw = BUF_MIN(free, btw);
	/*可写入为0 退出*/
	if (btw == 0)
	{
		return 0;
	}

	/*读取当前写指针*/
	buff_w_ptr = LWRB_LOAD(p_buff->w, memory_order_acquire);

	/* 1 将数据写入缓冲区的线性部分*/
	tocopy = BUF_MIN(p_buff->size - buff_w_ptr, btw);	//最大写入长度
	BUF_MEMCPY(&p_buff->p_buff[buff_w_ptr], c_p_d, tocopy);	//写入数据
	buff_w_ptr += tocopy;	//更新写指针
	btw -= tocopy;				//更新需要写入的长度

	/* 2 将数据写入缓冲区的开头（溢出部分）*/
	if (btw > 0) 
	{
		BUF_MEMCPY(p_buff->p_buff, &c_p_d[tocopy], btw);	//写入数据
		buff_w_ptr = btw;													//更新写指针
	}

	/* 3 检查缓冲区结束 TODO 如果写入长度是两倍的缓冲区长度 会丢失最后的数据*/
	if (buff_w_ptr >= p_buff->size)
	{
		buff_w_ptr = 0;
	}

	/*将最终值写入实际运行变量*/
	LWRB_STORE(p_buff->w, buff_w_ptr, memory_order_release);

	/*运行回调函数*/
	BUF_SEND_EVT(p_buff, LWRB_EVT_WRITE, tocopy + btw);

	/*返回写入的长度*/
	return tocopy + btw;
}

/***************************************************************
 * Name:				lwrb_read()
 * Input :			p_buff 句柄 btr 存储读取数据的缓冲区大小 
 * Output:			p_data 存储读取数据的缓冲区
 * Return:			读取的长度
 * Author:			
 * Revise:			V1.0
 * Description:	读取数据
 ***************************************************************/
size_t lwrb_read(lwrb_t* p_buff, void* p_data, size_t btr)
{
	size_t tocopy, full, buff_r_ptr;
	uint8_t* p_d = p_data;

	/*判断有效性*/
	if (!BUF_IS_VALID(p_buff) || p_data == NULL || btr == 0)
	{
			return 0;
	}

	/*计算可读取的最大字节数*/
	full = lwrb_get_full(p_buff);
	btr = BUF_MIN(full, btr);
	/*可读取为0 退出*/
	if (btr == 0)
	{
		return 0;
	}

	/*读取当前读指针*/
	buff_r_ptr = LWRB_LOAD(p_buff->r, memory_order_acquire);

	/* 1 将读取缓冲区的线性部分 */
	tocopy = BUF_MIN(p_buff->size - buff_r_ptr, btr);	//最大读取长度
	BUF_MEMCPY(p_d, &p_buff->p_buff[buff_r_ptr], tocopy);	//读取数据
	buff_r_ptr += tocopy;	//更新读指针
	btr -= tocopy;				//更新需要读取的长度

	/* 2 从缓冲区开始读取数据（溢出部分）*/
	if (btr > 0)
	{
		BUF_MEMCPY(&p_d[tocopy], p_buff->p_buff, btr);	//读取数据
		buff_r_ptr = btr;													//更新读指针
	}

	/* 3 检查缓冲区结束 TODO 和写入一个问题 太长会丢失*/
	if (buff_r_ptr >= p_buff->size)
	{
		buff_r_ptr = 0;
	}

	/*将最终值写入实际运行变量*/
	LWRB_STORE(p_buff->r, buff_r_ptr, memory_order_release);

	/*运行回调函数*/
	BUF_SEND_EVT(p_buff, LWRB_EVT_READ, tocopy + btr);

	/*返回读取的长度*/
	return tocopy + btr;
}

/***************************************************************
 * Name:				lwrb_peek()
 * Input :			c_p_buff 句柄 skip_count 跳过的长度 btr 存储读取数据的缓冲区大小 
 * Output:			p_data 存储读取数据的缓冲区
 * Return:			写入的长度
 * Author:			
 * Revise:			V1.0
 * Description:	窥读数据
 ***************************************************************/
size_t lwrb_peek(const lwrb_t* c_p_buff, size_t skip_count, void* p_data, size_t btp)
{
	size_t full, tocopy, r;
	uint8_t* p_d = p_data;

	/*判断有效性*/
	if (!BUF_IS_VALID(c_p_buff) || p_data == NULL || btp == 0)
	{
		return 0;
	}

	/*计算可读取的最大字节数*/
	full = lwrb_get_full(c_p_buff);
	/*跳过的长度过长 读取长度为0 */
	if (skip_count >= full)
	{
		return 0;
	}

	/*读取当前读指针*/
	r = LWRB_LOAD(c_p_buff->r, memory_order_relaxed);
	r += skip_count;
	/*跳过到了开头*/
	if (r >= c_p_buff->size)
	{
		r -= c_p_buff->size;
	}
	full -= skip_count;	//更新可读长度

	/*检查跳过后可读取的最大字节数*/
	btp = BUF_MIN(full, btp);
	if (btp == 0)
	{
		return 0;
	}

	/* 1 将读取缓冲区的线性部分 */
	tocopy = BUF_MIN(c_p_buff->size - r, btp);	//最大读取长度
	BUF_MEMCPY(p_d, &c_p_buff->p_buff[r], tocopy);	//读取数据
	btp -= tocopy;	//更新需要读取的长度

	/* 2 从缓冲区开始读取数据（溢出部分）*/
	if (btp > 0)
	{
		BUF_MEMCPY(&p_d[tocopy], c_p_buff->p_buff, btp);
	}

	/*返回读取的长度*/
	return tocopy + btp;
}

/***************************************************************
 * Name:				lwrb_get_free()
 * Input :			c_p_buff 句柄
 * Output:			void
 * Return:			空闲的长度
 * Author:			
 * Revise:			V1.0
 * Description:	获取空闲长度
 ***************************************************************/
size_t lwrb_get_free(const lwrb_t* c_p_buff)
{
	size_t size, w, r;

	/*判断有效性*/
	if (!BUF_IS_VALID(c_p_buff))
	{
		return 0;
	}

	/*
		* 将缓冲区指针复制到具有原子访问权限的局部变量。
		* 为了确保线程安全（仅在单入口、单出口FIFO模式用例中），将缓冲区r和w值写入本地w和r变量非常重要。
		* 局部变量将确保if语句始终使用相同的值，即使buff->w或buff->r在中断处理期间发生了变化。
		* 它们在负载操作期间可能会发生变化，重要的是，在这些分配之后的if-elseif操作期间，它们不会发生变化。
		* lwrb_get_free仅用于写入目的，当处于FIFO模式时，则：
		* - buff->w指针不会因另一个进程/中断而改变，因为我们刚才处于写入模式
		* - buff->r指针可能会被另一个进程更改。如果在将buff->r加载到局部变量后它发生了变化,
		*    缓冲区将看到“可用大小”小于实际大小。这不是问题，
		*    应用程序始终可以再次尝试将更多数据写入仅在复制操作期间读取的剩余可用内存
		*/
	w = LWRB_LOAD(c_p_buff->w, memory_order_relaxed);
	r = LWRB_LOAD(c_p_buff->r, memory_order_relaxed);

	if (r == w)			//r=w相等 全空
	{
		size = c_p_buff->size;
	}
	else if (r > w)	//r>w 
	{
		size = r - w;
	}
	else						//r<w 线性部分写完了 开始从头写
	{
		size = c_p_buff->size - (w - r);
	}

	/* 缓冲区空闲大小始终比实际大小小1 */
	return size - 1;
}

/***************************************************************
 * Name:				lwrb_get_full()
 * Input :			c_p_buff 句柄
 * Output:			void
 * Return:			使用的长度
 * Author:			
 * Revise:			V1.0
 * Description:	获取使用的长度
 ***************************************************************/
size_t lwrb_get_full(const lwrb_t* c_p_buff)
{
	size_t size, w, r;

	/*判断有效性*/
	if (!BUF_IS_VALID(c_p_buff))
	{
		return 0;
	}

	/*
		* 将缓冲区指针复制到具有原子访问权限的局部变量。
		* 为了确保线程安全（仅在单入口、单出口FIFO模式用例中），将缓冲区r和w值写入本地w和r变量非常重要。
		* 局部变量将确保if语句始终使用相同的值，即使buff->w或buff->r在中断处理期间发生了变化。
		* 它们在负载操作期间可能会发生变化，重要的是，在这些分配之后的if-elseif操作期间，它们不会发生变化。
		* lwrb_get_free仅用于写入目的，当处于FIFO模式时，则：
		* - buff->w指针不会因另一个进程/中断而改变，因为我们刚才处于写入模式
		* - buff->r指针可能会被另一个进程更改。如果在将buff->r加载到局部变量后它发生了变化,
		*    缓冲区将看到“可用大小”小于实际大小。这不是问题，
		*    应用程序始终可以再次尝试将更多数据写入仅在复制操作期间读取的剩余可用内存
		*/
	w = LWRB_LOAD(c_p_buff->w, memory_order_relaxed);
	r = LWRB_LOAD(c_p_buff->r, memory_order_relaxed);

	if (w == r)			//w = r 没有使用
	{
		size = 0;
	}
	else if (w > r)	//w > r 没有使用
	{
		size = w - r;
	}
	else						//w < r 线性部分写完了 开始从头写
	{
		size = c_p_buff->size - (r - w);
	}

	return size;
}

/***************************************************************
 * Name:				lwrb_get_linear_block_read_address()
 * Input :			c_p_buff 句柄
 * Output:			void
 * Return:			读缓存地址指针
 * Author:			
 * Revise:			V1.0
 * Description:	获取缓冲区的读缓存地址指针
 ***************************************************************/
void* lwrb_get_linear_block_read_address(const lwrb_t* c_p_buff)
{
	/*判断有效性*/
	if (!BUF_IS_VALID(c_p_buff))
	{
		return NULL;
	}

	return &c_p_buff->p_buff[c_p_buff->r];
}

/***************************************************************
 * Name:				lwrb_get_linear_block_read_length()
 * Input :			c_p_buff 句柄
 * Output:			void
 * Return:			使用的长度
 * Author:			
 * Revise:			V1.0
 * Description:	获取线性部分可以使用读取的空间
 ***************************************************************/
size_t lwrb_get_linear_block_read_length(const lwrb_t* c_p_buff)
{
	size_t len, w, r;

	/*判断有效性*/
	if (!BUF_IS_VALID(c_p_buff))
	{
		return 0;
	}

	/*使用临时值，以防在操作过程中更改。有关为什么这是可以的更多信息，请参阅lwrb_buff_free或lwrb_bff_full函数。*/
	w = LWRB_LOAD(c_p_buff->w, memory_order_relaxed);
	r = LWRB_LOAD(c_p_buff->r, memory_order_relaxed);

	if (w > r)
	{
		len = w - r;
	}
	else if (r > w)
	{
		len = c_p_buff->size - r;
	}
	else
	{
		len = 0;
	}
	return len;
}

/***************************************************************
 * Name:				lwrb_skip()
 * Input :			p_buff 句柄 len 跳过的长度
 * Output:			void
 * Return:			跳过的长度
 * Author:			
 * Revise:			V1.0
 * Description:	跳过读部分数据
 ***************************************************************/
size_t lwrb_skip(lwrb_t* p_buff, size_t len)
{
	size_t full, r;

	if (!BUF_IS_VALID(p_buff) || len == 0)
	{
		return 0;
	}

	full = lwrb_get_full(p_buff);	//使用的长度
	len = BUF_MIN(len, full);		//能跳过的最大长度
	r = LWRB_LOAD(p_buff->r, memory_order_acquire);	//读的地址
	/*增加跳过的长度*/
	r += len;
	if (r >= p_buff->size)
	{
		r -= p_buff->size;
	}

	/*保存读地址*/
	LWRB_STORE(p_buff->r, r, memory_order_release);
	/*运行回调函数*/
	BUF_SEND_EVT(p_buff, LWRB_EVT_READ, len);
	
	return len;
}

/***************************************************************
 * Name:				lwrb_get_linear_block_write_address()
 * Input :			c_p_buff 句柄
 * Output:			void
 * Return:			写缓存地址指针
 * Author:			
 * Revise:			V1.0
 * Description:	获取缓冲区的写缓存地址指针
 ***************************************************************/
void* lwrb_get_linear_block_write_address(const lwrb_t* c_p_buff)
{
	if (!BUF_IS_VALID(c_p_buff))
	{
		return NULL;
	}
	return &c_p_buff->p_buff[c_p_buff->w];
}

/***************************************************************
 * Name:				lwrb_get_linear_block_write_length()
 * Input :			c_p_buff 句柄
 * Output:			void
 * Return:			使用的长度
 * Author:			
 * Revise:			V1.0
 * Description:	获取线性部分可以使用写入的空间
 ***************************************************************/
size_t lwrb_get_linear_block_write_length(const lwrb_t* c_p_buff)
{
	size_t len, w, r;

	/*判断有效性*/
	if (!BUF_IS_VALID(c_p_buff))
	{
		return 0;
	}

	/*使用临时值，以防在操作过程中更改。有关为什么这是可以的更多信息，请参阅lwrb_buff_free或lwrb_bff_full函数。*/
	w = LWRB_LOAD(c_p_buff->w, memory_order_relaxed);
	r = LWRB_LOAD(c_p_buff->r, memory_order_relaxed);

	if (w >= r)
	{
		len = c_p_buff->size - w;
		/*防止写满了但是没读取过 被判断为空*/
		if (r == 0)
		{
			--len;
		}
	}
	else
	{
		len = r - w - 1;
	}
	
	return len;
}

/***************************************************************
 * Name:				lwrb_advance()
 * Input :			p_buff 句柄 len 跳过的长度
 * Output:			void
 * Return:			跳过的长度
 * Author:			
 * Revise:			V1.0
 * Description:	跳过写部分数据
 ***************************************************************/
size_t lwrb_advance(lwrb_t* p_buff, size_t len)
{
	size_t free, w;

	/*判断有效性*/
	if (!BUF_IS_VALID(p_buff) || len == 0)
	{
		return 0;
	}

	/* Use local variables before writing back to main structure */
	free = lwrb_get_free(p_buff);	//获取空闲长度
	len = BUF_MIN(len, free);		//获取最大跳过长度
	w = LWRB_LOAD(p_buff->w, memory_order_acquire);	//获取写的地址
	/*增加跳过的长度*/
	w += len;
	if (w >= p_buff->size)
	{
		w -= p_buff->size;
	}

	/*保存写地址*/
	LWRB_STORE(p_buff->w, w, memory_order_release);
	/*运行回调函数*/
	BUF_SEND_EVT(p_buff, LWRB_EVT_WRITE, len);

	return len;
}

/***************************************************************
 * Name:				lwrb_find()
 * Input :			c_p_buff 句柄 c_p_bts 查找的字符串 len 字符串的长度 start_offset 开始查找的偏移量
 * Output:			p_found_idx 查找到的地址
 * Return:			1 找到 0 没找到
 * Author:			
 * Revise:			V1.0
 * Description:	在缓冲区中查找字符串
 ***************************************************************/
uint8_t lwrb_find(const lwrb_t* c_p_buff, const void* c_p_bts, size_t len, size_t start_offset, size_t* p_found_idx)
{
	size_t full, r;
	uint8_t found = 0;
	const uint8_t* c_p_needle = c_p_bts;

	/*判断有效性*/
	if (!BUF_IS_VALID(c_p_buff) || c_p_needle == NULL || len == 0 || p_found_idx == NULL)
	{
		return 0;
	}

	*p_found_idx = 0;

	/*获取使用的空间*/
	full = lwrb_get_full(c_p_buff);
	/*验证初始条件*/
	if (full < (len + start_offset))
	{
		return 0;
	}

	/* for循环的最大数量为缓冲区长度的buff_full-input_len-start_offset */
	for (size_t skip_x = start_offset; !found && skip_x <= (full - start_offset - len); ++skip_x)
	{
		found = 1; //默认找到

		/*准备查找的起点*/
		r = c_p_buff->r + skip_x;
		if (r >= c_p_buff->size)
		{
			r -= c_p_buff->size;
		}

		/*在缓存区中查找*/
		for (size_t i = 0; i < len; ++i)
		{
			if (c_p_buff->p_buff[r] != c_p_needle[i])
			{
				found = 0;
				break;
			}

			if (++r >= c_p_buff->size)
			{
				r = 0;
			}
		}
		
		/*完全匹配*/
		if (found)
		{
			*p_found_idx = skip_x;
		}
	}
	return found;
}



