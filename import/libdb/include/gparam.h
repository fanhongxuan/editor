/*
 * Copyright (c) 1997, 1998, 1999, 2000, 2007, 2008
 *	Tama Communications Corporation
 *
 * This file is part of GNU GLOBAL.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _GPARAM_H_
#define _GPARAM_H_
#ifndef __BORLANDC__
#include <sys/param.h>
#endif

		/** max length of filter	*/
#define MAXFILLEN	1024
		/** max length of ident		*/
#define IDENTLEN	512
		/** max length of buffer	*/
#define MAXBUFLEN	1024
		/** max length of property	*/
#define MAXPROPLEN	1024
		/** max length of argument	*/
#define MAXARGLEN	512
		/** max length of token		*/
#define MAXTOKEN	512
		/** max length of fid		*/
#define MAXFIDLEN	32

#ifndef MAXPATHLEN
		/** max length of path		*/
#define MAXPATHLEN	1024
#endif
		/** max length of record key	*/
#if MAXPATHLEN < 1024
#define MAXKEYLEN	1024
#else
#define MAXKEYLEN	MAXPATHLEN
#endif
		/** max length of URL		*/
#define MAXURLLEN	1024
/*
 * The default cache size of db library is 50MB.
 * The minimum size is 500KB.
 */
		/** default cache size 50MB	*/
#define GTAGSCACHE	50000000
		/** minimum cache size 500KB	*/
#define GTAGSMINCACHE	500000

#endif /* ! _GPARAM_H_ */
