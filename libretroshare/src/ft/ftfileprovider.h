/*
 * libretroshare/src/ft ftFileProvider.h
 *
 * File Transfer for RetroShare.
 *
 * Copyright 2008 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#ifndef FT_FILE_PROVIDER_HEADER
#define FT_FILE_PROVIDER_HEADER

/* 
 * ftFileProvider.
 *
 */
#include <iostream>
#include <stdint.h>
#include "util/rsthreads.h"
#include "rsiface/rsfiles.h"

class ftFileProvider
{
	public:
		ftFileProvider(std::string path, uint64_t size, std::string hash);
		virtual ~ftFileProvider();

		virtual bool 	getFileData(uint64_t offset, uint32_t &chunk_size, void *data);
		virtual bool    FileDetails(FileInfo &info);
		std::string getHash();
		uint64_t getFileSize();
		bool fileOk();

		void setPeerId(const std::string& id) ;

		// Provides a client for the map of chunks actually present in the file. If the provider is also
		// a file creator, because the file is actually being downloaded, then the map may be partially complete.
		// Otherwize, a plain map is returned.
		//
		virtual void getAvailabilityMap(CompressedChunkMap& cmap) ;

		// a ftFileProvider feeds a distant peer. To display what the peers already has, we need to store/read this info.
		void getClientMap(const std::string& peer_id,CompressedChunkMap& cmap,bool& map_is_too_old) ;
		void setClientMap(const std::string& peer_id,const CompressedChunkMap& cmap) ;

		time_t    lastTS;   		// used for checking if it's alive
	protected:
		virtual	int initializeFileAttrs(); /* does for both */

		uint64_t    mSize;
		std::string hash;
		std::string file_name;
		FILE *fd;

		/* 
		 * Structure to gather statistics FIXME: lastRequestor - figure out a 
		 * way to get last requestor (peerID)
		 */
		std::string lastRequestor;
		uint64_t   req_loc;
		uint32_t   req_size;
		time_t    lastTS_t;   	// used for estimating transfer rate.

		// these two are used for speed estimation
		float 	  transfer_rate ;
		uint32_t		total_size ;

		// Info about what the downloading peer already has
		std::map<std::string,std::pair<CompressedChunkMap,time_t> > clients_chunk_maps ;

		/* 
		 * Mutex Required for stuff below 
		 */
		RsMutex ftcMutex;
};


#endif // FT_FILE_PROVIDER_HEADER
