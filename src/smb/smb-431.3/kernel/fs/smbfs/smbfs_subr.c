/*
 * Copyright (c) 2000-2001, Boris Popov
 * All rights reserved.
 *
 * Portions Copyright (C) 2001 - 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by Boris Popov.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: smbfs_subr.c,v 1.24 2006/02/03 04:04:12 lindak Exp $
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/sysctl.h>

#include <sys/kauth.h>

#include <sys/smb_apple.h>

#include <netsmb/smb.h>
#include <netsmb/smb_conn.h>
#include <netsmb/smb_subr.h>
#include <netsmb/smb_rq.h>
#include <netsmb/smb_dev.h>

#include <fs/smbfs/smbfs.h>
#include <fs/smbfs/smbfs_node.h>
#include <fs/smbfs/smbfs_subr.h>
#include <netsmb/smb_converter.h>

MALLOC_DEFINE(M_SMBFSDATA, "SMBFS data", "SMBFS private data");

/* 
 * Time & date conversion routines taken from msdosfs. Although leap
 * year calculation is bogus, it's sufficient before 2100 :)
 */
/*
 * This is the format of the contents of the deTime field in the direntry
 * structure.
 * We don't use bitfields because we don't know how compilers for
 * arbitrary machines will lay them out.
 */
#define DT_2SECONDS_MASK	0x1F	/* seconds divided by 2 */
#define DT_2SECONDS_SHIFT	0
#define DT_MINUTES_MASK		0x7E0	/* minutes */
#define DT_MINUTES_SHIFT	5
#define DT_HOURS_MASK		0xF800	/* hours */
#define DT_HOURS_SHIFT		11

/*
 * This is the format of the contents of the deDate field in the direntry
 * structure.
 */
#define DD_DAY_MASK		0x1F	/* day of month */
#define DD_DAY_SHIFT		0
#define DD_MONTH_MASK		0x1E0	/* month */
#define DD_MONTH_SHIFT		5
#define DD_YEAR_MASK		0xFE00	/* year - 1980 */
#define DD_YEAR_SHIFT		9
/*
 * Total number of days that have passed for each month in a regular year.
 */
static u_short regyear[] = {
	31, 59, 90, 120, 151, 181,
	212, 243, 273, 304, 334, 365
};

/*
 * Total number of days that have passed for each month in a leap year.
 */
static u_short leapyear[] = {
	31, 60, 91, 121, 152, 182,
	213, 244, 274, 305, 335, 366
};

void
smb_time_local2server(struct timespec *tsp, int tzoff, u_int32_t *seconds)
{
	/*
	 * XXX - what if we connected to the server when it was in
	 * daylight savings/summer time and we've subsequently switched
	 * to standard time, or vice versa, so that the time zone
	 * offset we got from the server is now wrong?
	 */
	*seconds = (u_int32_t)(tsp->tv_sec - tzoff * 60);
}

void
smb_time_server2local(u_long seconds, int tzoff, struct timespec *tsp)
{
	/*
	 * XXX - what if we connected to the server when it was in
	 * daylight savings/summer time and we've subsequently switched
	 * to standard time, or vice versa, so that the time zone
	 * offset we got from the server is now wrong?
	 */
	tsp->tv_sec = seconds + tzoff * 60;
}

/*
 * Number of seconds between 1970 and 1601 year
 */
u_int64_t DIFF1970TO1601 = 11644473600ULL;

/*
 * Time from server comes as UTC, so no need to use tz
 */
void
smb_time_NT2local(u_int64_t nsec, int tzoff, struct timespec *tsp)
{
	#pragma unused(tzoff)
	/* So the problem here is tsp->tv_sec is a long */
	smb_time_server2local((u_long)(nsec / 10000000 - DIFF1970TO1601), 0, tsp);
}

void
smb_time_local2NT(struct timespec *tsp, int tzoff, u_int64_t *nsec, int fat_fstype)
{
	#pragma unused(tzoff)
	u_int32_t seconds;

	smb_time_local2server(tsp, 0, &seconds);
	/* 
	 * Remember that FAT file systems only have a two second interval for 
	 * time. NTFS volumes do not have have this limitation, so only force 
	 * the two second interval on FAT File Systems.
	 */
	if (fat_fstype)
		*nsec = (((u_int64_t)(seconds) & ~1) + DIFF1970TO1601) * (u_int64_t)10000000;
	else
		*nsec = ((u_int64_t)seconds + DIFF1970TO1601) * (u_int64_t)10000000;
}

/*
 * The number of seconds between Jan 1, 1970 and Jan 1, 1980. In that
 * interval there were 8 regular years and 2 leap years.
 */
#define	SECONDSTO1980	(((8 * 365) + (2 * 366)) * (24 * 60 * 60))

static u_short lastdosdate;
static u_long  lastseconds;

void
smb_dos2unixtime(u_int dd, u_int dt, u_int dh, int tzoff,
	struct timespec *tsp)
{
	u_int32_t seconds;
	u_int32_t month;
	u_int32_t year;
	u_int32_t days;
	u_int16_t *months;

	if (dd == 0) {
		tsp->tv_sec = 0;
		tsp->tv_nsec = 0;
		return;
	}
	seconds = (((dt & DT_2SECONDS_MASK) >> DT_2SECONDS_SHIFT) << 1)
	    + ((dt & DT_MINUTES_MASK) >> DT_MINUTES_SHIFT) * 60
	    + ((dt & DT_HOURS_MASK) >> DT_HOURS_SHIFT) * 3600
	    + dh / 100;
	 /* Invalid seconds field, Windows 98 can return a bogus value */
	if (seconds > (24*60*60)) {
		SMBERROR("Bad DOS time! seconds = %u\n", seconds);
		seconds = 0;
	}	
	/*
	 * If the year, month, and day from the last conversion are the
	 * same then use the saved value.
	 */
	if (lastdosdate != dd) {
		lastdosdate = dd;
		days = 0;
		year = (dd & DD_YEAR_MASK) >> DD_YEAR_SHIFT;
		days = year * 365;
		days += year / 4 + 1;	/* add in leap days */
		/*
		 * XXX - works in 2000, but won't work in 2100.
		 */
		if ((year & 0x03) == 0)
			days--;		/* if year is a leap year */
		months = year & 0x03 ? regyear : leapyear;
		month = (dd & DD_MONTH_MASK) >> DD_MONTH_SHIFT;
		if (month < 1 || month > 12) {
			SMBERROR("Bad DOS time! month = %u\n", month);
			month = 1;
		}
		if (month > 1)
			days += months[month - 2];
		days += ((dd & DD_DAY_MASK) >> DD_DAY_SHIFT) - 1;
		lastseconds = (days * 24 * 60 * 60) + SECONDSTO1980;
	}
	smb_time_server2local(seconds + lastseconds, tzoff, tsp);
	tsp->tv_nsec = (dh % 100) * 10000000;
}

static int smb_fphelp(struct smbmount *smp, struct mbchain *mbp, struct smb_vc *vcp, 
					  struct smbnode *np, int flags, size_t *lenp)
{
	struct smbnode  *npstack[SMBFS_MAXPATHCOMP]; 
	struct smbnode  **npp = &npstack[0]; 
	int i, error = 0;

	if (smp->sm_args.path) {
		if (SMB_UNICODE_STRINGS(vcp))
			error = mb_put_uint16le(mbp, '\\');
		else
			error = mb_put_uint8(mbp, '\\');
		if (!error && lenp)
			*lenp += SMB_UNICODE_STRINGS(vcp) ? 2 : 1;
		/* We have a starting path, that has already been converted add it to the path */
		if (!error)
			error = mb_put_mem(mbp, (c_caddr_t)smp->sm_args.path, smp->sm_args.path_len, MB_MSYSTEM);
		if (!error && lenp)
			*lenp += smp->sm_args.path_len;
	}
	/* This is a stream file, skip it. We always use the stream parent for the lookup */	
	if (np->n_flag & N_ISSTREAM)
		np = np->n_parent;

	i = 0;
	while (np->n_parent) {
		if (i++ == SMBFS_MAXPATHCOMP)
			return ENAMETOOLONG;
		*npp++ = np;
		np = np->n_parent;
	}

	while (i--) {
		np = *--npp;
		if (SMB_UNICODE_STRINGS(vcp))
			error = mb_put_uint16le(mbp, '\\');
		else
			error = mb_put_uint8(mbp, '\\');
		if (!error && lenp)
			*lenp += SMB_UNICODE_STRINGS(vcp) ? 2 : 1;
		if (error)
			break;
		lck_rw_lock_shared(&np->n_name_rwlock);
		error = smb_put_dmem(mbp, vcp, (char *)(np->n_name), np->n_nmlen, flags, lenp);
		lck_rw_unlock_shared(&np->n_name_rwlock);
		if (error)
			break;
	}
	return error;
}

int
smbfs_fullpath(struct mbchain *mbp, struct smb_vc *vcp, struct smbnode *dnp, 
			   const char *name, size_t *lenp, int name_flags, u_int8_t sep)
{
	int error; 
	size_t len = 0;

	if (lenp) {
		len = *lenp;
		*lenp = 0;
	}
	if (SMB_UNICODE_STRINGS(vcp)) {
		error = mb_put_padbyte(mbp);
		if (error)
			return error;
	}
	if (dnp != NULL) {
		struct smbmount *smp = dnp->n_mount;
		
		error = smb_fphelp(smp, mbp, vcp, dnp, UTF_SFM_CONVERSIONS, lenp);
		if (error)
			return error;
		if (((smp->sm_args.path == NULL) && (dnp->n_ino == 2) && !name))
			name = ""; /* to get one backslash below */
	}
	if (name) {
		if (SMB_UNICODE_STRINGS(vcp))
			error = mb_put_uint16le(mbp, sep);
		else
			error = mb_put_uint8(mbp, sep);
                if (!error && lenp)
                        *lenp += SMB_UNICODE_STRINGS(vcp) ? 2 : 1;
		if (error)
			return error;
		error = smb_put_dmem(mbp, vcp, name, len, name_flags, lenp);
		if (error)
			return error;
	}
	error = mb_put_uint8(mbp, 0);
	if (!error && lenp)
		lenp++;
	if (SMB_UNICODE_STRINGS(vcp) && error == 0) {
		error = mb_put_uint8(mbp, 0);
		if (!error && lenp)
			lenp++;
	}
	return error;
}

/* 
 * Given a UTF8 path create a netowrk path
 *
 * utf8str - Must be null terminated. 
 * network - May be UTF16 or ASCII
 * 
 */
int smbfs_fullpath_to_network(struct smb_share *ssp, char *utf8str, char *network, size_t *ntwrk_len, 
							  char ntwrk_delimiter, int flags)
{
	int error = 0;
	char * delimiter;
	size_t component_len;	/* component length*/
	size_t resid = *ntwrk_len;	/* Room left in the the network buffer */
	struct smb_vc *vcp = SSTOVC(ssp);
	size_t max_component_len = ssp->ss_maxfilenamelen+1;
	
	while (utf8str && resid) {
		DBG_ASSERT(resid > 0);	/* Should never fail */
			/* Find the next delimiter in the utf-8 string */
		delimiter = strchr(utf8str, '/');
		/* Remove the delimiter so we can get the component */
		if (delimiter)
			*delimiter = 0;
			/* Get the size of this component */
		component_len = strnlen(utf8str, max_component_len);
		error = smb_convert_to_network((const char **)&utf8str, &component_len, &network, &resid, flags, SMB_UNICODE_STRINGS(vcp));
		if (error)
			return error;
		/* Put the delimiter back and move the pointer pass it */
		if (delimiter)
			*delimiter++ = '/';
		utf8str = delimiter;
		/* If we have more to process then add a bacck slash */
		if (utf8str) {
			if (!resid)
				return E2BIG;
			resid -= 1;
			*network++ = ntwrk_delimiter;	/* Use the network delimter passed in */
			if ((SMB_UNICODE_STRINGS(vcp))) {
				if (!resid)
					return E2BIG;
				resid -= 1;
				*network++ = 0;
			}					
		}
	}
	*ntwrk_len -= resid;
	DBG_ASSERT((ssize_t)(*ntwrk_len) >= 0);
	return error;
}

/*
 * They want the mount to start at some path offest. Take the path they gave us and create a
 * buffer that can be added to the front of every path we send across the network. This new
 * buffer will already be convert to a network style string.
 */
void smbfs_create_start_path(struct smbmount *smp, struct smb_mount_args *args)
{
	int error;
	int flags = UTF_PRECOMPOSED|UTF_NO_NULL_TERM|UTF_SFM_CONVERSIONS;
	
	/* Just in case someone sends us a bad string */
	args->path[MAXPATHLEN-1] = 0;
	
	/* Path length cannot be bigger than MAXPATHLEN and cannot contain the null byte */
	args->path_len = (args->path_len < MAXPATHLEN) ? args->path_len : (MAXPATHLEN - 1);
	/* path should never end with a slash */
	if (args->path[args->path_len - 1] == '/') {
		args->path_len -= 1;
		args->path[args->path_len] = 0;
	}
	
	smp->sm_args.path_len = (args->path_len * 2) + 2;	/* Start with the max size */
	MALLOC(smp->sm_args.path, char *, smp->sm_args.path_len, M_SMBFSDATA, M_WAITOK);
	if (smp->sm_args.path == NULL) {
		smp->sm_args.path_len = 0;
		return;	/* Give up */
	}
	/* Convert it to a network style path */
	error = smbfs_fullpath_to_network(smp->sm_share, args->path, smp->sm_args.path, 
									  &smp->sm_args.path_len, '\\', flags);
	if (error || (smp->sm_args.path_len == 0)) {
		SMBDEBUG("Deep Path Failed %d\n", error);
		if (smp->sm_args.path)
			free(smp->sm_args.path, M_SMBFSDATA);
		smp->sm_args.path_len = 0;
		smp->sm_args.path = NULL;
	}
} 

void
smbfs_fname_tolocal(struct smbfs_fctx *ctx)
{
	struct smb_vc *vcp = SSTOVC(ctx->f_ssp);
	char *dst, *odst;
	const char *src;
	size_t inlen, outlen, length;

	if (ctx->f_nmlen == 0)
		return;
	/*
	 * In Mac OS X the local name can be larger and
	 * in-place conversions are not supported.
	 *
	 * Remeber that f_name gets reused. We need to make sure 
	 * it can hold a full UTF-16 name. Look at smbfs_smb_findopenLM2
	 * for the minimum size of f_name. Fixed this because of PR-4183132.
	 * The old code could cause a buffer overrun. A simple test for this
	 * is to have a 14 or less character file name followed by a 129 or 
	 * more character file name. may not cause a panic, but with a little
	 * debugging you can see it happen.
	 */
	if (SMB_UNICODE_STRINGS(vcp))
		length = MAX(ctx->f_nmlen * 9, SMB_MAXFNAMELEN*2); /* why 9 */
	else
		length = MAX(ctx->f_nmlen * 3, SMB_MAXFNAMELEN); /* why 3 */

	dst = malloc(length, M_SMBFSDATA, M_WAITOK);
	outlen = length;
	src = ctx->f_name;
	inlen = ctx->f_nmlen;
	
	odst = dst;
	(void)smb_convert_from_network(&src, &inlen, &dst, &outlen, UTF_SFM_CONVERSIONS, (SMB_UNICODE_STRINGS(vcp)));
	if (ctx->f_name)
		free(ctx->f_name, M_SMBFSDATA);
	ctx->f_name = odst;
	ctx->f_nmlen = length - outlen;
	return;
}

/*
 * Converts a network name to a local UTF-8 name.
 *
 * Returns a UTF-8 string or NULL.
 *	ntwrk_name - either UTF-16 or ASCII Code Page
 *	nmlen - on input the length of the network name
 *			on output the length of the UTF-8 name
 * NOTE:
 *	This routine will not free the ntwrk_name.
 */
char *
smbfs_ntwrkname_tolocal(struct smb_vc *vcp, const char *ntwrk_name, size_t *nmlen)
{
	char *dst, *odst = NULL;
	size_t inlen, outlen, length;

	if (!nmlen || *nmlen == 0)
		return NULL;
	/*
	 * In Mac OS X the local name can be larger and
	 * in-place conversions are not supported.
	 */
	if (SMB_UNICODE_STRINGS(vcp))
		length = *nmlen * 9; /* why 9 */
	else
		length = *nmlen * 3; /* why 3 */
	length = MAX(length, SMB_MAXFNAMELEN);
	dst = malloc(length, M_SMBFSDATA, M_WAITOK);
	outlen = length;
	inlen = *nmlen;
	odst = dst;
	(void)smb_convert_from_network( &ntwrk_name, &inlen, &dst, &outlen, UTF_SFM_CONVERSIONS, (SMB_UNICODE_STRINGS(vcp)));
	*nmlen = length - outlen;
	return odst;
}
