/****************************************************************************
 *
 * Copyright 2019 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/
/****************************************************************************
 * os/binfmt/libelf/libelf_load.c
 *
 *   Copyright (C) 2012 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#include <sys/types.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>

#include <tinyara/arch.h>
#include <tinyara/addrenv.h>
#include <tinyara/mm/mm.h>
#include <tinyara/binfmt/elf.h>
#include <tinyara/common_logs/common_logs.h>
#include "libelf.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ELF_ALIGN_MASK   ((1 << CONFIG_ELF_ALIGN_LOG2) - 1)
#define ELF_ALIGNUP(a)   (((unsigned long)(a) + ELF_ALIGN_MASK) & ~ELF_ALIGN_MASK)
#define ELF_ALIGNDOWN(a) ((unsigned long)(a) & ~ELF_ALIGN_MASK)

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

/****************************************************************************
 * Private Constant Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/
/****************************************************************************
 * Name: elf_elfsize
 *
 * Description:
 *   Calculate total memory allocation for the ELF file.
 *
 * Returned Value:
 *   0 (OK) is returned on success and a negated errno is returned on
 *   failure.
 *
 ****************************************************************************/

static void elf_elfsize(struct elf_loadinfo_s *loadinfo)
{
	size_t textsize;
	size_t datasize;
#ifdef CONFIG_OPTIMIZE_APP_RELOAD_TIME
	size_t rosize = 0;
#endif
	int i;

	/* Accumulate the size each section into memory that is marked SHF_ALLOC */

	textsize = 0;
	datasize = 0;

	for (i = 0; i < loadinfo->ehdr.e_shnum; i++) {
		FAR Elf32_Shdr *shdr = &loadinfo->shdr[i];

		/* SHF_ALLOC indicates that the section requires memory during
		 * execution.
		 */

		if ((shdr->sh_flags & SHF_ALLOC) != 0) {
			/* SHF_WRITE indicates that the section address space is write-
			 * able
			 */

			if ((shdr->sh_flags & SHF_WRITE) != 0) {
				datasize += ELF_ALIGNUP(shdr->sh_size);
				if (shdr->sh_type == SHT_NOBITS) {
					loadinfo->binp->sizes[BIN_BSS] = ELF_ALIGNUP(shdr->sh_size);
				}
			} 
#ifdef CONFIG_OPTIMIZE_APP_RELOAD_TIME
			else if ((shdr->sh_flags & SHF_EXECINSTR) != 0) {
				textsize += ELF_ALIGNUP(shdr->sh_size);
			} else {
				rosize += ELF_ALIGNUP(shdr->sh_size);
			}
#else
			else {
				textsize += ELF_ALIGNUP(shdr->sh_size);
			}
#endif
		}
	}

	/* Save the allocation size */
	loadinfo->binp->sizes[BIN_TEXT] = textsize;
	loadinfo->binp->sizes[BIN_DATA] = datasize - loadinfo->binp->sizes[BIN_BSS];	/* Data size must contain size of data section only */
#ifdef CONFIG_OPTIMIZE_APP_RELOAD_TIME
	loadinfo->binp->sizes[BIN_RO] = rosize;
#endif

}

/****************************************************************************
 * Name: elf_loadfile
 *
 * Description:
 *   Read the section data into memory. Section addresses in the shdr[] are
 *   updated to point to the corresponding position in the memory.
 *
 * Returned Value:
 *   0 (OK) is returned on success and a negated errno is returned on
 *   failure.
 *
 ****************************************************************************/

static inline int elf_loadfile(FAR struct elf_loadinfo_s *loadinfo)
{
	FAR uint8_t *text;
	FAR uint8_t *data;
#ifdef CONFIG_OPTIMIZE_APP_RELOAD_TIME
	FAR uint8_t *ro;
#endif
	FAR uint8_t **pptr;
	int ret;
	int i;

	/* Read each section into memory that is marked SHF_ALLOC + SHT_NOBITS */

	binfo("Loaded sections:\n");
	text = (FAR uint8_t *)loadinfo->binp->sections[BIN_TEXT];
	data = (FAR uint8_t *)loadinfo->binp->sections[BIN_DATA];
#ifdef CONFIG_OPTIMIZE_APP_RELOAD_TIME
	ro = (FAR uint8_t *)loadinfo->binp->sections[BIN_RO];
#endif

	for (i = 0; i < loadinfo->ehdr.e_shnum; i++) {
		FAR Elf32_Shdr *shdr = &loadinfo->shdr[i];

		/* SHF_ALLOC indicates that the section requires memory during
		 * execution */

		if ((shdr->sh_flags & SHF_ALLOC) == 0) {
			continue;
		}

		/* SHF_WRITE indicates that the section address space is write-
		 * able
		 */

		if ((shdr->sh_flags & SHF_WRITE) != 0) {
			pptr = &data;
#ifdef CONFIG_OPTIMIZE_APP_RELOAD_TIME
		} else if ((shdr->sh_flags & SHF_EXECINSTR) != 0) {
			pptr = &text;
		} else {
			pptr = &ro;
		}
#else
		} else {
			pptr = &text;
		}
#endif


		/* SHT_NOBITS indicates that there is no data in the file for the
		 * section.
		 */

		if (shdr->sh_type != SHT_NOBITS) {
			/* Read the section data from sh_offset to the memory region */

			ret = elf_read(loadinfo, *pptr, shdr->sh_size, shdr->sh_offset);
			if (ret < 0) {
				berr("%s section %d: %d\n",clog_message_str[CMN_LOG_FILE_READ_ERROR], i, ret);
				return ret;
			}
		}

		/* Update sh_addr to point to copy in memory */

		binfo("%d. %08lx->%08lx\n", i, (unsigned long)shdr->sh_addr, (unsigned long)*pptr);

		shdr->sh_addr = (uintptr_t)*pptr;

		/* Setup the memory pointer for the next time through the loop */

		*pptr += ELF_ALIGNUP(shdr->sh_size);
	}

	return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: elf_load
 *
 * Description:
 *   Loads the binary into memory, allocating memory, performing relocations
 *   and initializing the data and bss segments.
 *
 * Returned Value:
 *   0 (OK) is returned on success and a negated errno is returned on
 *   failure.
 *
 ****************************************************************************/

int elf_load(FAR struct elf_loadinfo_s *loadinfo)
{
#ifdef CONFIG_CXX_EXCEPTION
	int exidx;
#endif
	int ret;

	binfo("loadinfo: %p\n", loadinfo);
	DEBUGASSERT(loadinfo && loadinfo->filfd >= 0);

	/* Load section headers into memory */

	ret = elf_loadshdrs(loadinfo);
	if (ret < 0) {
		berr("%s: #1 %d\n",clog_message_str[CMN_LOG_FAILED_OP], ret);
		goto errout_with_buffers;
	}

	/* Determine total size to allocate */

	elf_elfsize(loadinfo);

	/* Allocate (and zero) memory for the ELF file. */

	ret = elf_addrenv_alloc(loadinfo);
	if (ret < 0) {
		berr("%s #2 : %d\n",clog_message_str[CMN_LOG_FAILED_OP], ret);
		goto errout_with_buffers;
	}

	/* Load ELF section data into memory */

	ret = elf_loadfile(loadinfo);
	if (ret < 0) {
		berr("%s #3 : %d\n",clog_message_str[CMN_LOG_FAILED_OP], ret);
		goto errout_with_buffers;
	}

	/* Load static constructors and destructors. */

#ifdef CONFIG_BINFMT_CONSTRUCTORS
	ret = elf_loadctors(loadinfo);
	if (ret < 0) {
		berr("%s #4 : %d\n",clog_message_str[CMN_LOG_FAILED_OP], ret);
		goto errout_with_buffers;
	}

	ret = elf_loaddtors(loadinfo);
	if (ret < 0) {
		berr("%s #5 : %d\n",clog_message_str[CMN_LOG_FAILED_OP], ret);
		goto errout_with_buffers;
	}
#endif

#ifdef CONFIG_CXX_EXCEPTION
	exidx = elf_findsection(loadinfo, CONFIG_ELF_EXIDX_SECTNAME);
	if (exidx < 0) {
		binfo("elf_findsection: Exception Index section not found: %d\n", exidx);
	} else {
		up_init_exidx(loadinfo->shdr[exidx].sh_addr, loadinfo->shdr[exidx].sh_size);
	}
#endif

	return OK;

	/* Error exits */
errout_with_buffers:
	elf_unload(loadinfo);
	return ret;
}
