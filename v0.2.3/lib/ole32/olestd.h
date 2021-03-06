/*
 * Copyright 2000 Abey George
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#if !defined( __WINE_OLESTD_H_ )
#define __WINE_OLESTD_H_

#if !defined(__cplusplus) && !defined( __TURBOC__)
#define NONAMELESSUNION     /* use strict ANSI standard (for DVOBJ.H) */
#endif

/* Clipboard format strings */
#define CF_EMBEDSOURCE      "Embed Source"
#define CF_EMBEDDEDOBJECT   "Embedded Object"
#define CF_LINKSOURCE       "Link Source"
#define CF_CUSTOMLINKSOURCE "Custom Link Source"
#define CF_OBJECTDESCRIPTOR "Object Descriptor"
#define CF_LINKSRCDESCRIPTOR "Link Source Descriptor"
#define CF_OWNERLINK        "OwnerLink"
#define CF_FILENAME         "FileName"

#define OleStdQueryOleObjectData(lpformatetc)   \
   (((lpformatetc)->tymed & TYMED_ISTORAGE) ?    \
         NOERROR : ResultFromScode(DV_E_FORMATETC))

#define OleStdQueryLinkSourceData(lpformatetc)   \
   (((lpformatetc)->tymed & TYMED_ISTREAM) ?    \
         NOERROR : ResultFromScode(DV_E_FORMATETC))

#define OleStdQueryObjectDescriptorData(lpformatetc)    \
   (((lpformatetc)->tymed & TYMED_HGLOBAL) ?    \
         NOERROR : ResultFromScode(DV_E_FORMATETC))

#define OleStdQueryFormatMedium(lpformatetc, tymd)  \
   (((lpformatetc)->tymed & tymd) ?    \
         NOERROR : ResultFromScode(DV_E_FORMATETC))

/* Make an independent copy of a MetafilePict */
#define OleStdCopyMetafilePict(hpictin, phpictout)  \
   (*(phpictout) = OleDuplicateData(hpictin,CF_METAFILEPICT,GHND|GMEM_SHARE))

#endif /* __WINE_OLESTD_H_ */
