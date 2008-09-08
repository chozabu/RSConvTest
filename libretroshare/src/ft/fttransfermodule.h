/*
 * Retroshare file transfer module: ftTransferModule.h
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
 
#ifndef FT_TRANSFER_MODULE_HEADER
#define FT_TRANSFER_MODULE_HEADER

/*
 * FUNCTION DESCRIPTION
 *
 * Each Transfer Module is paired up with a single File Creator, and responsible for the transfer of one file.
 * The Transfer Module is responsible for sending requests to peers at the correct data rates, and storing the returned data
 * in a FileCreator.
 * There are multiple Transfer Modules in the File Transfer system. Their requests are multiplexed through the Client Module. * The Transfer Module contains all the algorithms for sensible Data Requests.
 * It must be able to cope with varying data rates and dropped peers without flooding the system with too many requests.
 *
 */

#include <map>
#include <list>
#include <string>

#include "ft/ftfilecreator.h"
#include "ft/ftdatamultiplex.h"

#include "util/rsthreads.h"

const int  PQIPEER_INIT                 = 0x0000;
const int  PQIPEER_NOT_ONLINE           = 0x0001;
const int  PQIPEER_DOWNLOADING          = 0x0002;
const int  PQIPEER_IDLE                 = 0x0004;
const int  PQIPEER_SUSPEND              = 0x0010;

const uint32_t PQIPEER_OFFLINE_CHECK  = 120; /* check every 2 minutes */
const uint32_t PQIPEER_DOWNLOAD_TIMEOUT  = 60; /* time it out, -> offline after 60 secs */
const uint32_t PQIPEER_DOWNLOAD_CHECK    = 10; /* desired delta = 10 secs */
const uint32_t PQIPEER_DOWNLOAD_TOO_FAST = 8; /* 8 secs */
const uint32_t PQIPEER_DOWNLOAD_TOO_SLOW = 12; /* 12 secs */
const uint32_t PQIPEER_DOWNLOAD_MIN_DELTA = 5; /* 5 secs */

const uint32_t TRANSFER_START_MIN = 10000;  /* 10000 byte  min limit */
const uint32_t TRANSFER_START_MAX = 10000; /* 10000 byte max limit */
/*
class Request
{
public:
  	uint64_t offset;
  	uint32_t chunkSize;
};
*/
class peerInfo
{
public:
	peerInfo(std::string peerId_in,uint32_t state_in,uint32_t maxRate_in):
		peerId(peerId_in),state(state_in),desiredRate(maxRate_in),actualRate(0),
		offset(0),chunkSize(TRANSFER_START_MIN),receivedSize(0),lastTS(0)
	{
		return;
	}
  	std::string peerId;
  	uint32_t state;
  	double desiredRate;
  	double actualRate;

  	//current file data request
  	uint64_t offset;
  	uint32_t chunkSize;

  	//already received data size
  	uint32_t receivedSize;

  	time_t lastTS;
};

class ftFileStatus
{
	enum Status {
		PQIFILE_INIT,
		PQIFILE_NOT_ONLINE,
		PQIFILE_DOWNLOADING,
		PQIFILE_PAUSE,
		PQIFILE_COMPLETE,
		PQIFILE_FAIL,
		PQIFILE_FAIL_CANCEL,
		PQIFILE_FAIL_NOT_AVAIL,
		PQIFILE_FAIL_NOT_OPEN,
		PQIFILE_FAIL_NOT_SEEK,
		PQIFILE_FAIL_NOT_WRITE,
		PQIFILE_FAIL_NOT_READ,
		PQIFILE_FAIL_BAD_PATH
	};
public:
	ftFileStatus(std::string hash_in):hash(hash_in),stat(PQIFILE_INIT)
	{
		return;
	}
	std::string hash;
	Status stat;
};

class ftTransferModule 
{
public:
  ftTransferModule(ftFileCreator *fc, ftDataMultiplex *dm);
  ~ftTransferModule();

  //interface to download controller
  bool setFileSources(std::list<std::string> peerIds);
  bool setPeerState(std::string peerId,uint32_t state,uint32_t maxRate);  //state = ONLINE/OFFLINE
  uint32_t getDataRate(std::string peerId);
  bool pauseTransfer();
  bool resumeTransfer();
  bool cancelTransfer();
  bool completeFileTransfer();

  //interface to multiplex module
  bool recvFileData(std::string peerId, uint64_t offset, 
			uint32_t chunk_size, void *data);
  void requestData(std::string peerId, uint64_t offset, uint32_t chunk_size);

  //interface to file creator
  bool getChunk(uint64_t &offset, uint32_t &chunk_size);
  bool storeData(uint64_t offset, uint32_t chunk_size, void *data);

  int tick();

  std::string hash() { return mHash; }
  uint64_t    size() { return mSize; }
 
  //internal used functions
  void queryInactive();
  void adjustSpeed();

private:

  /* These have independent Mutexes / are const locally (no Mutex protection)*/
  ftFileCreator *mFileCreator;
  ftDataMultiplex *mMultiplexor;

  std::string mHash;
  uint64_t    mSize;

  RsMutex tfMtx; /* below is mutex protected */

  std::list<std::string>         mFileSources;
  std::map<std::string,peerInfo> mOnlinePeers;
  	
  bool     mFlag;  //1:transfer complete, 0: not complete
  double desiredRate;
  double actualRate;

  ftFileStatus mFileStatus; //used for pause/resume file transfer
};

#endif  //FT_TRANSFER_MODULE_HEADER
