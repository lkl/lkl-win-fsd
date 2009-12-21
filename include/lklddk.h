#ifndef LKLDDK_H__
#define LKLDDK_H__

#include <ddk/ntddk.h>
#include <ddk/ntifs.h>

/*
 * Copyright (c) Lucian Adrian Grijincu, 2009
 *
 * These have been written by me based on publicly available information
 */

/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    The GNU General Public License is also available from:
    http://www.gnu.org/copyleft/gpl.html
*/

/*
 * Make a RTL string from a constant string literal.
 *
 * the 's' literal is a NUL terminated string. Because of this the
 * Length of the RTL string is the size of the string without the NUL
 * terminator.
 *
 * This macro works with both Unicode and ANSI constant string literals.
 */
#define RTL_CONSTANT_STRING(s) {			\
		.Length = sizeof(s) - sizeof(*(s)),	\
		.MaximumLength = sizeof(s),		\
		.Buffer = s}





/*
 * Get the number of elements of an array
 */
#define RTL_NUMBER_OF(v) ((int)(sizeof((v))/sizeof(*(v))))


/*
 * Get the severity code of a status value.
 * The first two bits of the status value represent the severity code.
 *
 * The structure of a status code is described in "Chapter 3:
 * Structured Driver Development" from "Windows NT File System
 * Internals, A Developer's Guide", by Rajeev Nagar, O'REILLY.
 */
#define _NT_STATUS_SEVERITY(x) ((NTSTATUS)(x) >> 30)

/*
 * Is the given status code and informational status code, a warning
 * or an error?  
 *
 * STATUS_SEVERITY_INFORMATIONAL, STATUS_SEVERITY_WARNING and
 * STATUS_SEVERITY_ERROR are defined by mingw
 */
#define NT_INFORMATION(x) (_NT_STATUS_SEVERITY(x) == STATUS_SEVERITY_INFORMATIONAL)
#define NT_WARNING(x)     (_NT_STATUS_SEVERITY(x) == STATUS_SEVERITY_WARNING)
#define NT_ERROR(x)       (_NT_STATUS_SEVERITY(x) == STATUS_SEVERITY_ERROR)





/********************************************************************/
/*
 * The rest of this file contains copy-pasted structures (with
 * possible modifications by me) from ntifs.gnu.h from the ext2fsd
 * project.
 *
 * I've copied these here becase the stock headers that come with
 * mingw32 are incomplete.
 */

//
//  This Fcb header is used for files which support caching
//  of compressed data, and related new support.
//
//  We start out by prefixing this structure with the normal
//  FsRtl header from above, which we have to do two different
//  ways for c++ or c.
//

#ifdef __cplusplus
typedef struct _FSRTL_ADVANCED_FCB_HEADER:FSRTL_COMMON_FCB_HEADER {
#else   // __cplusplus

typedef struct _FSRTL_ADVANCED_FCB_HEADER {

    //
    //  Put in the standard FsRtl header fields
    //
    FSRTL_COMMON_FCB_HEADER FsrtlCommonFcbHeader;

#endif // __cplusplus

    //
    //  This is a pointer to a Fast Mutex which may be used to
    //  properly synchronize access to the FsRtl header.  The
    //  Fast Mutex must be nonpaged.
    //

    PFAST_MUTEX FastMutex;

    //
    //  This is a pointer to a list head which may be used to queue
    //  up advances to EOF (end of file), via calls to the appropriate
    //  FsRtl routines.  This listhead may be paged.
    //

    PLIST_ENTRY PendingEofAdvances;

    //
    //  When FSRTL_FLAG_ADVANCED_HEADER is set, the following fields
    //  are present in the header.  If the compressed stream has not
    //  been initialized, all of the following fields will be NULL.
    //

    //
    //  This is the FileObect for the stream in which data is cached
    //  in its compressed form.
    //

    PFILE_OBJECT FileObjectC;

    //
    //  The following field points to the Section Object Pointers for
    //  the normal data stream used for cache coherency in the fast path.
    //

    PSECTION_OBJECT_POINTERS SectionObjectPointers;

} FSRTL_ADVANCED_FCB_HEADER;
typedef FSRTL_ADVANCED_FCB_HEADER *PFSRTL_ADVANCED_FCB_HEADER;

#endif//LKLDDK_H__
