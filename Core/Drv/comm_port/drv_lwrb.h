/**
 * \file            lwrb.h
 * \brief           LwRB - Lightweight ring buffer
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
#ifndef LWRB_HDR_H
#define LWRB_HDR_H

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \defgroup        LWRB Lightweight ring buffer manager
 * \brief           Lightweight ring buffer manager
 * \{
 */
#define LWRB_DISABLE_ATOMIC
#ifdef LWRB_DISABLE_ATOMIC
typedef unsigned long lwrb_ulong_t;
#else
#include <stdatomic.h>
typedef atomic_ulong lwrb_ulong_t;
#endif

/*缓冲区操作的事件类型*/
typedef enum {
	LWRB_EVT_READ,	//读事件
	LWRB_EVT_WRITE,	//写事件
	LWRB_EVT_RESET,	//重置事件
} lwrb_evt_type_t;

/*缓冲区结构声明*/
struct lwrb;

/***************************************************************
 * Name:				lwrb_evt_fn()
 * Input :			p_buff 缓存区 evt 事件 bp 大小
 * Output:			void
 * Return:			void
 * Author:			
 * Revise:			V1.0
 * Description:	事件函数抽象
 ***************************************************************/
typedef void (*lwrb_evt_fn)(struct lwrb* p_buff, lwrb_evt_type_t evt, size_t bp);

/*句柄结构*/
typedef struct lwrb {
	uint8_t* p_buff;	//指向缓冲区数据的指针	当 buff！= NULL 而且 size > 0 缓冲区被视为已初始化
	size_t size;		//缓冲区数据的大小	缓冲区使用的大小比size 小 1
	lwrb_ulong_t r;	//下一个读取指针 当r==w时 缓冲区被认为是空的 当w==r-1时 缓冲区是满的
	lwrb_ulong_t w;	//下一个写入指针 当r==w时 缓冲区被认为是空的 当w==r-1时 缓冲区是满的
	lwrb_evt_fn evt_fn;	//事件回调函数的指针
} lwrb_t;

uint8_t lwrb_init(lwrb_t* p_buff, void* p_buff_buff, size_t size);
uint8_t lwrb_is_ready(lwrb_t* p_buff);
void lwrb_free(lwrb_t* p_buff);
void lwrb_reset(lwrb_t* p_buff);
void lwrb_set_evt_fn(lwrb_t* p_buff, lwrb_evt_fn fn);

/* 读写函数 */
size_t lwrb_write(lwrb_t* p_buff, const void* c_p_data, size_t btw);
size_t lwrb_read(lwrb_t* p_buff, void* p_data, size_t btr);
size_t lwrb_peek(const lwrb_t* c_p_buff, size_t skip_count, void* p_data, size_t btp);

/* 缓存区存储信息 */
size_t lwrb_get_free(const lwrb_t* c_p_buff);
size_t lwrb_get_full(const lwrb_t* c_p_buff);

/* 读取数据块管理 */
void* lwrb_get_linear_block_read_address(const lwrb_t* c_p_buff);
size_t lwrb_get_linear_block_read_length(const lwrb_t* c_p_buff);
size_t lwrb_skip(lwrb_t* p_buff, size_t len);

/* 写入数据块管理 */
void* lwrb_get_linear_block_write_address(const lwrb_t* c_p_buff);
size_t lwrb_get_linear_block_write_length(const lwrb_t* c_p_buff);
size_t lwrb_advance(lwrb_t* p_buff, size_t len);

/* 缓冲区中搜索 */
uint8_t lwrb_find(const lwrb_t* c_p_buff, const void* c_p_bts, size_t len, size_t start_offset, size_t* p_found_idx);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWRB_HDR_H */
