/* -*- Mode: C; tab-width: 4 -*-
 *
 * Copyright (c) 1997-2004 Apple Computer, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.

    Change History (most recent first):
    
$Log: WinServices.cpp,v $
Revision 1.2  2006/08/14 23:25:20  cheshire
Re-licensed mDNSResponder daemon source code under Apache License, Version 2.0

Revision 1.1  2004/06/18 05:23:33  rpantos
First checked in


*/

#include "WinServices.h"


//===========================================================================================================================
//	UTF8StringToStringObject
//===========================================================================================================================

OSStatus	UTF8StringToStringObject( const char *inUTF8, CString &inObject )
{
	OSStatus		err;
	int				n;
	BSTR			unicode;
	
	unicode = NULL;
	
	n = MultiByteToWideChar( CP_UTF8, 0, inUTF8, -1, NULL, 0 );
	if( n > 0 )
	{
		unicode = (BSTR) malloc( (size_t)( n * sizeof( wchar_t ) ) );
		if( !unicode )
		{
			err = ERROR_INSUFFICIENT_BUFFER;
			goto exit;
		}

		n = MultiByteToWideChar( CP_UTF8, 0, inUTF8, -1, unicode, n );
		try
		{
			inObject = unicode;
		}
		catch( ... )
		{
			err = ERROR_NO_UNICODE_TRANSLATION;
			goto exit;
		}
	}
	else
	{
		inObject = "";
	}
	err = ERROR_SUCCESS;
	
exit:
	if( unicode )
	{
		free( unicode );
	}
	return( err );
}
